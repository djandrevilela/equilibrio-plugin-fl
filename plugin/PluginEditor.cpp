#include "PluginEditor.h"

#include "Util.h"

using namespace eqb;

namespace {
const juce::Colour kBg(0xff14161f), kPanel(0xff1d2130), kEdge(0xff2d3348), kText(0xffe2e8f0), kMuted(0xff8a94a8), kAccent(0xfff6ad55), kTeal(0xff4fd1c5);
}

void BandMeter::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(kPanel);
    g.fillRoundedRectangle(r, 8.0f);
    std::array<double, kBands> m{};
    const bool sig = proc.getBands(m);
    const auto tg = targetProfile(proc.role());
    double mean = 0;
    if (sig) { for (double v : m) mean += v; mean /= kBands; }
    const float zeroY = r.getCentreY() - 8, pxPerDb = 3.2f, w = (r.getWidth() - 24) / kBands;
    g.setColour(kEdge);
    g.drawHorizontalLine((int) zeroY, r.getX() + 8, r.getRight() - 8);
    g.setFont(juce::FontOptions(10.5f));
    static const char* sh[kBands] = {"Sub", "Bass", "LowM", "Mud", "Mid", "UpM", "Pres", "Air"};
    for (int i = 0; i < kBands; ++i) {
        const float x = r.getX() + 12 + i * w;
        if (sig) {
            const float v = (float) std::max(-38.0, std::min(38.0, m[(size_t) i] - mean));
            const double diff = (m[(size_t) i] - mean) - tg[(size_t) i];
            g.setColour(std::abs(diff) >= 4 ? juce::Colour(0xfff56565) : kTeal);
            const float y = zeroY - v * pxPerDb;
            g.fillRoundedRectangle(x + 6, std::min(y, zeroY), w - 12, std::max(2.0f, std::abs(zeroY - y)), 3.0f);
        }
        const float ty = zeroY - (float) std::max(-38.0, std::min(38.0, tg[(size_t) i])) * pxPerDb;
        g.setColour(kAccent);
        g.fillRect(x + 2, ty - 1.5f, w - 4, 3.0f);
        g.setColour(kMuted);
        g.drawText(sh[i], (int) x, (int) (r.getBottom() - 20), (int) w, 14, juce::Justification::centred);
    }
}

juce::String EquilibrioEditor::T(const char* pt, const char* en) const { return U8(L(proc.lang(), pt, en)); }

void EquilibrioEditor::styleBox(juce::TextEditor& t) {
    t.setMultiLine(true, true);
    t.setReadOnly(true);
    t.setCaretVisible(false);
    t.setColour(juce::TextEditor::backgroundColourId, kPanel);
    t.setColour(juce::TextEditor::textColourId, kText);
    t.setColour(juce::TextEditor::outlineColourId, kEdge);
    t.setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(t);
}

