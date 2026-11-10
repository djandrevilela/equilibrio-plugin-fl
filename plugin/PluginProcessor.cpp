#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>

#include "PluginEditor.h"
#include "Util.h"

using namespace eqb;

const char* const kKeyNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

juce::AudioProcessorValueTreeState::ParameterLayout EquilibrioProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto choice = [&](const String& id, const String& name, StringArray items, int def) { l.add(std::make_unique<AudioParameterChoice>(ParameterID{id, 1}, name, items, def)); };
    auto flt = [&](const String& id, const String& name, float lo, float hi, float def) { l.add(std::make_unique<AudioParameterFloat>(ParameterID{id, 1}, name, NormalisableRange<float>(lo, hi), def)); };
    auto boo = [&](const String& id, const String& name, bool def) { l.add(std::make_unique<AudioParameterBool>(ParameterID{id, 1}, name, def)); };
    auto intp = [&](const String& id, const String& name, int lo, int hi, int def) { l.add(std::make_unique<AudioParameterInt>(ParameterID{id, 1}, name, lo, hi, def)); };
    StringArray keys, roles, genres{"Future Bass", "Progressive House", "Big Room"}, pats;
    for (auto* k : kKeyNames) keys.add(k);
    for (int i = 0; i < kNumRoles; ++i) roles.add(U8(roleName((Role) i, Lang::EN)));
    for (int i = 0; i < kNumDuckPatterns; ++i) pats.add(U8(duckPatternName((DuckPattern) i, Lang::EN)));
    choice("lang", "Language", StringArray{TR("Portugu\xC3\xAAs (PT)"), "English (UK)"}, 0);
    choice("role", "Role", roles, 1);
    choice("genre", "Genre", genres, 1);
    boo("eqOn", "EQ", true);
    flt("eqAmt", "EQ amount", 0.f, 1.f, 1.f);
    boo("duckOn", "Sidechain", false);
    choice("duckPat", "Pattern", pats, 0);
    flt("depth", "Depth", 0.f, 0.95f, 0.55f);
    flt("attack", "Attack ms", 0.5f, 20.f, 3.f);
    flt("release", "Release beats", 0.1f, 1.0f, 0.4f);
    intp("kickNote", "Kick note", 24, 43, 36);
    choice("key", "Key", keys, 9);
    flt("gOut", "Output dB", -12.f, 12.f, 0.f);
    return l;
}

EquilibrioProcessor::EquilibrioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "EQUILIBRIO", createLayout()) {
    pDuckOn = apvts.getRawParameterValue("duckOn");
    pPattern = apvts.getRawParameterValue("duckPat");
    pDepth = apvts.getRawParameterValue("depth");
    pAttack = apvts.getRawParameterValue("attack");
    pRelease = apvts.getRawParameterValue("release");
    pOut = apvts.getRawParameterValue("gOut");
    for (auto& b : bandDb) b = -200.f;
    juce::StringArray ids{"role", "eqAmt", "eqOn"};
    for (auto& id : ids) apvts.addParameterListener(id, this);
    listenIds = ids;
    regenerate();
}
EquilibrioProcessor::~EquilibrioProcessor() {
    cancelPendingUpdate();
    for (auto& id : listenIds) apvts.removeParameterListener(id, this);
}

void EquilibrioProcessor::prepareToPlay(double sr, int) {
    sampleRateAtomic = sr;
    for (int i = 0; i < kBands; ++i) bandBq[(size_t) i] = designBandpass(bandCentre(i), std::max(0.5, bandCentre(i) / (bandHigh(i) - bandLow(i))), sr);
    for (auto& z : bandZ) z = {0, 0};
    bandPow.fill(0);
    for (auto& c : chans) { for (auto& z : c.z1) z = 0; for (auto& z : c.z2) z = 0; }
    regenerate();
}

