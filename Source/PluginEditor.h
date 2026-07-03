#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class DelayAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    DelayAudioProcessorEditor(DelayAudioProcessor&);
    ~DelayAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    DelayAudioProcessor& processor;

    class WireframeVisualizer;
    class ToggleSwitch;
    class AutoGainButton;
    class TempoControl;
    class RotaryKnob;
    class IconButton;
    class VUChannel;

    std::unique_ptr<WireframeVisualizer> visualizer;
    std::unique_ptr<ToggleSwitch> modeToggle;
    std::unique_ptr<AutoGainButton> autoGainBtn;
    std::unique_ptr<TempoControl> tempoControl;
    std::unique_ptr<RotaryKnob> regenKnob;
    std::unique_ptr<RotaryKnob> mixKnob;
    std::unique_ptr<RotaryKnob> outputKnob;
    std::unique_ptr<IconButton> brightnessBtn;
    std::unique_ptr<IconButton> colorBtn;
    std::unique_ptr<IconButton> sparkleBtn;
    std::unique_ptr<VUChannel> meterIn;
    std::unique_ptr<VUChannel> meterOut;

    juce::Image noiseTexture;
    void generateNoiseTexture();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayAudioProcessorEditor)
};
