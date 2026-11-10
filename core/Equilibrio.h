#pragma once
// Equilíbrio core: papéis de pista, perfis-alvo, sugestões de EQ, bombo/baixo, sidechain e conselhos. C++17 puro, determinístico, bilingue.
#include <array>
#include <string>
#include <vector>

namespace eqb {

enum class Lang { PT, EN };
std::string L(Lang l, const char* pt, const char* en);

enum class Role { Kick, Bass, Lead, Pad, Vocal, Percussion, Fx, FullMix };
constexpr int kNumRoles = 8;
enum class Genre { FutureBass, ProgHouse, BigRoom };
constexpr int kNumGenres = 3;
constexpr int kBands = 8;

std::string roleName(Role, Lang);
std::string genreName(Genre);
std::string bandName(int band, Lang);
double bandLow(int band);
double bandHigh(int band);
double bandCentre(int band);  // média geométrica

// Perfil-alvo orientativo (dB relativos, centrado em média zero), por papel
std::array<double, kBands> targetProfile(Role);

// ---- EQ ----
enum class FilterType { HighPass, LowPass, Bell, LowShelf, HighShelf };
struct EqFilter {
    FilterType type = FilterType::Bell;
    double freq = 1000, gainDb = 0, q = 1.0;
    std::string why;  // frase do porquê
};
std::vector<EqFilter> suggestedEq(Role, Lang);  // no máximo 4 filtros

struct Biquad { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };  // normalizado (a0 = 1)
Biquad designFilter(FilterType, double freq, double gainDb, double q, double sampleRate);
Biquad designBandpass(double freq, double q, double sampleRate);
double magnitudeDb(const Biquad&, double freq, double sampleRate);

// ---- Sidechain ----
struct Sidechain { double depth = 0.7, attackMs = 3.0, releaseBeats = 0.45; };
enum class DuckPattern { EveryBeat, HalfTime, FutureBass };
constexpr int kNumDuckPatterns = 3;
std::string duckPatternName(DuckPattern, Lang);
std::vector<double> duckTriggers(DuckPattern);  // tempos dentro do compasso (0..4)
Sidechain sidechainFor(Genre);
std::string sidechainWhy(Genre, Lang);
DuckPattern patternFor(Genre);
bool duckRecommended(Role);
double duckGain(double beatsSinceTrigger, const Sidechain&, double bpm);   // 0..1
double duckGainAt(double beat, DuckPattern, const Sidechain&, double bpm);  // gain a uma posição absoluta (em tempos)

// ---- Bombo / baixo ----
struct KickBassAdvice {
    double kickHz = 50, crossoverHz = 80;
    int suggestedKickNote = 36;
    std::string text;
};
KickBassAdvice kickBass(int kickMidiNote, int keyRoot, Lang);

// ---- Medição -> conselho ----
struct Advice { int band = 0; double diffDb = 0; std::string text; };
std::vector<Advice> analyse(Role, const std::array<double, kBands>& measuredDb, Lang);  // dB por banda, já normalizados pela largura

}  // namespace eqb
