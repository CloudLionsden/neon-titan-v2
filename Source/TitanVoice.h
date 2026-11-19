#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

// Snapshot of every setting a voice needs, refreshed once per audio block by its engine.
struct OscSettings
{
    float wave = 2.0f, level = 0.8f, detune = 0.25f, spread = 0.8f;
    int octave = 0, semi = 0, unison = 1;
    float fine = 0.0f;
};

struct VoiceSettings
{
    OscSettings osc[2];
    float fm = 0.0f, ring = 0.0f;
    float subLevel = 0.0f, noiseLevel = 0.0f, click = 0.0f;
    int subOctave = 0;
    int filterType = 0;
    float cutoff = 8000.0f, resonance = 0.15f, drive = 0.1f, envAmount = 0.0f, keyTrack = 0.3f, velCut = 0.0f, vowel = 0.0f;
    juce::ADSR::Parameters ampEnv, filterEnv;
    int lfoShape = 0;
    float lfoHz = 2.0f, lfoCut = 0.0f, lfoPitch = 0.0f, lfoAmp = 0.0f, lfoFade = 0.0f;
    bool lfoRetrig = true;
    double globalLfoPhase = 0.0;
    float glide = 0.0f;
    bool mono = false;
    float pitchEnv = 0.0f, pitchDecay = 0.08f;
    int transpose = 0;
};

// Single-cycle tables for the extra wave shapes (built once, shared by every voice).
struct WaveTables
{
    static constexpr int size = 2048;
    std::array<float, size> vocal {}, organ {}, hollow {};

    WaveTables()
    {
        auto build = [] (std::array<float, size>& t, std::initializer_list<std::pair<int, float>> partials)
        {
            float peak = 0.0f;
            for (int i = 0; i < size; ++i)
            {
                const double ph = (double) i / size * juce::MathConstants<double>::twoPi;
                double v = 0.0;
                for (auto [h, a] : partials) v += a * std::sin (ph * h);
                t[(size_t) i] = (float) v;
                peak = juce::jmax (peak, std::abs ((float) v));
            }
            for (auto& v : t) v /= peak;
        };
        // "Aah"-like spectrum with energy around harmonics 2-4 and 8-10
        build (vocal,  { { 1, 0.6f }, { 2, 1.0f }, { 3, 0.9f }, { 4, 0.55f }, { 5, 0.25f }, { 6, 0.15f }, { 7, 0.2f }, { 8, 0.35f }, { 9, 0.3f }, { 10, 0.15f }, { 12, 0.08f } });
        // Drawbar organ: 8', 4', 2 2/3', 2', 1 3/5', 1'
        build (organ,  { { 1, 1.0f }, { 2, 0.8f }, { 3, 0.6f }, { 4, 0.5f }, { 5, 0.25f }, { 6, 0.3f }, { 8, 0.25f } });
        // Hollow / clarinet-like: odd harmonics only
        build (hollow, { { 1, 1.0f }, { 3, 0.45f }, { 5, 0.22f }, { 7, 0.12f }, { 9, 0.07f }, { 11, 0.04f } });
    }

    static const WaveTables& get() { static const WaveTables t; return t; }

    static float read (const std::array<float, size>& t, float phase)
    {
        const float pos = phase * size;
        const int i0 = (int) pos & (size - 1);
        const float frac = pos - std::floor (pos);
        return t[(size_t) i0] + (t[(size_t) ((i0 + 1) & (size - 1))] - t[(size_t) i0]) * frac;
    }
};

// Zero-delay-feedback state-variable filter that gives low, band, high and notch outputs at once.
struct Svf
{
    float g = 0.1f, k = 1.4f, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;

    void set (float freq, float kk, double sr)
    {
        g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (16.0f, (float) (sr * 0.45), freq) / (float) sr);
        k = kk;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0.0f; }

    struct Out { float lp, bp, hp; };
    Out process (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return { v2, v1, x - k * v1 - v2 };
    }
};

