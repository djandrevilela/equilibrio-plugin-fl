# Equilíbrio — project principles (read before touching the code) / princípios do projeto

VST3 effect (JUCE) that teaches mixing to EDM producers: role-based EQ, kick/bass relationship, tempo-synced sidechain, live band analysis with advice.
Sister of Harmonia, Alvorada, Zenite, Farol and Receitas.

## Non-negotiable rules (set by the author)
1. **No randomness, no AI.** Every EQ curve, sidechain setting and piece of advice comes from fixed rules/tables (same inputs => same outputs).
2. **Only things that work**: conventional, conservative mixing moves (high-pass, mud cut, presence, etc.), moderate gains, no extreme settings.
3. **No copyright issues**: no copied presets, no artist or commercial product names. Generic conventions only.
4. **Bilingual**: every user-visible string exists in pt-PT and en-GB (`L(lang, pt, en)`). Tests check both.
5. `core/` is pure C++17 with tests; `plugin/` only orchestrates (JUCE).
6. Target curves are **approximate guidelines** (say so in the UI); never present them as absolute truth.

## Structure
- `core/Equilibrio`     roles, target band profiles, EQ suggestions, biquad design, kick/bass advice, sidechain curve, advice from measurements
- `plugin/PluginProcessor` EQ + tempo-synced ducker + band analyser (effect, stereo in/out)
- `plugin/PluginEditor`    UI
