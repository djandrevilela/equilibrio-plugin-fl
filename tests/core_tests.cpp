#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Equilibrio.h"

using namespace eqb;
static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FALHOU: %s (linha %d)\n", #c, __LINE__); } } while (0)

int main() {
    const double sr = 48000;
    for (int r = 0; r < kNumRoles; ++r)
        for (int lg = 0; lg < 2; ++lg) {
            Lang l = lg ? Lang::EN : Lang::PT;
            auto f = suggestedEq((Role) r, l);
            CHECK(f.size() >= 3 && f.size() <= 4);
            CHECK(!roleName((Role) r, l).empty());
            for (auto& x : f) {
                CHECK(!x.why.empty() && x.freq >= 20 && x.freq <= 20000 && std::abs(x.gainDb) <= 3.01 && x.q > 0.3 && x.q < 2.0);
                auto b = designFilter(x.type, x.freq, x.gainDb, x.q, sr);
                // estabilidade: pólos dentro do círculo unitário
                CHECK(std::abs(b.a2) < 1.0 && std::abs(b.a1) < 1.0 + b.a2);
                // resposta na frequência central
                const double m = magnitudeDb(b, x.freq, sr);
                if (x.type == FilterType::Bell) CHECK(std::abs(m - x.gainDb) < 0.05);
                if (x.type == FilterType::HighPass) CHECK(magnitudeDb(b, x.freq * 8, sr) > -0.2 && magnitudeDb(b, x.freq / 8, sr) < -30);
                if (x.type == FilterType::LowPass) CHECK(magnitudeDb(b, x.freq / 8, sr) > -0.2 && magnitudeDb(b, x.freq * 8, sr) < -30);
                if (x.type == FilterType::HighShelf) CHECK(std::abs(magnitudeDb(b, 19000, sr) - x.gainDb) < 0.6);
            }
            // o perfil-alvo é centrado
            auto t = targetProfile((Role) r);
            double s = 0; for (double v : t) s += v;
            CHECK(std::abs(s) < 1e-9);
        }
    // perfis diferentes por papel e com o sentido esperado
    CHECK(targetProfile(Role::Kick)[0] > targetProfile(Role::Lead)[0] + 20);
    CHECK(targetProfile(Role::Percussion)[7] > targetProfile(Role::Bass)[7] + 20);
    // sidechain
    for (int g = 0; g < kNumGenres; ++g) {
        auto s = sidechainFor((Genre) g);
        CHECK(s.depth > 0.3 && s.depth < 0.9 && s.releaseBeats > 0.2 && s.releaseBeats < 1.0);
        CHECK(!sidechainWhy((Genre) g, Lang::PT).empty() && sidechainWhy((Genre) g, Lang::PT) != sidechainWhy((Genre) g, Lang::EN));
        // o release acaba antes do disparo seguinte
        auto tr = duckTriggers(patternFor((Genre) g));
        double gap = 99;
        for (size_t i = 0; i < tr.size(); ++i) gap = std::min(gap, (i + 1 < tr.size() ? tr[i + 1] : tr[0] + 4.0) - tr[i]);
        CHECK(s.releaseBeats <= gap);
    }
    {
        Sidechain s{0.7, 3, 0.5};
        CHECK(duckGain(-1, s, 128) == 1.0);
        CHECK(std::abs(duckGain(0, s, 128) - 1.0) < 1e-9);
        const double atkBeats = 3 * 0.001 * 128 / 60;
        CHECK(std::abs(duckGain(atkBeats, s, 128) - 0.3) < 1e-9);  // máx. redução
        CHECK(duckGain(0.25, s, 128) > 0.3 && duckGain(0.25, s, 128) < 1.0);
        CHECK(std::abs(duckGain(0.6, s, 128) - 1.0) < 1e-9);
        double prev = 0;  // recuperação monótona
        for (double t = atkBeats; t < 0.6; t += 0.01) { double g = duckGain(t, s, 128); CHECK(g >= prev - 1e-9); prev = g; }
        for (int p = 0; p < kNumDuckPatterns; ++p)
            for (double b = 0; b < 16; b += 0.05) { double g = duckGainAt(b, (DuckPattern) p, s, 128); CHECK(g >= 0.3 - 1e-9 && g <= 1.0 + 1e-9); }
        CHECK(duckGainAt(4.0, DuckPattern::EveryBeat, s, 128) == duckGainAt(0.0, DuckPattern::EveryBeat, s, 128));
        CHECK(duckGainAt(2.5 + atkBeats, DuckPattern::FutureBass, s, 128) < 0.31);
    }
    // bombo / baixo
    {
        auto a = kickBass(36, 0, Lang::EN);  // C2 em tom de Dó
        CHECK(a.suggestedKickNote == 36 && a.text.find("already tuned") != std::string::npos);
        CHECK(std::abs(a.kickHz - 65.41) < 0.05);
        a = kickBass(37, 0, Lang::PT);  // Dó# -> Dó
        CHECK(a.suggestedKickNote == 36 && a.text.find("afina-o para C2") != std::string::npos);
        a = kickBass(34, 9, Lang::EN);  // A#1 em Lá: A1(33) é o mais perto
        CHECK(a.suggestedKickNote == 33);
        for (int n = 24; n <= 43; ++n) for (int k = 0; k < 12; ++k) { auto x = kickBass(n, k, Lang::PT); CHECK(x.crossoverHz >= 70 && x.crossoverHz <= 130 && ((x.suggestedKickNote - k) % 12 == 0 || ((x.suggestedKickNote - k) % 12 + 12) % 12 == 7)); }
    }
    // análise: sinal exatamente igual ao alvo -> sem conselhos; excesso numa banda -> conselho nessa banda
    for (int r = 0; r < kNumRoles; ++r) {
        auto t = targetProfile((Role) r);
        std::array<double, kBands> m;
        for (int i = 0; i < kBands; ++i) m[(size_t) i] = t[(size_t) i] - 30;  // nível global diferente não conta
        CHECK(analyse((Role) r, m, Lang::EN).empty());
        m[3] += 8;  // lama
        auto a = analyse((Role) r, m, Lang::PT);
        bool has = false;
        for (auto& x : a) has |= x.band == 3 && x.diffDb > 4;
        CHECK(has);
        for (auto& x : a) CHECK(!x.text.empty());
        m[3] -= 16;  // falta
        a = analyse((Role) r, m, Lang::EN);
        has = false;
        for (auto& x : a) has |= x.band == 3 && x.diffDb < -4;
        CHECK(has);
        for (auto& v : m) v = -200;
        CHECK(analyse((Role) r, m, Lang::PT).empty());  // silêncio
    }
    std::printf("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