EquilibrioEditor::EquilibrioEditor(EquilibrioProcessor& p) : juce::AudioProcessorEditor(&p), proc(p), meter(p) {
    setLookAndFeel(&lnf);
    lnf.setColour(juce::ComboBox::backgroundColourId, kPanel);
    lnf.setColour(juce::PopupMenu::backgroundColourId, kPanel);
    lnf.setColour(juce::Slider::thumbColourId, kAccent);
    lnf.setColour(juce::Slider::trackColourId, kAccent.withAlpha(0.5f));
    lnf.setColour(juce::ToggleButton::tickColourId, kAccent);
    lnf.setColour(juce::TextButton::buttonColourId, kPanel);
    auto combo = [&](juce::ComboBox& c, const char* id) {
        if (auto* pc = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id))) c.addItemList(pc->choices, 1);
        addAndMakeVisible(c);
        cAtt.push_back(std::make_unique<CA>(proc.apvts, id, c));
    };
    auto toggle = [&](juce::ToggleButton& t, const char* id) { addAndMakeVisible(t); bAtt.push_back(std::make_unique<BA>(proc.apvts, id, t)); };
    auto slider = [&](juce::Slider& s, const char* id) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 22);
        addAndMakeVisible(s);
        sAtt.push_back(std::make_unique<SA>(proc.apvts, id, s));
    };
    combo(langBox, "lang"); combo(roleBox, "role"); combo(genreBox, "genre"); combo(patBox, "duckPat"); combo(keyBox, "key");
    for (int n = 24; n <= 43; ++n) {
        static const char* nm[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        kickBox.addItem(juce::String(nm[n % 12]) + juce::String(n / 12 - 1) + "  (" + juce::String((int) std::lround(440.0 * std::pow(2.0, (n - 69) / 12.0))) + " Hz)", n);
    }
    addAndMakeVisible(kickBox);
    kickBox.setSelectedId((int) std::lround(proc.apvts.getRawParameterValue("kickNote")->load()), juce::dontSendNotification);
    kickBox.onChange = [this] { proc.setParam("kickNote", (float) kickBox.getSelectedId()); refresh(); };
    toggle(eqTg, "eqOn"); toggle(duckTg, "duckOn");
    slider(amtS, "eqAmt"); slider(depthS, "depth"); slider(attackS, "attack"); slider(releaseS, "release"); slider(outS, "gOut");
    for (auto* l : {&roleL, &genreL, &meterL, &eqL, &adviceL, &scL, &amtL, &depthL, &attackL, &releaseL, &kickL, &keyL, &outL}) {
        l->setColour(juce::Label::textColourId, kMuted);
        l->setFont(juce::FontOptions(12.0f, juce::Font::bold));
        addAndMakeVisible(*l);
    }
    disclaimer.setColour(juce::Label::textColourId, kMuted);
    disclaimer.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(disclaimer);
    addAndMakeVisible(meter);
    addAndMakeVisible(genreBtn);
    genreBtn.onClick = [this] { proc.applyGenreDefaults(); };
    for (auto* t : {&eqBox, &adviceBox, &whyBox, &kickText}) styleBox(*t);
    langBox.onChange = [this] { juce::MessageManager::callAsync([s = juce::Component::SafePointer<EquilibrioEditor>(this)] { if (s) s->refresh(); }); };
    roleBox.onChange = [this] { refresh(); };
    genreBox.onChange = [this] { refresh(); };
    keyBox.onChange = [this] { refresh(); };
    setSize(1000, 740);
    proc.addChangeListener(this);
    refresh();
    startTimerHz(4);
}
EquilibrioEditor::~EquilibrioEditor() {
    proc.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

void EquilibrioEditor::timerCallback() {
    meter.repaint();
    std::array<double, kBands> m{};
    juce::String t;
    if (proc.getBands(m)) {
        auto adv = analyse(proc.role(), m, proc.lang());
        if (adv.empty()) t = T("Sem problemas evidentes face ao alvo orientativo deste papel. Bom trabalho!", "No obvious problems against this role's guideline target. Nice work!");
        else for (auto& a : adv) t += juce::String(juce::CharPointer_UTF8("\xE2\x80\xA2 ")) + U8(a.text) + "\n\n";
    } else t = T("Põe som a tocar (o plugin analisa o sinal que entra).", "Play some audio (the plugin analyses the incoming signal).");
    if (t != lastAdvice) { lastAdvice = t; adviceBox.setText(t, false); }
}

void EquilibrioEditor::refresh() {
    const Lang lg = proc.lang();
    for (int i = 0; i < kNumRoles; ++i) roleBox.changeItemText(i + 1, U8(roleName((Role) i, lg)));
    for (int i = 0; i < kNumDuckPatterns; ++i) patBox.changeItemText(i + 1, U8(duckPatternName((DuckPattern) i, lg)));
    roleBox.setText(U8(roleName(proc.role(), lg)), juce::dontSendNotification);
    patBox.setText(U8(duckPatternName((DuckPattern) juce::jlimit(0, 2, (int) std::lround(proc.apvts.getRawParameterValue("duckPat")->load())), lg)), juce::dontSendNotification);
    roleL.setText(T("PAPEL DESTA PISTA", "ROLE OF THIS TRACK"), juce::dontSendNotification);
    genreL.setText(T("SUBG\xC3\x89NERO", "SUBGENRE"), juce::dontSendNotification);
    meterL.setText(T("ESPECTRO: barras = o teu som, linhas laranja = alvo orientativo", "SPECTRUM: bars = your sound, orange lines = guideline target"), juce::dontSendNotification);
    eqL.setText(T("EQ SUGERIDO PARA ESTE PAPEL", "SUGGESTED EQ FOR THIS ROLE"), juce::dontSendNotification);
    adviceL.setText(T("CONSELHOS EM TEMPO REAL", "LIVE ADVICE"), juce::dontSendNotification);
    scL.setText(T("SIDECHAIN SINCRONIZADO COM O TEMPO", "TEMPO-SYNCED SIDECHAIN"), juce::dontSendNotification);
    amtL.setText(T("Quantidade", "Amount"), juce::dontSendNotification);
    depthL.setText(T("Profundidade", "Depth"), juce::dontSendNotification);
    attackL.setText("Attack ms", juce::dontSendNotification);
    releaseL.setText(T("Release (tempos)", "Release (beats)"), juce::dontSendNotification);
    kickL.setText(T("NOTA DO BOMBO", "KICK NOTE"), juce::dontSendNotification);
    keyL.setText(T("TOM DA M\xC3\x9ASICA", "SONG KEY"), juce::dontSendNotification);
    outL.setText(T("Sa\xC3\xADda (dB)", "Output (dB)"), juce::dontSendNotification);
    eqTg.setButtonText(T("EQ ligado", "EQ on"));
    duckTg.setButtonText(T("Sidechain ligado (precisa do FL a tocar)", "Sidechain on (needs the host playing)"));
    genreBtn.setButtonText(T("Usar valores do subg\xC3\xA9nero", "Use genre defaults"));
    disclaimer.setText(T("Os alvos s\xC3\xA3o orientativos, n\xC3\xA3o regras absolutas: confia sempre nos teus ouvidos.", "Targets are guidelines, not absolute rules: always trust your ears."), juce::dontSendNotification);
    // EQ
    juce::String e;
    for (auto& f : suggestedEq(proc.role(), lg)) {
        juce::String type = f.type == FilterType::HighPass ? T("Corte graves", "High-pass") : f.type == FilterType::LowPass ? T("Corte agudos", "Low-pass") : f.type == FilterType::Bell ? "Bell" : f.type == FilterType::LowShelf ? "Low shelf" : "High shelf";
        e += type + "  " + juce::String((int) std::lround(f.freq)) + " Hz" + (f.gainDb != 0 ? (f.gainDb > 0 ? "  +" : "  ") + juce::String(f.gainDb, 1) + " dB" : juce::String()) + "\n   " + U8(f.why) + "\n\n";
    }
    eqBox.setText(e, false);
    // sidechain
    whyBox.setText(U8(sidechainWhy(proc.genre(), lg)) + "\n\n" + T("Recomendado para este papel: ", "Recommended for this role: ") + (duckRecommended(proc.role()) ? T("sim", "yes") : T("n\xC3\xA3o (normalmente \xC3\xA9 o bombo que manda o ritmo)", "no (usually the kick drives the rhythm)")), false);
    // bombo/baixo
    auto kb = kickBass((int) std::lround(proc.apvts.getRawParameterValue("kickNote")->load()), juce::jlimit(0, 11, (int) std::lround(proc.apvts.getRawParameterValue("key")->load())), lg);
    kickText.setText(U8(kb.text), false);
    kickBox.setSelectedId((int) std::lround(proc.apvts.getRawParameterValue("kickNote")->load()), juce::dontSendNotification);
    lastAdvice = "";
    repaint();
}

void EquilibrioEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    g.setColour(kAccent);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("EQUILIBRIO", 12, 8, 140, 24, juce::Justification::left);
}