void EquilibrioProcessor::setParam(const juce::String& id, float v) {
    if (auto* p = apvts.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}
void EquilibrioProcessor::applyGenreDefaults() {
    const auto s = sidechainFor(genre());
    setParam("depth", (float) s.depth);
    setParam("attack", (float) s.attackMs);
    setParam("release", (float) s.releaseBeats);
    setParam("duckPat", (float) (int) patternFor(genre()));
}
void EquilibrioProcessor::parameterChanged(const juce::String&, float) { triggerAsyncUpdate(); }
void EquilibrioProcessor::handleAsyncUpdate() {
    regenerate();
    sendChangeMessage();
}

void EquilibrioProcessor::regenerate() {
    auto set = std::make_shared<EqSet>();
    const double sr = sampleRateAtomic.load();
    if (pf("eqOn") > 0.5f) {
        auto f = suggestedEq(role(), Lang::EN);
        const double amt = pf("eqAmt");
        for (size_t i = 0; i < f.size() && i < 4; ++i) {
            const bool pass = f[i].type == FilterType::HighPass || f[i].type == FilterType::LowPass;
            set->bq[(size_t) set->count++] = designFilter(f[i].type, f[i].freq, pass ? 0.0 : f[i].gainDb * amt, f[i].q, sr);
        }
    }
    {
        juce::SpinLock::ScopedLockType sl(dataLock);
        if (published) graveyard.push_back(published);
        published = set;
    }
    graveyard.erase(std::remove_if(graveyard.begin(), graveyard.end(), [](const auto& p) { return p.use_count() == 1; }), graveyard.end());
}

bool EquilibrioProcessor::getBands(std::array<double, kBands>& out) const {
    bool any = false;
    for (int i = 0; i < kBands; ++i) { out[(size_t) i] = bandDb[(size_t) i].load(); any |= out[(size_t) i] > -150; }
    return any;
}

void EquilibrioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = std::min(2, buffer.getNumChannels());
    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    {
        juce::SpinLock::ScopedTryLockType tl(dataLock);
        if (tl.isLocked()) audioData = published;
    }
    // ---- analisador (sinal de entrada) ----
    {
        std::array<double, kBands> acc{};
        double total = 0;
        for (int i = 0; i < n; ++i) {
            double x = 0;
            for (int c = 0; c < nch; ++c) x += buffer.getReadPointer(c)[i];
            x /= nch;
            total += x * x;
            for (int b = 0; b < kBands; ++b) {
                const auto& q = bandBq[(size_t) b];
                auto& z = bandZ[(size_t) b];
                const double y = q.b0 * x + z[0];
                z[0] = q.b1 * x - q.a1 * y + z[1];
                z[1] = q.b2 * x - q.a2 * y;
                acc[(size_t) b] += y * y;
            }
        }
        const double a = std::exp(-(double) n / sr / 0.4);
        for (int b = 0; b < kBands; ++b) bandPow[(size_t) b] = a * bandPow[(size_t) b] + (1.0 - a) * acc[(size_t) b] / std::max(1, n);
        const bool sig = total / std::max(1, n) > 1e-9;
        for (int b = 0; b < kBands; ++b)
            bandDb[(size_t) b] = sig && bandPow[(size_t) b] > 1e-18 ? (float) (10.0 * std::log10(bandPow[(size_t) b] / (bandHigh(b) - bandLow(b)))) : -200.f;
    }
    // ---- EQ ----
    const EqSet* es = audioData.get();
    if (es && es->count > 0)
        for (int c = 0; c < nch; ++c) {
            float* d = buffer.getWritePointer(c);
            auto& ch = chans[(size_t) c];
            for (int s = 0; s < es->count; ++s) {
                const auto& q = es->bq[(size_t) s];
                double z1 = ch.z1[s], z2 = ch.z2[s];
                for (int i = 0; i < n; ++i) {
                    const double x = d[i];
                    const double y = q.b0 * x + z1;
                    z1 = q.b1 * x - q.a1 * y + z2;
                    z2 = q.b2 * x - q.a2 * y;
                    d[i] = (float) y;
                }
                ch.z1[s] = z1; ch.z2[s] = z2;
            }
        }
    // ---- sidechain sincronizado ----
    double duck = 1.0;
    if (pDuckOn->load() > 0.5f)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (pos->getIsPlaying())
                    if (auto ppq = pos->getPpqPosition()) {
                        const double bpm = pos->getBpm().hasValue() ? *pos->getBpm() : 120.0;
                        const double bps = bpm / 60.0 / sr;
                        const Sidechain sc{pDepth->load(), pAttack->load(), pRelease->load()};
                        const auto pat = (DuckPattern) juce::jlimit(0, kNumDuckPatterns - 1, (int) std::lround(pPattern->load()));
                        for (int i = 0; i < n; ++i) {
                            const double g = duckGainAt(*ppq + i * bps, pat, sc, bpm);
                            for (int c = 0; c < nch; ++c) buffer.getWritePointer(c)[i] *= (float) g;
                            duck = g;
                        }
                    }
    duckNow = duck;
    const float og = std::pow(10.0f, pOut->load() / 20.0f);
    if (std::abs(og - 1.0f) > 1e-4f) buffer.applyGain(og);
}

void EquilibrioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void EquilibrioProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (!xml || !xml->hasTagName(apvts.state.getType())) return;
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    cancelPendingUpdate();
    regenerate();
    sendChangeMessage();
}

juce::AudioProcessorEditor* EquilibrioProcessor::createEditor() { return new EquilibrioEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EquilibrioProcessor(); }
