#include "PluginProcessor.h"
#include "PluginEditor.h"

DelayAudioProcessor::DelayAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

DelayAudioProcessor::~DelayAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout DelayAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "tempo", "Tempo",
        juce::StringArray{ "1/32", "1/16", "1/8", "1/4", "1/2", "1/1" }, 2));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "regen", "Regen", juce::NormalisableRange<float>(0.0f, 100.0f), 30.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f), 50.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output", juce::NormalisableRange<float>(-60.0f, 12.0f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>("mode", "Mode", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("autogain", "Auto Gain", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("brightness", "Brightness", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("color", "Color", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("sparkle", "Sparkle", false));

    return layout;
}

void DelayAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    int maxDelaySamples = static_cast<int>(sr * 2.0) + samplesPerBlock;
    delayBuffer.setSize(getTotalNumOutputChannels(), maxDelaySamples);
    delayBuffer.clear();
    writePos = 0;
}

void DelayAudioProcessor::releaseResources() {}

void DelayAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto tempo   = apvts.getRawParameterValue("tempo")->load();
    auto regen   = apvts.getRawParameterValue("regen")->load() / 100.0f;
    auto mix     = apvts.getRawParameterValue("mix")->load() / 100.0f;
    auto outGain = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("output")->load());
    auto autoGain = apvts.getRawParameterValue("autogain")->load() > 0.5f;

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto pos = playHead->getPosition())
            bpm = pos->getBpm().orFallback(120.0);

    float div = 0.125f;
    switch ((int)tempo)
    {
        case 0: div = 1.0f / 32.0f; break;
        case 1: div = 1.0f / 16.0f; break;
        case 2: div = 1.0f / 8.0f;  break;
        case 3: div = 1.0f / 4.0f;  break;
        case 4: div = 1.0f / 2.0f;  break;
        case 5: div = 1.0f;         break;
    }

    float delayTime = static_cast<float>(60.0 / bpm * 4.0 * div);
    int delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 1,
                                    static_cast<int>(delayTime * sampleRate));

    // Input metering
    float inRMS = 0.0f;
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        auto* d = buffer.getReadPointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s)
            inRMS += d[s] * d[s];
    }
    inRMS = std::sqrt(inRMS / (buffer.getNumSamples() * totalNumInputChannels));
    inputLevel.store(inRMS);

    // Delay processing
    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto* delayData = delayBuffer.getWritePointer(ch);
        int len = delayBuffer.getNumSamples();

        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            float input = data[s];
            int readPos = (writePos - delaySamples + len) % len;
            float delayed = delayData[readPos];
            float feedback = input + delayed * regen;
            delayData[writePos] = feedback;

            float wetDry = input * (1.0f - mix) + delayed * mix;
            data[s] = wetDry * outGain;

            writePos = (writePos + 1) % len;
        }
    }

    if (autoGain)
    {
        float comp = 1.0f / (1.0f + regen * 0.5f);
        buffer.applyGain(comp);
    }

    // Output metering
    float outRMS = 0.0f;
    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        auto* d = buffer.getReadPointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s)
            outRMS += d[s] * d[s];
    }
    outRMS = std::sqrt(outRMS / (buffer.getNumSamples() * totalNumOutputChannels));
    outputLevel.store(outRMS);
}

juce::AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new DelayAudioProcessorEditor(*this);
}

void DelayAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DelayAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}
