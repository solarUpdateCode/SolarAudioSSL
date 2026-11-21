cmake_minimum_required(VERSION 3.22)
project(SolarAudioSSL VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
endif()
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(SolarAudioSSL
    COMPANY_NAME "SolarAudio"
    COMPANY_WEBSITE "https://github.com/solarUpdateCode/SolarAudioSSL"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Sola PLUGIN_CODE Sssl
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "SolarAudioSSL"
    BUNDLE_ID "com.solaraudio.solaraudiossl"
    AU_MAIN_TYPE kAudioUnitType_Effect)
target_sources(SolarAudioSSL PRIVATE Source/SSLDSP.h Source/SolarAudioSSL.cpp)
target_compile_definitions(SolarAudioSSL PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_JUCE_APP_USAGE=0 JUCE_STRICT_REFCOUNTEDPOINTER=1)
target_link_libraries(SolarAudioSSL PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(SolarAudioSSL PRIVATE /W3 /bigobj)
else()
    target_compile_options(SolarAudioSSL PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
