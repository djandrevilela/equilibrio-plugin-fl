#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include "Equilibrio.h"

extern const char* const kKeyNames[12];

struct EqSet {
    int count = 0;
    std::array<eqb::Biquad, 4> bq{};
};

class EquilibrioProcessor : public juce::AudioProcessor,
                            public juce::ChangeBroadcaster,
                            private juce::AsyncUpdater,
                            private juce::AudioProcessorValueTreeState::Listener {
public:
    EquilibrioProcessor();
    ~EquilibrioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override {
        return l.getMainOutputChannelSet() == l.getMainInputChannelSet() && !l.getMainOutputChannelSet().isDisabled();
    }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Equilibrio"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- API do editor ----
    juce::AudioProcessorValueTreeState apvts;
    eqb::Lang lang() const { return pi("lang") == 0 ? eqb::Lang::PT : eqb::Lang::EN; }
    eqb::Role role() const { return (eqb::Role) juce::jlimit(0, eqb::kNumRoles - 1, pi("role")); }
    eqb::Genre genre() const { return (eqb::Genre) juce::jlimit(0, eqb::kNumGenres - 1, pi("genre")); }
    void setParam(const juce::String& id, float realValue);
    void applyGenreDefaults();
    bool getBands(std::array<double, eqb::kBands>& out) const;  // false = sem sinal
    double duckMeter() const { return duckNow.load(); }

    void regenerateForTest() { regenerate(); }
    void processForTest(juce::AudioBuffer<float>& b) { juce::MidiBuffer m; processBlock(b, m); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged(const juce::String&, float) override;
    void handleAsyncUpdate() override;
    void regenerate();

    float pf(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }
    int pi(const juce::String& id) const { return (int) std::lround(pf(id)); }

    mutable juce::SpinLock dataLock;
    std::shared_ptr<const EqSet> published, audioData;
    std::vector<std::shared_ptr<const EqSet>> graveyard;
    juce::StringArray listenIds;
    std::atomic<double> sampleRateAtomic{44100.0};

    std::atomic<float>*pDuckOn, *pPattern, *pDepth, *pAttack, *pRelease, *pOut;
    struct Chan { double z1[4] = {0, 0, 0, 0}, z2[4] = {0, 0, 0, 0}; };
    std::array<Chan, 2> chans;
    // analisador
    std::array<eqb::Biquad, eqb::kBands> bandBq;
    std::array<std::array<double, 2>, eqb::kBands> bandZ{};
    std::array<double, eqb::kBands> bandPow{};
    std::array<std::atomic<float>, eqb::kBands> bandDb;
    std::atomic<double> duckNow{1.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EquilibrioProcessor)
};