struct TitanSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class TitanVoice : public juce::SynthesiserVoice
{
public:
    explicit TitanVoice (const VoiceSettings& s) : settings (s)
    {
        for (auto& o : phases)
            for (auto& ph : o)
                ph = random.nextDouble();
    }

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<TitanSound*> (s) != nullptr; }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
        if (newRate <= 0) return;
        sr = newRate;
        ampAdsr.setSampleRate (newRate);
        filterAdsr.setSampleRate (newRate);
        resetFilters();
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        note = midiNote + settings.transpose;
        vel = velocity;
        targetFreq = juce::MidiMessage::getMidiNoteInHertz (juce::jlimit (0, 127, note));
        const bool canGlide = settings.mono && settings.glide > 0.0005f && lastFreq > 0.0;
        currentFreq = canGlide ? lastFreq : targetFreq;
        glideCoef = canGlide ? std::exp (-1.0 / juce::jmax (1.0, settings.glide * sr)) : 0.0;

        if (! wasStolenLegato)
        {
            pitchEnvValue = 1.0f;
            clickValue = 1.0f;
            ampAdsr.reset();
            filterAdsr.reset();
            resetFilters();
            samplesSinceStart = 0;
        }
        wasStolenLegato = false;

        ampAdsr.setParameters (settings.ampEnv);
        filterAdsr.setParameters (settings.filterEnv);
        ampAdsr.noteOn();
        filterAdsr.noteOn();

        lfoPhase = settings.lfoRetrig ? 0.0 : settings.globalLfoPhase;
        sampleAndHold = random.nextFloat() * 2.0f - 1.0f;
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampAdsr.noteOff();
            filterAdsr.noteOff();
        }
        else
        {
            wasStolenLegato = settings.mono;   // mono slides keep the envelope running (no click)
            if (! wasStolenLegato)
                ampAdsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int value) override { pitchBend = (value - 8192) / 8192.0f * 2.0f; }
    void controllerMoved (int, int) override {}
    using juce::SynthesiserVoice::renderNextBlock;

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (! isVoiceActive())
            return;

        ampAdsr.setParameters (settings.ampEnv);
        filterAdsr.setParameters (settings.filterEnv);

        // Per-block unison setup
        for (int o = 0; o < 2; ++o)
        {
            const auto& os = settings.osc[o];
            const int n = juce::jlimit (1, maxUnison, os.unison);
            uniCount[(size_t) o] = n;
            const float baseSemis = (float) os.octave * 12.0f + (float) os.semi + os.fine / 100.0f;
            for (int u = 0; u < n; ++u)
            {
                const float pos = n == 1 ? 0.0f : (float) u / (float) (n - 1) * 2.0f - 1.0f;
                const float cents = pos * os.detune * 50.0f;
                uniRatio[(size_t) o][(size_t) u] = std::pow (2.0f, (baseSemis + cents / 100.0f) / 12.0f);
                const float angle = (pos * os.spread + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
                uniGainL[(size_t) o][(size_t) u] = std::cos (angle);
                uniGainR[(size_t) o][(size_t) u] = std::sin (angle);
            }
            uniNorm[(size_t) o] = 1.0f / std::sqrt ((float) n);
        }

        const float driveGain = 1.0f + settings.drive * 12.0f;
        const float driveNorm = 1.0f / std::tanh (driveGain * 0.5f + 0.5f);
        const float subRatio = settings.subOctave == 0 ? 0.5f : 0.25f;
        const float keyOct = settings.keyTrack * (float) (note - 60) / 12.0f + settings.velCut * (vel - 0.85f) * 4.0f;
        const float pitchEnvCoef = std::exp (-1.0f / (juce::jmax (0.001f, settings.pitchDecay) * (float) sr * 0.25f));
        const float clickCoef = std::exp (-1.0f / (0.006f * (float) sr));
        const bool needB = settings.osc[1].level > 0.0001f || settings.fm > 0.0001f || settings.ring > 0.0001f;
        const float fmIndex = settings.fm * settings.fm * 3.0f;
        const float fadeSamples = settings.lfoFade * (float) sr;

        for (int i = 0; i < num; ++i)
        {
            if (glideCoef > 0.0)
            {
                currentFreq = targetFreq + (currentFreq - targetFreq) * glideCoef;
                if (std::abs (currentFreq - targetFreq) < 0.01) { currentFreq = targetFreq; glideCoef = 0.0; }
            }

            // LFO (with optional fade-in for natural string/vocal vibrato)
            lfoPhase += settings.lfoHz / sr;
            if (lfoPhase >= 1.0) { lfoPhase -= 1.0; sampleAndHold = random.nextFloat() * 2.0f - 1.0f; }
            float lfo = lfoValue();
            if (fadeSamples > 1.0f)
                lfo *= juce::jmin (1.0f, (float) samplesSinceStart / fadeSamples);
            ++samplesSinceStart;

            const float penv = settings.pitchEnv * pitchEnvValue;
            pitchEnvValue *= pitchEnvCoef;
            const double freq = currentFreq * std::pow (2.0f, (pitchBend + lfo * settings.lfoPitch * 2.0f + penv) / 12.0f);

            // Oscillator B first: it can be heard, modulate A (FM) or be multiplied with A (ring mod)
            float bl = 0.0f, br = 0.0f, bMono = 0.0f;
            if (needB) oscillator (1, freq, 0.0f, bl, br, bMono);
            float al = 0.0f, ar = 0.0f, aMono = 0.0f;
            if (settings.osc[0].level > 0.0001f || settings.ring > 0.0001f) oscillator (0, freq, bMono * fmIndex, al, ar, aMono);

            const float la = settings.osc[0].level, lb = settings.osc[1].level;
            float l = al * la + bl * lb, r = ar * la + br * lb;
            if (settings.ring > 0.0001f)
            {
                const float rl = al * bl * 2.0f * juce::jmax (la, lb), rr = ar * br * 2.0f * juce::jmax (la, lb);
                l += (rl - l) * settings.ring;
                r += (rr - r) * settings.ring;
            }

            if (settings.subLevel > 0.0001f)
            {
                const float sub = std::sin ((float) subPhase * juce::MathConstants<float>::twoPi) * settings.subLevel * 0.7071f;
                subPhase += freq * subRatio / sr;
                subPhase -= std::floor (subPhase);
                l += sub; r += sub;
            }
            if (settings.noiseLevel > 0.0001f)
            {
                l += (random.nextFloat() * 2.0f - 1.0f) * settings.noiseLevel * 0.3f;
                r += (random.nextFloat() * 2.0f - 1.0f) * settings.noiseLevel * 0.3f;
            }

            l = std::tanh (l * driveGain * 0.5f) * driveNorm;
            r = std::tanh (r * driveGain * 0.5f) * driveNorm;

            const float fenv = filterAdsr.getNextSample();
            if ((controlCounter++ & 15) == 0)
                updateFilters (settings.envAmount * 5.0f * fenv + lfo * settings.lfoCut * 4.0f + keyOct,
                               settings.envAmount * fenv * 2.0f + lfo * settings.lfoCut * 2.0f);
            l = filterSample (0, l);
            r = filterSample (1, r);

            if (settings.click > 0.001f && clickValue > 0.0005f)
            {
                const float c = (random.nextFloat() * 2.0f - 1.0f) * clickValue * settings.click * vel * 0.6f;
                clickValue *= clickCoef;
                l += c; r += c;
            }

            const float tremolo = 1.0f - settings.lfoAmp * (0.5f + 0.5f * lfo);
            const float amp = ampAdsr.getNextSample() * vel * tremolo;

            out.addSample (0, start + i, l * amp);
            if (out.getNumChannels() > 1)
                out.addSample (1, start + i, r * amp);

            if (! ampAdsr.isActive())
            {
                lastFreq = currentFreq;
                clearCurrentNote();
                break;
            }
        }
        lastFreq = currentFreq;
    }

