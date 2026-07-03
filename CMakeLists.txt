cmake_minimum_required(VERSION 3.15)
project(DelayVST VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE 7.0.12
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DelayVST
    COMPANY_NAME "DelayCorp"
    PLUGIN_MANUFACTURER_CODE DlyC
    PLUGIN_CODE Dly1
    FORMATS VST3
    PRODUCT_NAME "Delay"
    PLUGIN_DESCRIPTION "Professional Delay VST3"
)

target_sources(DelayVST PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(DelayVST
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DelayVST PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)
