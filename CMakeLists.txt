cmake_minimum_required(VERSION 3.22)
project(Equilibrio VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/utf-8)
endif()

option(EQUILIBRIO_BUILD_PLUGIN "Compilar o plugin VST3 (descarrega o JUCE)" ON)
option(EQUILIBRIO_COPY_AFTER_BUILD "Copiar o VST3 para a pasta de plugins do sistema após compilar" OFF)
option(EQUILIBRIO_BUILD_RENDER_TEST "Compilar o teste de áudio offline (precisa do JUCE)" OFF)

set(EQUILIBRIO_CORE_SOURCES core/Equilibrio.cpp)

# --- Motor + testes (sem JUCE) ---
add_library(equilibrio_core STATIC ${EQUILIBRIO_CORE_SOURCES})
target_include_directories(equilibrio_core PUBLIC core)
add_executable(equilibrio_core_tests tests/core_tests.cpp)
target_link_libraries(equilibrio_core_tests PRIVATE equilibrio_core)
enable_testing()
add_test(NAME core COMMAND equilibrio_core_tests)

# --- Plugin VST3 ---
if(EQUILIBRIO_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)

  juce_add_plugin(Equilibrio
    COMPANY_NAME "Harmonia Suite"
    PLUGIN_MANUFACTURER_CODE Hrml
    PLUGIN_CODE Eqb1
    FORMATS VST3
    PRODUCT_NAME "Equilibrio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx EQ
    COPY_PLUGIN_AFTER_BUILD ${EQUILIBRIO_COPY_AFTER_BUILD})

  target_sources(Equilibrio PRIVATE ${EQUILIBRIO_CORE_SOURCES} plugin/PluginProcessor.cpp plugin/PluginEditor.cpp)
  target_include_directories(Equilibrio PRIVATE core plugin)
  target_compile_definitions(Equilibrio PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(Equilibrio
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

  if(EQUILIBRIO_BUILD_RENDER_TEST)
    juce_add_console_app(EquilibrioRenderTest PRODUCT_NAME "EquilibrioRenderTest")
    target_sources(EquilibrioRenderTest PRIVATE ${EQUILIBRIO_CORE_SOURCES} plugin/PluginProcessor.cpp plugin/PluginEditor.cpp tests/render_test.cpp)
    target_include_directories(EquilibrioRenderTest PRIVATE core plugin)
    target_compile_definitions(EquilibrioRenderTest PRIVATE
      JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(EquilibrioRenderTest PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
  endif()
endif()