void EquilibrioEditor::resized() {
    langBox.setBounds(860, 10, 128, 26);
    roleL.setBounds(170, 4, 200, 14); roleBox.setBounds(170, 18, 200, 26);
    genreL.setBounds(384, 4, 200, 14); genreBox.setBounds(384, 18, 180, 26);
    meterL.setBounds(12, 50, 480, 16); meter.setBounds(12, 68, 480, 232);
    eqL.setBounds(504, 50, 484, 16);
    eqTg.setBounds(504, 68, 120, 24); amtL.setBounds(630, 68, 80, 24); amtS.setBounds(710, 68, 278, 24);
    eqBox.setBounds(504, 98, 484, 202);
    adviceL.setBounds(12, 310, 480, 16); adviceBox.setBounds(12, 328, 480, 196);
    scL.setBounds(504, 310, 484, 16);
    duckTg.setBounds(504, 328, 300, 24); patBox.setBounds(808, 328, 180, 24);
    depthL.setBounds(504, 356, 100, 22); depthS.setBounds(606, 356, 200, 22);
    attackL.setBounds(504, 380, 100, 22); attackS.setBounds(606, 380, 200, 22);
    releaseL.setBounds(504, 404, 100, 22); releaseS.setBounds(606, 404, 200, 22);
    genreBtn.setBounds(816, 356, 172, 26);
    whyBox.setBounds(504, 432, 484, 92);
    kickL.setBounds(12, 534, 140, 16); kickBox.setBounds(12, 552, 150, 26);
    keyL.setBounds(176, 534, 140, 16); keyBox.setBounds(176, 552, 80, 26);
    outL.setBounds(280, 534, 120, 16); outS.setBounds(280, 552, 208, 26);
    kickText.setBounds(12, 586, 976, 120);
    disclaimer.setBounds(12, 714, 976, 16);
}
