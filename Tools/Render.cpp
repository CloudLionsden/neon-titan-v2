// NEON TITAN test tool (not part of the plugin).
//   TitanRender qc <start> <end>          render presets [start, end) and print CSV: index,category,subtype,name,peak,rms,ok
//   TitanRender demo <outDir> <i,j,k...>  write WAV previews of the listed presets
//   TitanRender shot <outDir>             save a PNG of the plugin window
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/Presets.h"

namespace
{
    constexpr double sr = 44100.0, bpm = 140.0;
    constexpr int block = 256;

    struct N { double start, len; int note; };

    juce::AudioBuffer<float> renderPreset (int index)
    {
        const double beat = 60.0 / bpm;
        NeonTitanProcessor proc;
        proc.fallbackBpm = bpm;
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.setCurrentProgram (index);
        proc.prepareToPlay (sr, block);

        const auto& p = getFactoryPresets()[(size_t) index];
        const bool mono = proc.apvts.getRawParameterValue ("L1_mono")->load() > 0.5f;
        std::vector<N> riff;
        if (mono)
            riff = { { 0, 1.5, 36 }, { 1.5, 0.5, 36 }, { 2, 1, 39 }, { 3, 1, 34 }, { 4, 2, 36 }, { 6, 1, 43 }, { 7, 1, 41 } };
        else if (p.category == "Chords" || proc.apvts.getRawParameterValue ("L1_chord")->load() > 0.5f)
            riff = { { 0, 1.5, 60 }, { 1.5, 0.5, 60 }, { 2, 2, 57 }, { 4, 1.5, 53 }, { 5.5, 0.5, 53 }, { 6, 2, 55 } };   // single keys: chord mode adds the rest
        else if (p.category == "Pad" || p.category == "FX" || p.category == "Ambient" || p.category == "Choir" || p.category == "Cinematic"
                 || p.category == "Strings" || p.category == "Brass" || p.category == "Multi")
            riff = { { 0, 4, 57 }, { 0, 4, 60 }, { 0, 4, 64 }, { 0, 4, 67 }, { 4, 4, 53 }, { 4, 4, 57 }, { 4, 4, 60 }, { 4, 4, 64 } };
        else if (p.category == "Piano")
            riff = { { 0, 1, 48 }, { 0, 2, 60 }, { 0, 2, 64 }, { 0, 2, 67 }, { 1, 0.5, 72 }, { 1.5, 0.5, 76 }, { 2, 2, 45 }, { 2, 2, 60 }, { 2, 2, 64 }, { 2, 2, 69 },
                     { 4, 2, 41 }, { 4, 2, 60 }, { 4, 2, 65 }, { 4, 2, 69 }, { 5, 0.5, 72 }, { 5.5, 0.5, 74 }, { 6, 2, 43 }, { 6, 2, 59 }, { 6, 2, 62 }, { 6, 2, 67 } };
        else if (p.category == "Pluck" || p.category == "Lead")
            riff = { { 0, 0.5, 72 }, { 0.5, 0.5, 75 }, { 1, 0.5, 79 }, { 1.5, 0.5, 82 }, { 2, 1, 80 }, { 3, 1, 79 },
                     { 4, 0.5, 72 }, { 4.5, 0.5, 75 }, { 5, 0.5, 77 }, { 5.5, 0.5, 79 }, { 6, 2, 75 } };
        else
            riff = { { 0, 2, 60 }, { 0, 2, 63 }, { 0, 2, 67 }, { 2, 2, 58 }, { 2, 2, 62 }, { 2, 2, 65 },
                     { 4, 2, 56 }, { 4, 2, 60 }, { 4, 2, 63 }, { 6, 2, 55 }, { 6, 2, 58 }, { 6, 2, 62 } };

        const int total = (int) (sr * beat * 8.0 + sr * 2.0);
        juce::AudioBuffer<float> out (2, total), buf (2, block);
        out.clear();
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = juce::jmin (block, total - pos);
            juce::MidiBuffer midi;
            for (const auto& note : riff)
            {
                const int on = (int) (note.start * beat * sr), off = (int) ((note.start + note.len) * beat * sr) - 1;
                if (on >= pos && on < pos + n)   midi.addEvent (juce::MidiMessage::noteOn (1, note.note, (juce::uint8) 110), on - pos);
                if (off >= pos && off < pos + n) midi.addEvent (juce::MidiMessage::noteOff (1, note.note), off - pos);
            }
            buf.setSize (2, n, false, false, true);
            proc.processBlock (buf, midi);
            for (int c = 0; c < 2; ++c)
                out.copyFrom (c, pos, buf, c, 0, n);
        }
        return out;
    }


    // Audio fingerprint used to find near-duplicate presets:
    // 24-band tone colour, brightness + its movement, noisiness, attack, sustain, modulation rate/depth, stereo width.
    std::vector<float> fingerprint (const juce::AudioBuffer<float>& b)
    {
        constexpr int order = 11, N = 1 << order, hop = N / 2;
        juce::dsp::FFT fft (order);
        std::vector<float> window ((size_t) N), frame ((size_t) N * 2);
        for (int i = 0; i < N; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / (N - 1));

        constexpr int nb = 24;
        std::array<float, nb + 1> edges {};
        for (int k = 0; k <= nb; ++k) edges[(size_t) k] = 40.0f * std::pow (16000.0f / 40.0f, (float) k / nb);

        const int total = b.getNumSamples();
        const auto* L = b.getReadPointer (0); const auto* R = b.getReadPointer (1);
        std::vector<float> energyDb, centroid, flatness;
        std::vector<std::array<double, nb>> bandFrames;
        double mid2 = 0, side2 = 0;
        for (int i = 0; i < total; ++i) { const double m = 0.5 * (L[i] + R[i]), s = 0.5 * (L[i] - R[i]); mid2 += m * m; side2 += s * s; }

        for (int pos = 0; pos + N <= total; pos += hop)
        {
            std::fill (frame.begin(), frame.end(), 0.0f);
            double e = 0;
            for (int i = 0; i < N; ++i) { const float m = 0.5f * (L[pos + i] + R[pos + i]); frame[(size_t) i] = m * window[(size_t) i]; e += m * m; }
            fft.performFrequencyOnlyForwardTransform (frame.data());
            std::array<double, nb> bands {};
            double num = 0, den = 0, logSum = 0, linSum = 0; int cnt = 0;
            for (int k = 1; k < N / 2; ++k)
            {
                const float f = (float) k * 44100.0f / N;
                const double pw = (double) frame[(size_t) k] * frame[(size_t) k] + 1e-12;
                if (f >= edges[0] && f < edges[nb])
                {
                    int bi = 0; while (bi < nb - 1 && f >= edges[(size_t) bi + 1]) ++bi;
                    bands[(size_t) bi] += pw;
                }
                num += f * pw; den += pw;
                if (f > 100 && f < 10000) { logSum += std::log (pw); linSum += pw; ++cnt; }
            }
            energyDb.push_back ((float) (10.0 * std::log10 (e / N + 1e-12)));
            centroid.push_back ((float) std::log2 (num / den + 1.0));
            flatness.push_back (cnt > 0 ? (float) (std::exp (logSum / cnt) / (linSum / cnt + 1e-20)) : 0.0f);
            bandFrames.push_back (bands);
        }

        const float maxDb = *std::max_element (energyDb.begin(), energyDb.end());
        std::vector<int> active;
        for (int i = 0; i < (int) energyDb.size(); ++i) if (energyDb[(size_t) i] > maxDb - 45.0f) active.push_back (i);
        if (active.empty()) active.push_back (0);

        std::vector<float> f;
        std::array<double, nb> bandSum {};
        for (int i : active) for (int k = 0; k < nb; ++k) bandSum[(size_t) k] += bandFrames[(size_t) i][(size_t) k];
        std::array<float, nb> bandDb {}; float meanDb = 0;
        for (int k = 0; k < nb; ++k) { bandDb[(size_t) k] = (float) (10.0 * std::log10 (bandSum[(size_t) k] / active.size() + 1e-12)); meanDb += bandDb[(size_t) k]; }
        meanDb /= nb;
        for (int k = 0; k < nb; ++k) f.push_back (juce::jmax (-60.0f, bandDb[(size_t) k] - meanDb));

        auto meanStd = [] (const std::vector<float>& v, float& m, float& sd) { m = 0; for (float x : v) m += x; m /= (float) v.size(); sd = 0; for (float x : v) sd += (x - m) * (x - m); sd = std::sqrt (sd / (float) v.size()); };
        std::vector<float> c, fl, en;
        for (int i : active) { c.push_back (centroid[(size_t) i]); fl.push_back (flatness[(size_t) i]); en.push_back (energyDb[(size_t) i]); }
        float cm, cs, fm, fs, em, es;
        meanStd (c, cm, cs); meanStd (fl, fm, fs); meanStd (en, em, es);
        f.push_back (cm); f.push_back (cs); f.push_back (fm); f.push_back (es);

        int peakFrame = 0; for (int i = 0; i < (int) energyDb.size(); ++i) if (energyDb[(size_t) i] >= maxDb - 1.0f) { peakFrame = i; break; }
        f.push_back (std::log (1.0f + peakFrame * (float) hop / 44100.0f * 10.0f));       // attack time
        f.push_back ((float) active.size() / (float) energyDb.size());                       // how long it sounds
        const size_t third = juce::jmax ((size_t) 1, en.size() / 3);
        float early = 0, late = 0; for (size_t i = 0; i < third; ++i) { early += en[i]; late += en[en.size() - 1 - i]; }
        f.push_back ((late - early) / (float) third);                                         // decays or sustains

        auto modulation = [&] (const std::vector<float>& v, float& rate, float& depth)
        {
            rate = 0; depth = 0;
            if (v.size() < 16) return;
            float m, sd; meanStd (v, m, sd);
            const int n = (int) v.size(); double best = 0; int bestK = 0;
            for (int k = 2; k < n / 2; ++k)
            {
                double re = 0, im = 0;
                for (int t = 0; t < n; ++t) { re += (v[(size_t) t] - m) * std::cos (2 * juce::MathConstants<double>::pi * k * t / n); im += (v[(size_t) t] - m) * std::sin (2 * juce::MathConstants<double>::pi * k * t / n); }
                const double mag = std::sqrt (re * re + im * im) / n;
                if (mag > best) { best = mag; bestK = k; }
            }
            rate = (float) bestK * 44100.0f / hop / (float) n;
            depth = (float) best;
        };
        float r1, d1, r2, d2;
        modulation (c, r1, d1); modulation (en, r2, d2);
        f.push_back (std::log (1.0f + r1)); f.push_back (d1); f.push_back (std::log (1.0f + r2)); f.push_back (d2);
        f.push_back ((float) (10.0 * std::log10 ((side2 + 1e-9) / (mid2 + 1e-9))));        // stereo width
        return f;
    }

    void writeWav (const juce::AudioBuffer<float>& b, const juce::File& f)
    {
        f.deleteFile();
        auto stream = std::unique_ptr<juce::OutputStream> (f.createOutputStream().release());
        juce::WavAudioFormat wav;
        auto opts = juce::AudioFormatWriterOptions().withSampleRate (sr).withNumChannels (2).withBitsPerSample (16);
        if (auto writer = wav.createWriterFor (stream, opts))
            writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode = argc > 1 ? argv[1] : "qc";
    const auto& presets = getFactoryPresets();

    if (mode == "qc")
    {
        const int start = argc > 2 ? juce::String (argv[2]).getIntValue() : 0;
        const int end = argc > 3 ? juce::jmin ((int) presets.size(), juce::String (argv[3]).getIntValue()) : (int) presets.size();
        for (int i = start; i < end; ++i)
        {
            auto out = renderPreset (i);
            float peak = 0.0f; bool finite = true;
            for (int c = 0; c < 2; ++c)
                for (int s = 0; s < out.getNumSamples(); ++s)
                {
                    const float v = out.getSample (c, s);
                    if (! std::isfinite (v)) finite = false;
                    peak = juce::jmax (peak, std::abs (v));
                }
            const float rms = (out.getRMSLevel (0, 0, out.getNumSamples()) + out.getRMSLevel (1, 0, out.getNumSamples())) * 0.5f;
            const bool ok = finite && peak > 0.02f && peak <= 1.0f;
            const auto& p = presets[(size_t) i];
            std::printf ("%d,%s,%s,%s,%.4f,%.5f,%d\n", i, p.category.toRawUTF8(), p.subtype.toRawUTF8(),
                         p.name.replaceCharacter (',', ' ').toRawUTF8(), peak, rms, ok ? 1 : 0);
            std::fflush (stdout);
        }
        return 0;
    }

    if (mode == "feat")
    {
        const int start = argc > 2 ? juce::String (argv[2]).getIntValue() : 0;
        const int end = argc > 3 ? juce::jmin ((int) presets.size(), juce::String (argv[3]).getIntValue()) : (int) presets.size();
        for (int i = start; i < end; ++i)
        {
            auto f = fingerprint (renderPreset (i));
            const auto& p = presets[(size_t) i];
            std::printf ("%d,%s,%s", i, p.category.toRawUTF8(), p.subtype.toRawUTF8());
            for (float v : f) std::printf (",%.4f", v);
            std::printf ("\n");
            std::fflush (stdout);
        }
        return 0;
    }

    const juce::File outDir (argc > 2 ? juce::String (argv[2]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    if (mode == "demo")
    {
        auto ids = juce::StringArray::fromTokens (argc > 3 ? argv[3] : "0", ",", "");
        int n = 0;
        for (auto& s : ids)
        {
            const int i = s.getIntValue();
            const auto& p = presets[(size_t) i];
            auto name = juce::String (++n).paddedLeft ('0', 2) + "_" + p.category + "_" + p.name;
            writeWav (renderPreset (i), outDir.getChildFile (name.replaceCharacters (" /", "__") + ".wav"));
            std::printf ("%s\n", name.toRawUTF8());
        }
        return 0;
    }

    if (mode == "shot")
    {
        NeonTitanProcessor proc;
        const int idx = argc > 3 ? juce::String (argv[3]).getIntValue() : 2;
        proc.setCurrentProgram (idx);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
        auto file = outDir.getChildFile ("NeonTitan_UI.png");
        file.deleteFile();
        juce::FileOutputStream fos (file);
        juce::PNGImageFormat().writeImageToStream (img, fos);
        std::printf ("UI snapshot %dx%d\n", img.getWidth(), img.getHeight());
        return 0;
    }
    return 1;
}
