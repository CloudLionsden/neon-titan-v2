#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <unordered_map>
#include <string>
#include "TitanVoice.h"

// One complete NEON TITAN layer: 16-voice synth + its own effects chain.
class TitanEngine
{
public:
    static constexpr int numVoices = 16;

    TitanEngine (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
    {
        for (auto* param : apvts.processor.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            {
                const auto id = rp->getParameterID();
                if (id.startsWith (prefix))
                    raw[id.substring (prefix.length()).toStdString()] = apvts.getRawParameterValue (id);
            }

        static const char* oscFields[8] = { "_wave", "_oct", "_semi", "_fine", "_level", "_uni", "_detune", "_spread" };
        for (int o = 0; o < 2; ++o)
            for (int f = 0; f < 8; ++f)
                oscRaw[(size_t) o][(size_t) f] = raw.at (std::string (o == 0 ? "oscA" : "oscB") + oscFields[f]);

        for (auto& h : chordHeld) h.reserve (8);
        midi.ensureSize (8192);
        chordOut.ensureSize (8192);
        for (int i = 0; i < numVoices; ++i)
            synth.addVoice (new TitanVoice (settings));
        synth.addSound (new TitanSound());
    }

    void prepare (double sampleRate, int blockSize)
    {
        sr = sampleRate;
        synth.setCurrentPlaybackSampleRate (sampleRate);
        temp.setSize (2, juce::jmax (1, blockSize));

        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, blockSize), 2 };
        chorus.prepare (spec);
        chorus.setRate (0.8f); chorus.setDepth (0.3f); chorus.setCentreDelay (8.0f); chorus.setFeedback (0.0f);
        phaser.prepare (spec);
        phaser.setRate (0.35f); phaser.setDepth (0.8f); phaser.setCentreFrequency (900.0f); phaser.setFeedback (0.55f);
        reverb.prepare (spec);
        reverb.reset();
        delayL.assign ((size_t) (sampleRate * 4.0) + 1, 0.0f);
        delayR.assign ((size_t) (sampleRate * 4.0) + 1, 0.0f);
        delayWrite = 0;
        squashEnv = 0.0f;
        crushHold[0] = crushHold[1] = 0.0f; crushCount = 0.0f;
        for (auto* s : { &gainL, &gainR, &dist, &width })
            s->reset (sampleRate, 0.05);
        gainL.setCurrentAndTargetValue (0.0f);
        gainR.setCurrentAndTargetValue (0.0f);
        dist.setCurrentAndTargetValue (get ("fx_dist"));
        width.setCurrentAndTargetValue (get ("fx_width"));
    }

    bool isOn() const { return get ("on") > 0.5f; }

    // Renders this layer and ADDS it into out.
    void process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midiIn, double bpm)
    {
        const bool on = isOn();
        if (! on)
        {
            if (wasOn) { synth.allNotesOff (0, false); for (auto& h : chordHeld) h.clear(); }
            wasOn = false;
            return;
        }
        wasOn = true;

        const int n = out.getNumSamples();
        if (temp.getNumSamples() < n) temp.setSize (2, n, false, false, true);
        juce::AudioBuffer<float> buf (temp.getArrayOfWritePointers(), 2, n);
        buf.clear();

        updateSettings (bpm);
        lfoGlobalPhase += settings.lfoHz * n / sr;
        lfoGlobalPhase -= std::floor (lfoGlobalPhase);

        midi.clear();
        midi.addEvents (midiIn, 0, n, 0);
        expandChords (midi);
        synth.renderNextBlock (buf, midi, 0, n);
        buf.applyGain (0.5f);
        applyFx (buf, bpm);

        // Layer volume and pan into the mix
        const float vol = juce::Decibels::decibelsToGain (get ("vol"), -35.9f);
        const float pan = get ("pan");
        const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        gainL.setTargetValue (vol * std::cos (angle) * 1.41421356f);
        gainR.setTargetValue (vol * std::sin (angle) * 1.41421356f);
        auto* oL = out.getWritePointer (0);
        auto* oR = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;
        const auto* L = buf.getReadPointer (0);
        const auto* R = buf.getReadPointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float gl = gainL.getNextValue(), gr = gainR.getNextValue();
            if (oR) { oL[i] += L[i] * gl; oR[i] += R[i] * gr; }
            else      oL[i] += (L[i] * gl + R[i] * gr) * 0.5f;
        }
    }

    float get (const std::string& id) const { return raw.at (id)->load(); }

