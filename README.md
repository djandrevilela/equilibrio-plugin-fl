# Equilíbrio (VST3 FX) — aprende a misturar enquanto mistura · learn to mix while you mix

**PT-PT** e **EN-GB** na mesma interface. Plugin de efeito (põe-no numa pista ou no master). Tudo é **determinístico** (sem aleatoriedade, sem IA):
tabelas e regras de mistura convencionais e conservadoras. Sem presets copiados nem nomes de produtos.
A pt-PT and en-GB effect plugin. Everything is deterministic: conventional, conservative mixing rules; no copied presets or product names.

## O que faz · What it does
- **Papel da pista** (bombo, baixo/sub, lead, pad, vocal, hats/percussão, FX/riser, mistura completa): aplica um **EQ sugerido** de 3-4 filtros (corte de graves, lama, presença, ar)
  com a **explicação de cada filtro**. Botão de liga/desliga e **quantidade** (0-100 %) para o aplicares aos poucos.
  Role-based suggested EQ (3-4 filters) with a reason for each, an on/off switch and an amount control.
- **Espectro em tempo real**: 8 bandas (sub, graves, médios-graves, lama, médios, médios-agudos, presença, ar) do som que entra, com o **alvo orientativo** do papel
  em linhas laranja. Bandas a 4 dB ou mais do alvo ficam a vermelho e geram **conselhos** (ex.: "Lama: +6 dB face ao alvo. Tenta um corte largo de 4 dB perto de 350 Hz").
  Live 8-band spectrum against a guideline target, with advice for bands 4 dB or more off target.
- **Sidechain sincronizado com o tempo** (usa o transporte do FL, sem routing): padrões "cada tempo", "meio tempo" e "future bass", profundidade, attack e release, com valores por subgénero
  (future bass, progressive house, big room) e a explicação de porquê. O release nunca ultrapassa o intervalo entre disparos nos valores por omissão.
  Tempo-synced sidechain ducker (no routing needed) with genre defaults and an explanation.
- **Bombo e baixo**: indicas a nota do bombo e o tom da música; diz onde fica o fundamental (Hz), a **frequência de cruzamento** sugerida, onde cortar o baixo, e **para que nota afinar o bombo** (tónica ou quinta do tom).
  Kick/bass advice: fundamental, crossover, where to cut the bass and which note to tune the kick to.

## Compilar · Build
- **GitHub Actions**: push da pasta → artefacto `Equilibrio-VST3`.
- **Windows (VS 2022 + CMake + Git)**: `cmake -B build -G "Visual Studio 17 2022" -A x64` e `cmake --build build --config Release --target Equilibrio_VST3`.
- **Só motor + testes**: `cmake -B build -DEQUILIBRIO_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build`.
- **Teste de áudio offline** (opcional): `-DEQUILIBRIO_BUILD_RENDER_TEST=ON` → `EquilibrioRenderTest`.

## Estado e limites · Status and limits
- Motor testado (1,5 mil verificações: filtros estáveis e com o ganho certo, curva do sidechain, bombo/baixo, conselhos). Compila em Linux e o teste de áudio offline passa (EQ de cada papel, analisador).
- **Não foi testado dentro do FL Studio.** Not tested inside FL Studio.
- Os **alvos são aproximados** e não foram calibrados com material real: usa-os como ponto de partida, não como regra. Targets are approximate and not calibrated on real material: a starting point, not a rule.
- O analisador usa 8 filtros passa-banda simples (indicativo, não um analisador de precisão). The analyser uses 8 simple band-pass filters (indicative only).
- O sidechain só funciona com o anfitrião a tocar (precisa da posição do transporte). Sidechain works only while the host is playing.
- JUCE é AGPLv3 ou comercial. O nome "Equilíbrio" é provisório. JUCE is AGPLv3 or commercial; the name is provisional.
