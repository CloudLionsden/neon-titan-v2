#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

NeonTitanProcessor::NeonTitanProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NeonTitan", Params::createLayout())
{
    for (int l = 0; l < Params::numLayers; ++l)
        engines[(size_t) l] = std::make_unique<TitanEngine> (apvts, Params::layerPrefix (l));
    masterVolRaw = apvts.getRawParameterValue ("master_vol");
}

bool NeonTitanProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void NeonTitanProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& e : engines) e->prepare (sampleRate, samplesPerBlock);
    keyboardState.reset();
    masterGain.reset (sampleRate, 0.05);
    masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (masterVolRaw->load(), -35.9f));
}

void NeonTitanProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    double bpm = fallbackBpm;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto hostBpm = pos->getBpm())
                bpm = *hostBpm;

    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    for (auto& e : engines)
        e->process (buffer, midi, bpm);

    // Master volume, then a gentle limiter so layered sounds never clip Maschine's input
    masterGain.setTargetValue (juce::Decibels::decibelsToGain (masterVolRaw->load(), -35.9f));
    float peakL = 0.0f, peakR = 0.0f;
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        const float g = masterGain.getNextValue();
        L[i] = std::tanh (L[i] * g);
        peakL = juce::jmax (peakL, std::abs (L[i]));
        if (R) { R[i] = std::tanh (R[i] * g); peakR = juce::jmax (peakR, std::abs (R[i])); }
    }
    meterL.store (juce::jmax (peakL, meterL.load() * 0.9f));
    meterR.store (juce::jmax (R ? peakR : peakL, meterR.load() * 0.9f));
}

// ---------------- Presets ----------------
int NeonTitanProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }

const juce::String NeonTitanProcessor::getProgramName (int index)
{
    const auto& p = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? p[(size_t) index].category + " - " + p[(size_t) index].name : juce::String();
}

void NeonTitanProcessor::setParam (const juce::String& id, float realValue)
{
    if (auto* rp = apvts.getParameter (id))
        rp->setValueNotifyingHost (rp->convertTo0to1 (realValue));
}

void NeonTitanProcessor::loadPresetIntoLayer (int presetIndex, int layer)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (presetIndex, (int) presets.size()) || ! juce::isPositiveAndBelow (layer, Params::numLayers))
        return;
    const auto& p = presets[(size_t) presetIndex];
    if (p.isMulti)
    {
        setCurrentProgram (presetIndex);
        return;
    }
    const auto prefix = Params::layerPrefix (layer);

    // Reset this layer's sound to defaults (mixer settings are kept), then apply the preset
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto id = rp->getParameterID();
            if (id.startsWith (prefix) && ! Params::isMixerParam (id.substring (prefix.length())))
                rp->setValueNotifyingHost (rp->getDefaultValue());
        }
    for (const auto& [id, value] : p.values)
        setParam (prefix + id, value);
    setParam (prefix + "on", 1.0f);
    layerPreset[(size_t) layer] = presetIndex;
    currentProgram = presetIndex;
}

void NeonTitanProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;
    const auto& p = presets[(size_t) index];
    if (! p.isMulti)
    {
        loadPresetIntoLayer (index, selectedLayer.load());
        return;
    }
    for (int l = 0; l < Params::numLayers; ++l)
    {
        const auto prefix = Params::layerPrefix (l);
        const auto& lp = p.layers[(size_t) l];
        if (lp.preset >= 0)
        {
            loadPresetIntoLayer (lp.preset, l);
            setParam (prefix + "vol", lp.vol);
            setParam (prefix + "pan", lp.pan);
            setParam (prefix + "transpose", (float) lp.transpose);
        }
        else
        {
            setParam (prefix + "on", 0.0f);
            layerPreset[(size_t) l] = -1;
        }
    }
    currentProgram = index;
}

void NeonTitanProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    for (int l = 0; l < Params::numLayers; ++l)
        state.setProperty ("layerPreset" + juce::String (l), layerPreset[(size_t) l], nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void NeonTitanProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            for (int l = 0; l < Params::numLayers; ++l)
                layerPreset[(size_t) l] = tree.getProperty ("layerPreset" + juce::String (l), l == 0 ? 0 : -1);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* NeonTitanProcessor::createEditor() { return new NeonTitanEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NeonTitanProcessor(); }