private:
    void updateSettings (double bpm)
    {
        const float mSize = get ("m_size"), mGrowl = get ("m_growl"), mWobble = get ("m_wobble"), mBright = get ("m_bright");
        for (int o = 0; o < 2; ++o)
        {
            const auto& r = oscRaw[(size_t) o];
            auto& os = settings.osc[o];
            os.wave   = r[0]->load();
            os.octave = (int) r[1]->load();
            os.semi   = (int) r[2]->load();
            os.fine   = r[3]->load();
            os.level  = r[4]->load();
            os.unison = (int) r[5]->load();
            os.detune = juce::jlimit (0.0f, 1.0f, r[6]->load() + mSize * 0.4f);
            os.spread = juce::jlimit (0.0f, 1.0f, r[7]->load() + mSize * 0.3f);
            if (mSize > 0.5f && os.unison < 3) os.unison = 3;
        }
        settings.fm         = get ("fm");
        settings.ring       = get ("ring");
        settings.subLevel   = get ("sub_level");
        settings.subOctave  = (int) get ("sub_oct");
        settings.noiseLevel = get ("noise_level");
        settings.click      = get ("click");
        settings.filterType = (int) get ("flt_type");
        settings.cutoff     = juce::jlimit (20.0f, 20000.0f, get ("flt_cutoff") * std::pow (2.0f, mBright * 3.0f));
        settings.resonance  = juce::jlimit (0.0f, 1.0f, get ("flt_res") + mGrowl * 0.35f);
        settings.drive      = juce::jlimit (0.0f, 1.0f, get ("flt_drive") + mGrowl * 0.5f);
        settings.envAmount  = get ("flt_env");
        settings.keyTrack   = get ("flt_key");
        settings.velCut     = get ("vel_cut");
        settings.vowel      = get ("vowel");
        settings.ampEnv     = { get ("amp_a"),  get ("amp_d"),  get ("amp_s"),  get ("amp_r") };
        settings.filterEnv  = { get ("fenv_a"), get ("fenv_d"), get ("fenv_s"), get ("fenv_r") };
        settings.lfoShape   = (int) get ("lfo_shape");
        const int sync = (int) get ("lfo_sync");
        static const double beatsPerCycle[] = { 0.0, 4.0, 2.0, 1.0, 0.5, 0.25, 2.0 / 3.0, 1.0 / 3.0 };
        settings.lfoHz      = sync == 0 ? get ("lfo_rate") : (float) (bpm / 60.0 / beatsPerCycle[juce::jlimit (1, 7, sync)]);
        settings.lfoCut     = juce::jlimit (0.0f, 1.0f, get ("lfo_cut") + mWobble);
        settings.lfoPitch   = get ("lfo_pitch");
        settings.lfoAmp     = get ("lfo_amp");
        settings.lfoFade    = get ("lfo_fade");
        settings.lfoRetrig  = get ("lfo_retrig") > 0.5f;
        settings.glide      = get ("glide");
        settings.mono       = get ("mono") > 0.5f;
        settings.pitchEnv   = get ("pitch_env");
        settings.pitchDecay = get ("pitch_dec");
        settings.transpose  = (int) get ("transpose");
        settings.globalLfoPhase = lfoGlobalPhase;
        synth.mono = settings.mono;
    }

    void applyFx (juce::AudioBuffer<float>& buffer, double bpm)
    {
        const int numSamples = buffer.getNumSamples();
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);

        // Distortion
        dist.setTargetValue (get ("fx_dist") + get ("m_growl") * 0.3f);
        for (int i = 0; i < numSamples; ++i)
        {
            const float d = juce::jlimit (0.0f, 1.0f, dist.getNextValue());
            if (d > 0.001f)
            {
                const float g = 1.0f + d * 20.0f, norm = 1.0f / std::tanh (g), mix = juce::jmin (1.0f, d * 1.5f);
                L[i] += (std::tanh (L[i] * g) * norm - L[i]) * mix;
                R[i] += (std::tanh (R[i] * g) * norm - R[i]) * mix;
            }
        }

        // Squash: OTT-style upward + downward compression for loud, dense modern sounds
        const float squash = get ("fx_squash");
        if (squash > 0.001f)
        {
            const float att = std::exp (-1.0f / (0.002f * (float) sr)), rel = std::exp (-1.0f / (0.08f * (float) sr));
            for (int i = 0; i < numSamples; ++i)
            {
                const float lvl = juce::jmax (std::abs (L[i]), std::abs (R[i]));
                squashEnv = lvl > squashEnv ? att * squashEnv + (1.0f - att) * lvl : rel * squashEnv + (1.0f - rel) * lvl;
                const float g = juce::jlimit (0.25f, 6.0f, std::pow (0.25f / (squashEnv + 1.0e-4f), squash * 0.7f));
                const float mix = juce::jmin (1.0f, squash * 1.3f);
                L[i] += (L[i] * g - L[i]) * mix;
                R[i] += (R[i] * g - R[i]) * mix;
            }
        }

        // Bitcrusher: fewer bits + lower sample rate
        const float crush = get ("fx_crush");
        if (crush > 0.001f)
        {
            const float levels = std::pow (2.0f, 16.0f - crush * 12.0f);
            const float step = 1.0f + crush * crush * 24.0f;
            for (int i = 0; i < numSamples; ++i)
            {
                crushCount += 1.0f;
                if (crushCount >= step)
                {
                    crushCount -= step;
                    crushHold[0] = std::round (L[i] * levels) / levels;
                    crushHold[1] = std::round (R[i] * levels) / levels;
                }
                L[i] += (crushHold[0] - L[i]) * juce::jmin (1.0f, crush * 2.0f);
                R[i] += (crushHold[1] - R[i]) * juce::jmin (1.0f, crush * 2.0f);
            }
        }

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);

        if (const float c = get ("fx_chorus"); c > 0.001f) { chorus.setMix (c * 0.6f); chorus.process (ctx); }
        if (const float ph = get ("fx_phaser"); ph > 0.001f) { phaser.setMix (ph * 0.7f); phaser.process (ctx); }

        // Ping-pong delay, locked to tempo
        if (const float dMix = get ("dly_mix"); dMix > 0.001f && ! delayL.empty())
        {
            static const double beats[] = { 1.0, 0.5, 0.75, 0.25, 1.5, 1.0 / 3.0 };
            const int len = (int) delayL.size();
            const int dSamples = juce::jlimit (1, len - 1, (int) (beats[juce::jlimit (0, 5, (int) get ("dly_time"))] * 60.0 / bpm * sr));
            const float fb = get ("dly_fb");
            for (int i = 0; i < numSamples; ++i)
            {
                const int r = (delayWrite - dSamples + len) % len;
                const float outL = delayL[(size_t) r], outR = delayR[(size_t) r];
                delayL[(size_t) delayWrite] = (L[i] + R[i]) * 0.5f + outR * fb;
                delayR[(size_t) delayWrite] = outL * fb;
                L[i] += outL * dMix * 0.7f;
                R[i] += outR * dMix * 0.7f;
                delayWrite = (delayWrite + 1) % len;
            }
        }

        const float revMix = juce::jlimit (0.0f, 1.0f, get ("fx_reverb") + get ("m_size") * 0.35f);
        if (revMix > 0.001f)
        {
            juce::dsp::Reverb::Parameters rp;
            rp.roomSize = juce::jlimit (0.0f, 1.0f, get ("fx_size") + get ("m_size") * 0.2f);
            rp.damping = 0.45f;
            rp.wetLevel = revMix * 0.5f;
            rp.dryLevel = 1.0f - revMix * 0.35f;
            rp.width = 1.0f;
            reverb.setParameters (rp);
            reverb.process (ctx);
        }

        width.setTargetValue (juce::jlimit (0.0f, 2.0f, get ("fx_width") + get ("m_size") * 0.5f));
        const float out = juce::Decibels::decibelsToGain (get ("master"));
        for (int i = 0; i < numSamples; ++i)
        {
            const float w = width.getNextValue();
            const float mid = (L[i] + R[i]) * 0.5f, side = (L[i] - R[i]) * 0.5f * w;
            L[i] = std::tanh ((mid + side) * out);
            R[i] = std::tanh ((mid - side) * out);
        }
    }

    void expandChords (juce::MidiBuffer& m)
    {
        static const std::vector<std::vector<int>> shapes {
            { 0 }, { 0, 4, 7 }, { 0, 3, 7 }, { 0, 4, 7, 11 }, { 0, 3, 7, 10 }, { 0, 4, 7, 10 }, { 0, 4, 7, 11, 14 },
            { 0, 3, 7, 10, 14 }, { 0, 2, 7 }, { 0, 5, 7 }, { 0, 4, 7, 14 }, { 0, 3, 7, 10, 14, 17 }, { 0, 7, 12 }, { 0, 12 } };
        const int type = settings.mono ? 0 : juce::jlimit (0, (int) shapes.size() - 1, (int) get ("chord"));
        bool anyHeld = false;
        for (const auto& h : chordHeld) if (! h.empty()) { anyHeld = true; break; }
        if (type == 0 && ! anyHeld) return;

        chordOut.clear();
        for (const auto meta : m)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                auto& held = chordHeld[(size_t) msg.getNoteNumber()];
                for (int nn : held) chordOut.addEvent (juce::MidiMessage::noteOff (msg.getChannel(), nn), meta.samplePosition);
                held.clear();
                for (int iv : shapes[(size_t) type])
                    if (const int nn = msg.getNoteNumber() + iv; nn <= 127)
                    {
                        held.push_back (nn);
                        chordOut.addEvent (juce::MidiMessage::noteOn (msg.getChannel(), nn, msg.getVelocity()), meta.samplePosition);
                    }
            }
            else if (msg.isNoteOff())
            {
                auto& held = chordHeld[(size_t) msg.getNoteNumber()];
                if (held.empty()) chordOut.addEvent (msg, meta.samplePosition);
                for (int nn : held) chordOut.addEvent (juce::MidiMessage::noteOff (msg.getChannel(), nn), meta.samplePosition);
                held.clear();
            }
            else chordOut.addEvent (msg, meta.samplePosition);
        }
        m.swapWith (chordOut);
    }

    std::unordered_map<std::string, std::atomic<float>*> raw;
    std::array<std::array<std::atomic<float>*, 8>, 2> oscRaw {};
    TitanSynth synth;
    VoiceSettings settings;
    juce::MidiBuffer midi, chordOut;
    std::array<std::vector<int>, 128> chordHeld;
    juce::AudioBuffer<float> temp;
    double sr = 44100.0, lfoGlobalPhase = 0.0;
    bool wasOn = false;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Reverb reverb;
    std::vector<float> delayL, delayR;
    int delayWrite = 0;
    float squashEnv = 0.0f, crushHold[2] { 0, 0 }, crushCount = 0.0f;
    juce::SmoothedValue<float> gainL, gainR, dist, width;
};
