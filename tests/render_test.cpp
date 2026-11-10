// Teste de áudio offline: EQ estável, sem NaN, o analisador responde a ruído/tons e o EQ do papel altera o sinal.
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <cstdio>

#include "PluginProcessor.h"

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    EquilibrioProcessor proc;
    const double sr = 44100.0;
    const int bs = 512;
    proc.setPlayConfigDetails(2, 2, sr, bs);
    proc.prepareToPlay(sr, bs);
    int fails = 0;
    auto run = [&](double freq, int blocks, double& outPeak, double& outRms, bool& bad) {
        juce::AudioBuffer<float> buf(2, bs);
        double ph = 0, ss = 0, ssIn = 0;
        long cnt = 0;
        outPeak = 0;
        for (int b = 0; b < blocks; ++b) {
            for (int i = 0; i < bs; ++i) {
                float x = (float) (0.3 * std::sin(ph));
                ph += 2 * 3.14159265358979 * freq / sr;
                buf.setSample(0, i, x); buf.setSample(1, i, x);
                if (b > blocks / 2) ssIn += x * x;
            }
            proc.processForTest(buf);
            if (b > blocks / 2)
                for (int i = 0; i < bs; ++i) {
                    float y = buf.getSample(0, i);
                    if (!std::isfinite(y)) bad = true;
                    outPeak = std::max(outPeak, (double) std::abs(y));
                    ss += y * y; ++cnt;
                }
        }
        outRms = std::sqrt(ss / (double) cnt) / std::max(1e-9, std::sqrt(ssIn / (double) cnt));  // ganho aparente
    };
    for (int role = 0; role < eqb::kNumRoles; ++role) {
        proc.apvts.getParameter("role")->setValueNotifyingHost(proc.apvts.getParameter("role")->convertTo0to1((float) role));
        proc.regenerateForTest();
        double pk, g50, g1k, g10k; bool bad = false;
        run(50, 200, pk, g50, bad); run(1000, 200, pk, g1k, bad); run(10000, 200, pk, g10k, bad);
        std::printf("%-12s ganho 50 Hz %+.1f dB | 1 kHz %+.1f dB | 10 kHz %+.1f dB %s\n", eqb::roleName((eqb::Role) role, eqb::Lang::EN).c_str(),
                    20 * std::log10(g50), 20 * std::log10(g1k), 20 * std::log10(g10k), bad ? "<-- NaN" : "");
        if (bad || pk > 1.0) ++fails;
        if (role == (int) eqb::Role::Lead && g50 > 0.3) ++fails;          // lead: corta graves
        if (role == (int) eqb::Role::Kick && g50 < 1.05) ++fails;         // bombo: reforça 60 Hz
    }
    // analisador: tom de 1 kHz deve dominar a banda "Médios" (500-2k) depois de normalizar
    {
        double a, b2; bool bad = false;
        run(1000, 400, a, b2, bad);
        std::array<double, eqb::kBands> m{};
        bool sig = proc.getBands(m);
        int best = 0;
        for (int i = 1; i < eqb::kBands; ++i) if (m[(size_t) i] > m[(size_t) best]) best = i;
        std::printf("analisador: sinal=%d banda dominante=%d (esperado 4)\n", (int) sig, best);
        if (!sig || best != 4) ++fails;
    }
    std::printf(fails == 0 ? "Tudo OK.\n" : "%d problema(s).\n", fails);
    return fails == 0 ? 0 : 1;
}
