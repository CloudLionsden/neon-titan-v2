#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// NEON TITAN parameters.
// Each of the 4 layers has a full engine; its parameter IDs are prefixed "L1_" ... "L4_".
// Presets store un-prefixed IDs (e.g. "flt_cutoff") and are applied to a layer by adding the prefix.
namespace Params
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
    using Range  = juce::NormalisableRange<float>;
    using PList  = std::vector<std::unique_ptr<juce::RangedAudioParameter>>;

    static constexpr int numLayers = 4;

    inline const juce::StringArray filterTypes { "Low Pass", "Band Pass", "High Pass", "Notch", "Vowel" };
    inline const juce::StringArray lfoShapes   { "Sine", "Triangle", "Saw", "Square", "Random" };
    inline const juce::StringArray lfoSyncs    { "Free", "1/1", "1/2", "1/4", "1/8", "1/16", "1/4 T", "1/8 T" };
    inline const juce::StringArray subOctaves  { "-1 Oct", "-2 Oct" };
    inline const juce::StringArray chordTypes  { "Off", "Major", "Minor", "Maj7", "Min7", "Dom7", "Maj9", "Min9", "Sus2", "Sus4", "Add9", "Min11", "Power", "Octave" };
    inline const juce::StringArray delayTimes  { "1/4", "1/8", "1/8 D", "1/16", "1/4 D", "1/8 T" };
    // Wave knob positions: 0 Sine, 1 Triangle, 2 Saw, 3 Square, 4 Pulse, 5 Vocal, 6 Organ, 7 Hollow
    static constexpr float maxWave = 7.0f;

    inline juce::String layerPrefix (int layer) { return "L" + juce::String (layer + 1) + "_"; }

    inline Range skewed (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    struct Builder
    {
        PList& p;
        juce::String id, name;   // prefix for IDs and display names

        void f (const juce::String& pid, const juce::String& pname, Range r, float def)
        { p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + pid, 1 }, name + pname, r, def)); }
        void i (const juce::String& pid, const juce::String& pname, int lo, int hi, int def)
        { p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id + pid, 1 }, name + pname, lo, hi, def)); }
        void c (const juce::String& pid, const juce::String& pname, const juce::StringArray& items, int def)
        { p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id + pid, 1 }, name + pname, items, def)); }
        void b (const juce::String& pid, const juce::String& pname, bool def)
        { p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id + pid, 1 }, name + pname, def)); }
    };

    inline void addEngine (PList& p, const juce::String& idPrefix, const juce::String& namePrefix, bool onByDefault)
    {
        Builder b { p, idPrefix, namePrefix };
        for (auto [o, on, lvl] : { std::tuple<const char*, const char*, float> { "oscA", "Osc A ", 0.8f }, { "oscB", "Osc B ", 0.0f } })
        {
            const juce::String id (o), nm (on);
            b.f (id + "_wave", nm + "Wave", Range (0.0f, maxWave), 2.0f);
            b.i (id + "_oct", nm + "Octave", -3, 3, 0);
            b.i (id + "_semi", nm + "Semitone", -12, 24, 0);
            b.f (id + "_fine", nm + "Fine", Range (-100.0f, 100.0f), 0.0f);
            b.f (id + "_level", nm + "Level", Range (0.0f, 1.0f), lvl);
            b.i (id + "_uni", nm + "Unison", 1, 7, 1);
            b.f (id + "_detune", nm + "Detune", Range (0.0f, 1.0f), 0.25f);
            b.f (id + "_spread", nm + "Spread", Range (0.0f, 1.0f), 0.8f);
        }
        b.f ("fm", "FM (B > A)", Range (0.0f, 1.0f), 0.0f);
        b.f ("ring", "Ring Mod", Range (0.0f, 1.0f), 0.0f);
        b.f ("sub_level", "Sub Level", Range (0.0f, 1.0f), 0.0f);
        b.c ("sub_oct", "Sub Octave", subOctaves, 0);
        b.f ("noise_level", "Noise", Range (0.0f, 1.0f), 0.0f);
        b.f ("click", "Hammer Click", Range (0.0f, 1.0f), 0.0f);

        b.c ("flt_type", "Filter Type", filterTypes, 0);
        b.f ("flt_cutoff", "Cutoff", skewed (20.0f, 20000.0f, 1200.0f), 8000.0f);
        b.f ("flt_res", "Resonance", Range (0.0f, 1.0f), 0.15f);
        b.f ("flt_drive", "Drive", Range (0.0f, 1.0f), 0.1f);
        b.f ("flt_env", "Env Amount", Range (-1.0f, 1.0f), 0.0f);
        b.f ("flt_key", "Key Track", Range (0.0f, 1.0f), 0.3f);
        b.f ("vel_cut", "Velocity > Cutoff", Range (0.0f, 1.0f), 0.0f);
        b.f ("vowel", "Vowel (A-E-I-O-U)", Range (0.0f, 4.0f), 0.0f);

        for (auto [e, en, a, d, s, r] : { std::tuple<const char*, const char*, float, float, float, float> { "fenv", "Filter Env ", 0.005f, 0.4f, 0.3f, 0.4f },
                                                                                                   { "amp", "Amp ", 0.003f, 0.3f, 0.9f, 0.25f } })
        {
            const juce::String id (e), nm (en);
            b.f (id + "_a", nm + "Attack",  skewed (0.001f, 5.0f, 0.3f), a);
            b.f (id + "_d", nm + "Decay",   skewed (0.001f, 5.0f, 0.4f), d);
            b.f (id + "_s", nm + "Sustain", Range (0.0f, 1.0f), s);
            b.f (id + "_r", nm + "Release", skewed (0.001f, 8.0f, 0.5f), r);
        }

        b.c ("lfo_shape", "LFO Shape", lfoShapes, 0);
        b.c ("lfo_sync", "LFO Sync", lfoSyncs, 0);
        b.f ("lfo_rate", "LFO Rate", skewed (0.05f, 20.0f, 2.0f), 2.0f);
        b.f ("lfo_cut", "LFO > Cutoff", Range (0.0f, 1.0f), 0.0f);
        b.f ("lfo_pitch", "LFO > Pitch", Range (0.0f, 1.0f), 0.0f);
        b.f ("lfo_amp", "LFO > Volume", Range (0.0f, 1.0f), 0.0f);
        b.f ("lfo_fade", "LFO Fade In", skewed (0.0f, 4.0f, 0.6f), 0.0f);
        b.b ("lfo_retrig", "LFO Retrigger", true);

        b.b ("mono", "Mono", false);
        b.c ("chord", "Chord Mode", chordTypes, 0);
        b.f ("glide", "Glide", skewed (0.0f, 1.0f, 0.15f), 0.0f);
        b.f ("pitch_env", "Pitch Env", Range (-24.0f, 24.0f), 0.0f);
        b.f ("pitch_dec", "Pitch Decay", skewed (0.005f, 2.0f, 0.1f), 0.08f);

        b.f ("fx_dist", "Distortion", Range (0.0f, 1.0f), 0.0f);
        b.f ("fx_squash", "Squash (OTT)", Range (0.0f, 1.0f), 0.0f);
        b.f ("fx_crush", "Bitcrush", Range (0.0f, 1.0f), 0.0f);
        b.f ("fx_chorus", "Chorus", Range (0.0f, 1.0f), 0.0f);
        b.f ("fx_phaser", "Phaser", Range (0.0f, 1.0f), 0.0f);
        b.f ("dly_mix", "Delay", Range (0.0f, 1.0f), 0.0f);
        b.c ("dly_time", "Delay Time", delayTimes, 1);
        b.f ("dly_fb", "Delay Feedback", Range (0.0f, 0.9f), 0.35f);
        b.f ("fx_reverb", "Reverb", Range (0.0f, 1.0f), 0.15f);
        b.f ("fx_size", "Room Size", Range (0.0f, 1.0f), 0.6f);
        b.f ("fx_width", "Width", Range (0.0f, 2.0f), 1.0f);
        b.f ("master", "Output", Range (-24.0f, 6.0f), -6.0f);

        b.f ("m_size", "Macro SIZE", Range (0.0f, 1.0f), 0.0f);
        b.f ("m_growl", "Macro GROWL", Range (0.0f, 1.0f), 0.0f);
        b.f ("m_wobble", "Macro WOBBLE", Range (0.0f, 1.0f), 0.0f);
        b.f ("m_bright", "Macro BRIGHT", Range (-1.0f, 1.0f), 0.0f);

        // Layer mixer controls (not stored in single-sound presets)
        b.b ("on", "On", onByDefault);
        b.f ("vol", "Volume", Range (-36.0f, 6.0f), 0.0f);
        b.f ("pan", "Pan", Range (-1.0f, 1.0f), 0.0f);
        b.i ("transpose", "Transpose", -24, 24, 0);
    }

    inline Layout createLayout()
    {
        PList p;
        for (int l = 0; l < numLayers; ++l)
            addEngine (p, layerPrefix (l), "L" + juce::String (l + 1) + " ", l == 0);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "master_vol", 1 }, "Master Volume", Range (-36.0f, 6.0f), 0.0f));
        return { p.begin(), p.end() };
    }

    // IDs that belong to the layer mixer rather than the sound itself
    inline bool isMixerParam (const juce::String& unprefixedId)
    {
        return unprefixedId == "on" || unprefixedId == "vol" || unprefixedId == "pan" || unprefixedId == "transpose";
    }
}
