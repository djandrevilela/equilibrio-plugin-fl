#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>

#include "PluginProcessor.h"

class BandMeter : public juce::Component {
public:
    explicit BandMeter(EquilibrioProcessor& p) : proc(p) {}
    void paint(juce::Graphics&) override;

private:
    EquilibrioProcessor& proc;
};

class EquilibrioEditor : public juce::AudioProcessorEditor, private juce::ChangeListener, private juce::Timer {
public:
    explicit EquilibrioEditor(EquilibrioProcessor&);
    ~EquilibrioEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
    void timerCallback() override;
    void refresh();
    juce::String T(const char* pt, const char* en) const;
    void styleBox(juce::TextEditor&);

    EquilibrioProcessor& proc;
    juce::LookAndFeel_V4 lnf;
    BandMeter meter;
    juce::ComboBox langBox, roleBox, genreBox, patBox, kickBox, keyBox;
    juce::ToggleButton eqTg, duckTg;
    juce::Slider amtS, depthS, attackS, releaseS, outS;
    juce::Label roleL, genreL, meterL, eqL, adviceL, scL, amtL, depthL, attackL, releaseL, kickL, keyL, outL, disclaimer;
    juce::TextButton genreBtn;
    juce::TextEditor eqBox, adviceBox, whyBox, kickText;
    juce::String lastAdvice;

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<CA>> cAtt;
    std::vector<std::unique_ptr<BA>> bAtt;
    std::vector<std::unique_ptr<SA>> sAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EquilibrioEditor)
};