private:
    static constexpr int maxUnison = 7;

    static float polyBlep (float t, float dt)
    {
        if (dt <= 0.0f) return 0.0f;
        if (t < dt)        { t /= dt;             return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    static float pulse (float t, float dt, float duty)
    {
        float t2 = t + (1.0f - duty); t2 -= std::floor (t2);
        return (t < duty ? 1.0f : -1.0f) + polyBlep (t, dt) - polyBlep (t2, dt);
    }

    static float wave (int w, float t, float dt)
    {
        const auto& tables = WaveTables::get();
        switch (w)
        {
            case 0:  return std::sin (t * juce::MathConstants<float>::twoPi);
            case 1:  return 1.0f - 4.0f * std::abs (t - 0.5f);
            case 2:  return (2.0f * t - 1.0f) - polyBlep (t, dt);
            case 3:  return pulse (t, dt, 0.5f);
            case 4:  return pulse (t, dt, 0.22f) * 0.9f;
            case 5:  return WaveTables::read (tables.vocal, t);
            case 6:  return WaveTables::read (tables.organ, t);
            default: return WaveTables::read (tables.hollow, t);
        }
    }

    // Wave knob morphs smoothly between neighbouring shapes
    static float morph (float t, float dt, float w)
    {
        w = juce::jlimit (0.0f, 7.0f, w);
        const int a = juce::jmin (6, (int) w);
        const float f = w - (float) a;
        if (f < 0.001f) return wave (a, t, dt);
        if (f > 0.999f) return wave (a + 1, t, dt);
        return wave (a, t, dt) * (1.0f - f) + wave (a + 1, t, dt) * f;
    }

    void oscillator (int o, double freq, float phaseMod, float& l, float& r, float& mono)
    {
        const auto& os = settings.osc[o];
        const int n = uniCount[(size_t) o];
        for (int u = 0; u < n; ++u)
        {
            const double inc = freq * uniRatio[(size_t) o][(size_t) u] / sr;
            auto& ph = phases[(size_t) o][(size_t) u];
            float t = (float) ph + phaseMod;
            t -= std::floor (t);
            const float s = morph (t, (float) inc, os.wave);
            ph += inc;
            ph -= std::floor (ph);
            l += s * uniGainL[(size_t) o][(size_t) u];
            r += s * uniGainR[(size_t) o][(size_t) u];
            mono += s;
        }
        const float g = uniNorm[(size_t) o];
        l *= g; r *= g; mono *= g;
    }

    void resetFilters()
    {
        for (auto& f : svf) f.reset();
        for (auto& ch : formants) for (auto& f : ch) f.reset();
    }

    void updateFilters (float octaves, float vowelMod)
    {
        const float res = settings.resonance;
        if (settings.filterType == 4)
        {
            // Vowel filter: three resonant peaks that move between A, E, I, O, U
            static const float F[5][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 }, { 570, 840, 2410 }, { 300, 870, 2240 } };
            const float pos = juce::jlimit (0.0f, 3.999f, settings.vowel + vowelMod);
            const int a = (int) pos;
            const float f = pos - (float) a;
            const float shift = std::pow (2.0f, juce::jlimit (-1.0f, 1.0f, (settings.cutoff - 1200.0f) / 6000.0f));
            const float k = 0.28f - 0.2f * res;
            for (int ch = 0; ch < 2; ++ch)
                for (int b = 0; b < 3; ++b)
                    formants[(size_t) ch][(size_t) b].set ((F[a][b] + (F[a + 1][b] - F[a][b]) * f) * shift, k, sr);
            formantK = k;
            return;
        }
        const float cutoff = settings.cutoff * std::pow (2.0f, octaves);
        const float k = 2.0f - 1.96f * res;
        svf[0].set (cutoff, k, sr);
        svf[1].set (cutoff, k, sr);
    }

    float filterSample (int ch, float x)
    {
        if (settings.filterType == 4)
        {
            auto& f = formants[(size_t) ch];
            return (f[0].process (x).bp * 1.0f + f[1].process (x).bp * 0.6f + f[2].process (x).bp * 0.3f) * formantK * 2.2f;
        }
        const auto o = svf[(size_t) ch].process (x);
        switch (settings.filterType)
        {
            case 0:  return o.lp;
            case 1:  return o.bp * svf[(size_t) ch].k;
            case 2:  return o.hp;
            default: return o.lp + o.hp;   // notch
        }
    }

    float lfoValue() const
    {
        const float t = (float) lfoPhase;
        switch (settings.lfoShape)
        {
            case 0:  return std::sin (t * juce::MathConstants<float>::twoPi);
            case 1:  return 1.0f - 4.0f * std::abs (t - 0.5f);
            case 2:  return 1.0f - 2.0f * t;
            case 3:  return t < 0.5f ? 1.0f : -1.0f;
            default: return sampleAndHold;
        }
    }

    const VoiceSettings& settings;
    juce::Random random;
    double sr = 44100.0;
    int note = 60;
    float vel = 1.0f, pitchBend = 0.0f;
    double targetFreq = 440.0, currentFreq = 440.0, lastFreq = 0.0, glideCoef = 0.0;
    bool wasStolenLegato = false;

    std::array<std::array<double, maxUnison>, 2> phases {};
    std::array<std::array<float, maxUnison>, 2> uniRatio {}, uniGainL {}, uniGainR {};
    std::array<int, 2> uniCount { 1, 1 };
    std::array<float, 2> uniNorm { 1.0f, 1.0f };
    double subPhase = 0.0, lfoPhase = 0.0;
    float sampleAndHold = 0.0f, pitchEnvValue = 0.0f, clickValue = 0.0f, formantK = 0.2f;
    unsigned int controlCounter = 0;
    int samplesSinceStart = 0;

    juce::ADSR ampAdsr, filterAdsr;
    std::array<Svf, 2> svf;
    std::array<std::array<Svf, 3>, 2> formants;
};

// Synthesiser with a mono/glide mode (one voice that slides between notes, great for 808s).
class TitanSynth : public juce::Synthesiser
{
public:
    bool mono = false;

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (! mono || getNumVoices() == 0)
        {
            juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
            return;
        }
        const juce::ScopedLock sl (lock);
        for (auto* sound : sounds)
            if (sound->appliesToNote (midiNoteNumber) && sound->appliesToChannel (midiChannel))
            {
                startVoice (getVoice (0), sound, midiChannel, midiNoteNumber, velocity);
                return;
            }
    }
};
