#include "Equilibrio.h"

#include <algorithm>
#include <cmath>

namespace eqb {

std::string L(Lang l, const char* pt, const char* en) { return l == Lang::PT ? pt : en; }

static const double kEdges[kBands + 1] = {20, 60, 120, 250, 500, 2000, 4000, 8000, 20000};
double bandLow(int b) { return kEdges[b]; }
double bandHigh(int b) { return kEdges[b + 1]; }
double bandCentre(int b) { return std::sqrt(kEdges[b] * kEdges[b + 1]); }

std::string bandName(int b, Lang l) {
    static const char* pt[kBands] = {"Sub", "Graves", "Médios-graves", "Lama", "Médios", "Médios-agudos", "Presença", "Ar"};
    static const char* en[kBands] = {"Sub", "Bass", "Low-mids", "Mud", "Mids", "Upper-mids", "Presence", "Air"};
    std::string r = l == Lang::PT ? pt[b] : en[b];
    auto f = [](double v) { return v >= 1000 ? std::to_string((int) std::lround(v / 1000)) + "k" : std::to_string((int) std::lround(v)); };
    return r + " (" + f(kEdges[b]) + "-" + f(kEdges[b + 1]) + " Hz)";
}

std::string roleName(Role r, Lang l) {
    switch (r) {
        case Role::Kick: return L(l, "Bombo", "Kick");
        case Role::Bass: return L(l, "Baixo / sub", "Bass / sub");
        case Role::Lead: return "Lead";
        case Role::Pad: return L(l, "Pad / acordes", "Pad / chords");
        case Role::Vocal: return L(l, "Vocal / chop", "Vocal / chop");
        case Role::Percussion: return L(l, "Hats / percussão", "Hats / percussion");
        case Role::Fx: return L(l, "FX / riser", "FX / riser");
        case Role::FullMix: return L(l, "Mistura completa", "Full mix");
    }
    return "";
}

std::string genreName(Genre g) {
    switch (g) {
        case Genre::FutureBass: return "Future Bass";
        case Genre::ProgHouse: return "Progressive House";
        case Genre::BigRoom: return "Big Room";
    }
    return "";
}

std::array<double, kBands> targetProfile(Role r) {
    // base: espectro "rosa" (-3 dB/oitava) medido em densidade por banda, centrado; depois desvio por papel
    std::array<double, kBands> t{};
    for (int i = 0; i < kBands; ++i) t[(size_t) i] = -3.0 * std::log2(bandCentre(i) / bandCentre(0));
    static const double off[kNumRoles][kBands] = {
        {10, 10, 0, -12, -18, -16, -20, -24},    // Kick
        {6, 10, 4, -4, -14, -20, -26, -30},      // Bass
        {-30, -22, -8, 0, 5, 5, 1, -2},          // Lead
        {-30, -18, -4, 2, 5, 3, -2, -6},         // Pad
        {-30, -22, -6, 0, 6, 5, 1, -3},          // Vocal
        {-30, -30, -24, -14, -4, 2, 8, 8},       // Percussion
        {-18, -12, -8, -4, 0, 2, 2, 2},          // Fx
        {1, 1, 0, -1, 0, 0, 0, -1},              // FullMix
    };
    double mean = 0;
    for (int i = 0; i < kBands; ++i) { t[(size_t) i] += off[(int) r][i]; mean += t[(size_t) i]; }
    mean /= kBands;
    for (auto& v : t) v -= mean;
    return t;
}

// ------------------------------------------------------------------ EQ
std::vector<EqFilter> suggestedEq(Role r, Lang l) {
    using F = FilterType;
    auto mk = [&](F t, double f, double g, double q, const char* pt, const char* en) { return EqFilter{t, f, g, q, L(l, pt, en)}; };
    switch (r) {
        case Role::Kick:
            return {mk(F::HighPass, 25, 0, 0.707, "Corta o infragrave inaudível que só gasta headroom.", "Cuts inaudible infra-bass that only eats headroom."),
                    mk(F::Bell, 60, 3, 1.0, "Reforça o peso do fundamental do bombo.", "Reinforces the weight of the kick fundamental."),
                    mk(F::Bell, 300, -3, 1.2, "Tira a 'caixa' de madeira e a lama que o bombo acumula.", "Removes the boxy 'wood' and the mud a kick collects."),
                    mk(F::Bell, 3500, 2, 1.0, "Dá o clique que se ouve em colunas pequenas.", "Adds the click that is audible on small speakers.")};
        case Role::Bass:
            return {mk(F::HighPass, 30, 0, 0.707, "Limpa o infragrave e deixa espaço ao bombo.", "Cleans the infra-bass and leaves room for the kick."),
                    mk(F::Bell, 250, -2, 1.0, "Reduz a lama nos médios-graves.", "Reduces the mud in the low-mids."),
                    mk(F::Bell, 800, 2, 1.0, "Realça harmónicos para o baixo se ouvir em colunas pequenas.", "Lifts harmonics so the bass is audible on small speakers."),
                    mk(F::LowPass, 9000, 0, 0.707, "Tira o brilho inútil: o baixo não precisa dele e liberta espaço.", "Removes useless brightness: the bass does not need it, and it frees space.")};
        case Role::Lead:
            return {mk(F::HighPass, 150, 0, 0.707, "Os graves do lead competem com o baixo: corta-os.", "A lead's lows compete with the bass: cut them."),
                    mk(F::Bell, 300, -3, 1.0, "Tira a lama que turva a melodia.", "Removes the mud that clouds the melody."),
                    mk(F::Bell, 3000, 2, 1.0, "Dá presença para a melodia passar a frente.", "Adds presence so the melody cuts through."),
                    mk(F::HighShelf, 10000, 1.5, 0.707, "Um pouco de ar para abrir o som.", "A little air to open up the sound.")};
        case Role::Pad:
            return {mk(F::HighPass, 200, 0, 0.707, "Um pad não precisa de graves: deixa-os ao baixo.", "A pad does not need lows: leave them to the bass."),
                    mk(F::Bell, 400, -3, 0.9, "Reduz a lama que os acordes acumulam.", "Reduces the mud that chords collect."),
                    mk(F::Bell, 3000, -2, 1.0, "Abre espaço para o lead e o vocal.", "Opens space for the lead and vocal."),
                    mk(F::HighShelf, 9000, -2, 0.707, "Suaviza o topo para o pad ficar atrás na mistura.", "Softens the top so the pad sits behind in the mix.")};
        case Role::Vocal:
            return {mk(F::HighPass, 100, 0, 0.707, "Remove ruído grave e vibrações.", "Removes low rumble and vibration."),
                    mk(F::Bell, 250, -3, 1.0, "Tira o som 'de caixa' da voz.", "Removes the 'boxy' sound of the voice."),
                    mk(F::Bell, 3500, 3, 1.0, "Dá inteligibilidade e presença.", "Adds intelligibility and presence."),
                    mk(F::HighShelf, 10000, 2, 0.707, "Ar e brilho para a voz respirar.", "Air and sparkle so the voice can breathe.")};
        case Role::Percussion:
            return {mk(F::HighPass, 300, 0, 0.707, "Hats e percussão não têm informação abaixo disto: liberta os graves.", "Hats and percussion carry nothing below this: free the lows."),
                    mk(F::Bell, 1000, -2, 1.0, "Reduz o 'plástico' dos médios.", "Reduces the 'plastic' mids."),
                    mk(F::HighShelf, 9000, 1.5, 0.707, "Brilho para os hats cortarem.", "Brightness so the hats cut through.")};
        case Role::Fx:
            return {mk(F::HighPass, 250, 0, 0.707, "Risers e FX com graves enchem a mistura.", "Risers and FX with lows clog the mix."),
                    mk(F::Bell, 2500, -2, 0.9, "Deixa a zona dos vocais livre.", "Keeps the vocal zone clear."),
                    mk(F::HighShelf, 8000, 1, 0.707, "Um pouco de ar para o riser abrir.", "A touch of air for the riser to open.")};
        case Role::FullMix:
            return {mk(F::HighPass, 25, 0, 0.707, "Limpa o infragrave de toda a mistura.", "Cleans infra-bass from the whole mix."),
                    mk(F::Bell, 300, -1.5, 0.8, "Tira um pouco de lama global.", "Removes a little global mud."),
                    mk(F::HighShelf, 10000, 1, 0.707, "Um toque de ar no topo.", "A touch of air on top.")};
    }
    return {};
}

Biquad designFilter(FilterType t, double f, double gDb, double q, double sr) {
    f = std::max(10.0, std::min(f, sr * 0.45));
    q = std::max(0.1, q);
    const double w0 = 2.0 * 3.141592653589793 * f / sr, c = std::cos(w0), s = std::sin(w0), alpha = s / (2.0 * q);
    const double A = std::pow(10.0, gDb / 40.0);
    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
    switch (t) {
        case FilterType::HighPass: b0 = (1 + c) / 2; b1 = -(1 + c); b2 = (1 + c) / 2; a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha; break;
        case FilterType::LowPass: b0 = (1 - c) / 2; b1 = 1 - c; b2 = (1 - c) / 2; a0 = 1 + alpha; a1 = -2 * c; a2 = 1 - alpha; break;
        case FilterType::Bell:
            b0 = 1 + alpha * A; b1 = -2 * c; b2 = 1 - alpha * A; a0 = 1 + alpha / A; a1 = -2 * c; a2 = 1 - alpha / A; break;
        case FilterType::LowShelf: {
            const double k = 2 * std::sqrt(A) * alpha;
            b0 = A * ((A + 1) - (A - 1) * c + k); b1 = 2 * A * ((A - 1) - (A + 1) * c); b2 = A * ((A + 1) - (A - 1) * c - k);
            a0 = (A + 1) + (A - 1) * c + k; a1 = -2 * ((A - 1) + (A + 1) * c); a2 = (A + 1) + (A - 1) * c - k; break;
        }
        case FilterType::HighShelf: {
            const double k = 2 * std::sqrt(A) * alpha;
            b0 = A * ((A + 1) + (A - 1) * c + k); b1 = -2 * A * ((A - 1) + (A + 1) * c); b2 = A * ((A + 1) + (A - 1) * c - k);
            a0 = (A + 1) - (A - 1) * c + k; a1 = 2 * ((A - 1) - (A + 1) * c); a2 = (A + 1) - (A - 1) * c - k; break;
        }
    }
    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

Biquad designBandpass(double f, double q, double sr) {
    f = std::max(10.0, std::min(f, sr * 0.45));
    const double w0 = 2.0 * 3.141592653589793 * f / sr, c = std::cos(w0), alpha = std::sin(w0) / (2.0 * q);
    const double a0 = 1 + alpha;
    return {alpha / a0, 0, -alpha / a0, -2 * c / a0, (1 - alpha) / a0};
}

double magnitudeDb(const Biquad& b, double f, double sr) {
    const double w = 2.0 * 3.141592653589793 * f / sr;
    const double c1 = std::cos(w), s1 = std::sin(w), c2 = std::cos(2 * w), s2 = std::sin(2 * w);
    const double nr = b.b0 + b.b1 * c1 + b.b2 * c2, ni = -(b.b1 * s1 + b.b2 * s2);
    const double dr = 1 + b.a1 * c1 + b.a2 * c2, di = -(b.a1 * s1 + b.a2 * s2);
    return 10.0 * std::log10((nr * nr + ni * ni) / (dr * dr + di * di));
}

// ------------------------------------------------------------------ sidechain
std::string duckPatternName(DuckPattern p, Lang l) {
    switch (p) {
        case DuckPattern::EveryBeat: return L(l, "Cada tempo (bombo a 4)", "Every beat (four-on-floor)");
        case DuckPattern::HalfTime: return L(l, "Meio tempo (tempos 1 e 3)", "Half time (beats 1 and 3)");
        case DuckPattern::FutureBass: return L(l, "Future Bass (1 e 'e' do 3)", "Future Bass (1 and the 'and' of 3)");
    }
    return "";
}
std::vector<double> duckTriggers(DuckPattern p) {
    switch (p) {
        case DuckPattern::EveryBeat: return {0, 1, 2, 3};
        case DuckPattern::HalfTime: return {0, 2};
        case DuckPattern::FutureBass: return {0, 2.5};
    }
    return {0};
}
Sidechain sidechainFor(Genre g) {
    switch (g) {
        case Genre::FutureBass: return {0.75, 3.0, 0.60};
        case Genre::ProgHouse: return {0.55, 3.0, 0.40};
        case Genre::BigRoom: return {0.70, 3.0, 0.45};
    }
    return {};
}
DuckPattern patternFor(Genre g) { return g == Genre::FutureBass ? DuckPattern::FutureBass : DuckPattern::EveryBeat; }
bool duckRecommended(Role r) { return r == Role::Bass || r == Role::Lead || r == Role::Pad || r == Role::Fx; }

std::string sidechainWhy(Genre g, Lang l) {
    std::string s = L(l, "O sidechain baixa o volume do som a cada bombo para os dois não se taparem. O release deve acabar um pouco antes do bombo seguinte, para o som voltar a tempo. ",
                      "Sidechain lowers the sound at every kick so the two do not mask each other. The release should end just before the next kick so the sound is back in time. ");
    switch (g) {
        case Genre::FutureBass: s += L(l, "No future bass o bombo é esparso (meio tempo), por isso o release é mais longo e a profundidade maior: o 'bombeamento' faz parte do estilo.", "In future bass the kick is sparse (half time), so the release is longer and the depth greater: the 'pumping' is part of the style."); break;
        case Genre::ProgHouse: s += L(l, "Em progressive house o efeito é subtil e rápido, para manter a fluidez.", "In progressive house the effect is subtle and quick, to keep the flow."); break;
        case Genre::BigRoom: s += L(l, "Em big room é bem marcado, para o bombo bater forte.", "In big room it is well marked, so the kick hits hard."); break;
    }
    return s;
}

double duckGain(double t, const Sidechain& s, double bpm) {
    if (t < 0) return 1.0;
    const double attackBeats = s.attackMs * 0.001 * bpm / 60.0;
    double d;
    if (t < attackBeats) d = s.depth * (attackBeats > 0 ? t / attackBeats : 1.0);
    else {
        const double x = (t - attackBeats) / std::max(1e-6, s.releaseBeats);
        d = x >= 1.0 ? 0.0 : s.depth * (1.0 - x) * (1.0 - x);
    }
    return 1.0 - d;
}

double duckGainAt(double beat, DuckPattern p, const Sidechain& s, double bpm) {
    const auto tr = duckTriggers(p);
    const double bar = std::floor(beat / 4.0), pos = beat - bar * 4.0;
    double last = tr.back() - 4.0;  // último do compasso anterior
    for (double t : tr) if (t <= pos + 1e-12) last = t;
    return duckGain(pos - last, s, bpm);
}

// ------------------------------------------------------------------ bombo / baixo
KickBassAdvice kickBass(int note, int root, Lang l) {
    KickBassAdvice a;
    a.kickHz = 440.0 * std::pow(2.0, (note - 69) / 12.0);
    a.crossoverHz = std::max(70.0, std::min(130.0, a.kickHz * 1.6));
    int best = 36, bd = 99;
    for (int n = 24; n <= 43; ++n) {
        const int pc = ((n - root) % 12 + 12) % 12;
        if (pc == 0 || pc == 7) {
            const int d = std::abs(n - note);
            if (d < bd) { bd = d; best = n; }
        }
    }
    a.suggestedKickNote = best;
    static const char* nm[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    auto name = [&](int n) { return std::string(nm[((n % 12) + 12) % 12]) + std::to_string(n / 12 - 1); };
    std::string s = L(l, "O fundamental do bombo está a cerca de ", "The kick fundamental sits at about ") + std::to_string((int) std::lround(a.kickHz)) + " Hz (" + name(note) + "). ";
    s += L(l, "Divide o espaço: o bombo manda abaixo de ~", "Split the space: the kick rules below ~") + std::to_string((int) std::lround(a.crossoverHz)) + L(l, " Hz e o baixo acima. Faz um corte de ~3 dB no baixo à volta de ", " Hz and the bass above. Cut ~3 dB from the bass around ") +
         std::to_string((int) std::lround(a.kickHz)) + L(l, " Hz (ou usa sidechain). ", " Hz (or use sidechain). ");
    if (best == note) s += L(l, "O bombo já está afinado para a tónica ou a quinta do tom: ótimo, não vai destoar do baixo.", "The kick is already tuned to the key's tonic or fifth: great, it will not clash with the bass.");
    else s += L(l, "Para o bombo não destoar do tom, afina-o para ", "So the kick does not clash with the key, tune it to ") + name(best) + " (" + (best > note ? "+" : "") + std::to_string(best - note) + L(l, " semitons).", " semitones).");
    a.text = s;
    return a;
}

// ------------------------------------------------------------------ conselhos
std::vector<Advice> analyse(Role r, const std::array<double, kBands>& m, Lang l) {
    std::vector<Advice> out;
    double mean = 0;
    int n = 0;
    for (double v : m) if (v > -150) { mean += v; ++n; }
    if (n < kBands) return out;  // sem sinal
    mean /= n;
    const auto tg = targetProfile(r);
    static const char* exPt[kBands] = {"demasiado sub: gasta headroom e compete com o bombo", "graves a mais: soa pesado e abafa o bombo", "acumulação nos médios-graves: soa encaixotado",
                                       "lama: o som fica turvo e sem definição", "médios a mais: soa nasal e cansa", "agudos agressivos: soa áspero", "aspereza e sibilância na presença", "demasiado ar: soa chiado"};
    static const char* exEn[kBands] = {"too much sub: it eats headroom and fights the kick", "too much bass: it sounds heavy and smothers the kick", "low-mid build-up: it sounds boxy",
                                       "mud: the sound gets cloudy and undefined", "too many mids: it sounds nasal and tiring", "harsh upper-mids: it sounds abrasive", "harshness and sibilance in the presence range", "too much air: it sounds hissy"};
    static const char* dePt[kBands] = {"falta sub: perde peso em sistemas grandes", "faltam graves: soa fino", "falta corpo", "vazio nos 250-500 Hz: pode soar oco", "faltam médios: perde-se na mistura", "falta definição e ataque", "falta brilho", "falta ar: soa abafado"};
    static const char* deEn[kBands] = {"not enough sub: it loses weight on big systems", "not enough bass: it sounds thin", "lacks body", "a hole at 250-500 Hz: it can sound hollow", "lacks mids: it gets lost in the mix", "lacks definition and attack", "lacks brightness", "lacks air: it sounds dull"};
    for (int i = 0; i < kBands; ++i) {
        const double diff = (m[(size_t) i] - mean) - tg[(size_t) i];
        if (std::abs(diff) < 4.0) continue;
        std::string t = bandName(i, l) + ": " + (diff > 0 ? "+" : "") + std::to_string((int) std::lround(diff)) + L(l, " dB face ao alvo (", " dB vs target (") + roleName(r, l) + "). " + (diff > 0 ? (l == Lang::PT ? exPt[i] : exEn[i]) : (l == Lang::PT ? dePt[i] : deEn[i])) + ". ";
        if (diff > 0) t += L(l, "Tenta um corte largo de ", "Try a wide cut of ") + std::to_string((int) std::min(6.0, std::round(0.7 * diff))) + L(l, " dB (Q ~1) perto de ", " dB (Q ~1) near ") + std::to_string((int) std::lround(bandCentre(i))) + " Hz.";
        else t += L(l, "Tenta um reforço suave de ", "Try a gentle boost of ") + std::to_string((int) std::min(4.0, std::round(0.6 * -diff))) + L(l, " dB perto de ", " dB near ") + std::to_string((int) std::lround(bandCentre(i))) + " Hz.";
        out.push_back({i, diff, t});
    }
    return out;
}

}  // namespace eqb
