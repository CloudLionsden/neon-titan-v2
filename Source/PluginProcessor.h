#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TitanEngine.h"
#include "Params.h"

class NeonTitanProcessor : public juce::AudioProcessor
{
public:
    NeonTitanProcessor();
    ~NeonTitanProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NEON TITAN"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // Host program list = the whole preset library. Singles load into the selected layer; Multis load all 4.
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void loadPresetIntoLayer (int presetIndex, int layer);
    int getLayerPreset (int layer) const { return layerPreset[(size_t) juce::jlimit (0, 3, layer)]; }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    std::atomic<int> selectedLayer { 0 };
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f };
    double fallbackBpm = 120.0;   // used when the host gives no tempo (and by the test tool)

private:
    void setParam (const juce::String& id, float realValue);

    std::array<std::unique_ptr<TitanEngine>, Params::numLayers> engines;
    std::array<int, Params::numLayers> layerPreset { 0, -1, -1, -1 };
    int currentProgram = 0;
    juce::SmoothedValue<float> masterGain;
    std::atomic<float>* masterVolRaw = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonTitanProcessor)
};
