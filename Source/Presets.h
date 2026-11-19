#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <utility>
#include <array>
#include "PresetGenerator.h"
#include "PresetKeep.h"
#include "PresetGains.h"

// A preset is either a single sound (loads into one layer) or a Multi (sets up all 4 layers).
struct LayerSlot { int preset = -1; float vol = 0.0f, pan = 0.0f; int transpose = 0; };

struct TitanPreset
{
    juce::String name, category, subtype;
    std::vector<std::pair<const char*, float>> values;   // un-prefixed parameter IDs, real values
    bool isMulti = false;
    std::array<LayerSlot, 4> layers {};
};

struct ClassicPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

struct Candidate
{
    juce::String category, subtype, adj, noun;
    std::vector<std::pair<const char*, float>> values;
};



inline std::vector<ClassicPreset> getClassicPresets()
{
    return {
        { "Init", {} },

        { "Titan Reese", {
            { "oscA_wave", 2.0f }, { "oscA_uni", 5 }, { "oscA_detune", 0.35f }, { "oscA_level", 0.8f },
            { "oscB_wave", 2.0f }, { "oscB_oct", -1 }, { "oscB_uni", 3 }, { "oscB_detune", 0.5f }, { "oscB_level", 0.6f },
            { "sub_level", 0.5f }, { "flt_cutoff", 900.0f }, { "flt_res", 0.25f }, { "flt_drive", 0.45f }, { "flt_key", 0.2f },
            { "lfo_shape", 0 }, { "lfo_rate", 0.25f }, { "lfo_cut", 0.15f }, { "lfo_retrig", 0 },
            { "amp_a", 0.005f }, { "amp_s", 1.0f }, { "amp_r", 0.2f }, { "mono", 1 }, { "glide", 0.06f },
            { "fx_dist", 0.25f }, { "fx_chorus", 0.2f }, { "fx_reverb", 0.08f } } },

        { "Neon Wobble 1/8", {
            { "oscA_wave", 2.3f }, { "oscA_uni", 3 }, { "oscA_detune", 0.2f }, { "oscB_wave", 3.0f }, { "oscB_oct", -1 }, { "oscB_level", 0.5f },
            { "sub_level", 0.6f }, { "flt_cutoff", 250.0f }, { "flt_res", 0.55f }, { "flt_drive", 0.6f },
            { "lfo_shape", 0 }, { "lfo_sync", 4 }, { "lfo_cut", 0.85f }, { "lfo_retrig", 1 },
            { "amp_s", 1.0f }, { "amp_r", 0.15f }, { "mono", 1 }, { "glide", 0.03f },
            { "fx_dist", 0.45f }, { "fx_reverb", 0.05f }, { "m_wobble", 0.0f } } },

        { "Cyber Lead", {
            { "oscA_wave", 2.0f }, { "oscA_uni", 7 }, { "oscA_detune", 0.28f }, { "oscA_spread", 1.0f },
            { "oscB_wave", 3.0f }, { "oscB_oct", 1 }, { "oscB_level", 0.25f }, { "oscB_uni", 3 },
            { "flt_cutoff", 3500.0f }, { "flt_res", 0.2f }, { "flt_env", 0.35f }, { "fenv_d", 0.6f }, { "fenv_s", 0.2f },
            { "lfo_pitch", 0.08f }, { "lfo_rate", 5.5f }, { "lfo_retrig", 0 },
            { "amp_a", 0.01f }, { "amp_s", 0.85f }, { "amp_r", 0.35f }, { "glide", 0.05f },
            { "fx_chorus", 0.25f }, { "fx_reverb", 0.3f }, { "fx_size", 0.75f }, { "fx_width", 1.3f } } },

        { "Sub 808 Glide", {
            { "oscA_wave", 0.0f }, { "oscA_level", 1.0f }, { "oscA_oct", -1 },
            { "oscB_wave", 1.0f }, { "oscB_oct", -1 }, { "oscB_level", 0.15f },
            { "flt_cutoff", 2500.0f }, { "flt_drive", 0.55f }, { "flt_key", 0.0f },
            { "amp_a", 0.002f }, { "amp_d", 1.6f }, { "amp_s", 0.0f }, { "amp_r", 0.6f },
            { "mono", 1 }, { "glide", 0.09f }, { "fx_dist", 0.3f }, { "fx_reverb", 0.0f }, { "fx_width", 0.0f } } },

        { "Growl Bass", {
            { "oscA_wave", 2.6f }, { "oscA_uni", 3 }, { "oscA_detune", 0.15f }, { "oscB_wave", 1.4f }, { "oscB_semi", 7 }, { "oscB_level", 0.4f },
            { "sub_level", 0.55f }, { "flt_type", 1 }, { "flt_cutoff", 700.0f }, { "flt_res", 0.7f }, { "flt_drive", 0.8f },
            { "lfo_shape", 1 }, { "lfo_sync", 5 }, { "lfo_cut", 0.6f }, { "lfo_retrig", 1 },
            { "amp_s", 1.0f }, { "amp_r", 0.1f }, { "mono", 1 }, { "fx_dist", 0.65f }, { "fx_reverb", 0.0f } } },

        { "Supersaw Anthem", {
            { "oscA_wave", 2.0f }, { "oscA_uni", 7 }, { "oscA_detune", 0.42f }, { "oscA_spread", 1.0f },
            { "oscB_wave", 2.0f }, { "oscB_oct", -1 }, { "oscB_uni", 7 }, { "oscB_detune", 0.38f }, { "oscB_level", 0.55f },
            { "flt_cutoff", 6000.0f }, { "flt_res", 0.1f }, { "amp_a", 0.02f }, { "amp_s", 0.9f }, { "amp_r", 0.6f },
            { "fx_chorus", 0.3f }, { "fx_reverb", 0.35f }, { "fx_size", 0.85f }, { "fx_width", 1.5f }, { "master", -9.0f } } },

        { "Night Drive Pad", {
            { "oscA_wave", 1.6f }, { "oscA_uni", 5 }, { "oscA_detune", 0.3f }, { "oscB_wave", 0.5f }, { "oscB_oct", 1 }, { "oscB_level", 0.35f },
            { "noise_level", 0.06f }, { "flt_cutoff", 1600.0f }, { "flt_res", 0.2f }, { "lfo_shape", 0 }, { "lfo_rate", 0.18f }, { "lfo_cut", 0.3f }, { "lfo_retrig", 0 },
            { "amp_a", 1.2f }, { "amp_d", 1.0f }, { "amp_s", 0.85f }, { "amp_r", 2.8f },
            { "fx_chorus", 0.45f }, { "fx_reverb", 0.6f }, { "fx_size", 0.92f }, { "fx_width", 1.6f } } },

        { "Pluck City", {
            { "oscA_wave", 2.2f }, { "oscA_uni", 3 }, { "oscA_detune", 0.18f }, { "oscB_wave", 3.0f }, { "oscB_oct", 1 }, { "oscB_level", 0.2f },
            { "flt_cutoff", 500.0f }, { "flt_res", 0.3f }, { "flt_env", 0.65f }, { "fenv_a", 0.001f }, { "fenv_d", 0.25f }, { "fenv_s", 0.0f },
            { "amp_a", 0.001f }, { "amp_d", 0.45f }, { "amp_s", 0.0f }, { "amp_r", 0.35f },
            { "fx_reverb", 0.35f }, { "fx_size", 0.7f }, { "fx_width", 1.3f } } },

        { "Acid Line", {
            { "oscA_wave", 2.0f }, { "oscB_level", 0.0f }, { "flt_cutoff", 300.0f }, { "flt_res", 0.85f }, { "flt_drive", 0.5f },
            { "flt_env", 0.7f }, { "fenv_a", 0.001f }, { "fenv_d", 0.22f }, { "fenv_s", 0.0f },
            { "amp_a", 0.001f }, { "amp_s", 0.8f }, { "amp_r", 0.08f }, { "mono", 1 }, { "glide", 0.07f }, { "fx_dist", 0.35f }, { "fx_reverb", 0.1f } } },

        { "Hyper Stab", {
            { "oscA_wave", 3.0f }, { "oscA_uni", 5 }, { "oscA_detune", 0.25f }, { "oscB_wave", 2.0f }, { "oscB_semi", 7 }, { "oscB_uni", 5 }, { "oscB_level", 0.6f },
            { "flt_cutoff", 1800.0f }, { "flt_env", 0.5f }, { "fenv_d", 0.18f }, { "fenv_s", 0.1f },
            { "amp_a", 0.001f }, { "amp_d", 0.3f }, { "amp_s", 0.0f }, { "amp_r", 0.25f },
            { "fx_dist", 0.2f }, { "fx_reverb", 0.4f }, { "fx_size", 0.8f }, { "fx_width", 1.4f } } },

        { "Riser Noise", {
            { "oscA_level", 0.2f }, { "noise_level", 0.9f }, { "flt_type", 1 }, { "flt_cutoff", 400.0f }, { "flt_res", 0.6f },
            { "flt_env", 0.9f }, { "fenv_a", 4.0f }, { "fenv_s", 1.0f },
            { "amp_a", 2.5f }, { "amp_s", 1.0f }, { "amp_r", 1.5f }, { "fx_reverb", 0.55f }, { "fx_size", 0.9f }, { "fx_width", 1.7f } } }
    };
}


// Every candidate the recipes can make, in a fixed order (identical on every computer).
inline std::vector<Candidate> buildCandidatePool()
{
    std::vector<Candidate> pool;
    const auto recipes = getRecipes();
    const auto& words = generatorWords();
    for (size_t ri = 0; ri < recipes.size(); ++ri)
    {
        const auto& rc = recipes[ri];
        const auto& w = words.at (rc.category);
        for (int i = 0; i < rc.pool; ++i)
        {
            juce::Random rnd ((juce::int64) (ri + 1) * 1000003 + i * 7919 + 777);
            Candidate c;
            c.category = rc.category;
            c.subtype = rc.subtype;
            c.values = rc.base;
            for (const auto& [id, lo, hi] : rc.ranges)
                c.values.push_back ({ id, lo + rnd.nextFloat() * (hi - lo) });
            for (const auto& [id, options] : rc.picks)
                c.values.push_back ({ id, options[(size_t) rnd.nextInt ((int) options.size())] });
            c.adj = w.adj[(size_t) rnd.nextInt ((int) w.adj.size())];
            c.noun = w.noun[(size_t) rnd.nextInt ((int) w.noun.size())];
            pool.push_back (std::move (c));
        }
    }
    return pool;
}

inline const std::vector<TitanPreset>& getFactoryPresets();

inline std::vector<TitanPreset> buildPresetLibrary()
{
    std::vector<TitanPreset> out;
    std::map<juce::String, int> used;
    auto uniqueName = [&used] (const juce::String& base)
    {
        juce::String name = base;
        for (int n = 2; used.count (name) > 0; ++n) name = base + " " + juce::String (n);
        used[name] = 1;
        return name;
    };

    for (auto& c : getClassicPresets())
    {
        TitanPreset p;
        p.name = uniqueName (c.name); p.category = "Classic"; p.subtype = "Classic"; p.values = c.values;
        out.push_back (std::move (p));
    }

    // Singles: the de-duplicated candidates (or the whole pool while building the keep list)
    const auto pool = buildCandidatePool();
    auto addSingle = [&] (const Candidate& c)
    {
        TitanPreset p;
        p.category = c.category; p.subtype = c.subtype; p.values = c.values;
        p.name = uniqueName (c.adj + " " + c.noun);
        out.push_back (std::move (p));
    };
    if (kPresetKeepCount > 0)
        for (int k = 0; k < kPresetKeepCount; ++k) addSingle (pool[(size_t) kPresetKeep[k]]);
    else
        for (const auto& c : pool) addSingle (c);

    // Multis: layered combinations of the singles above (only in the final library)
    if (kPresetKeepCount > 0)
    {
        static const std::vector<const char*> mAdj { "Colossal","Infinite","Neon","Golden","Midnight","Titan","Astral","Velvet","Thunder","Crystal","Shadow","Aurora","Phantom","Solar","Hyper","Sacred","Iron","Cosmic","Royal","Frozen" };
        static const std::vector<const char*> mNoun { "Stack","Layers","Machine","Kingdom","Horizon","Empire","Engine","Odyssey","Cathedral","Skyline","Universe","Monument","Fusion","Realm","Saga","World" };
        const auto multis = getMultiRecipes();
        const size_t numSingles = out.size();
        for (size_t mi = 0; mi < multis.size(); ++mi)
        {
            const auto& mr = multis[mi];
            for (int i = 0; i < mr.count; ++i)
            {
                juce::Random rnd ((juce::int64) (mi + 1) * 99991 + i * 613 + 31337);
                TitanPreset p;
                p.category = "Multi"; p.subtype = mr.subtype; p.isMulti = true;
                p.name = uniqueName (juce::String (mr.name) + " " + mAdj[(size_t) rnd.nextInt ((int) mAdj.size())] + " " + mNoun[(size_t) rnd.nextInt ((int) mNoun.size())]);
                for (size_t l = 0; l < mr.layers.size() && l < 4; ++l)
                {
                    const auto& spec = mr.layers[l];
                    std::vector<int> matches;
                    for (size_t s = 12; s < numSingles; ++s)
                        if (out[s].category == spec.category && (juce::String (spec.subtype).isEmpty() || out[s].subtype == spec.subtype))
                            matches.push_back ((int) s);
                    if (matches.empty()) continue;
                    p.layers[l] = { matches[(size_t) rnd.nextInt ((int) matches.size())], spec.vol + rnd.nextFloat() * 2.0f - 1.0f, spec.pan, spec.transpose };
                }
                out.push_back (std::move (p));
            }
        }
    }

    // Loudness match (measured by Tools/Render.cpp + Tools/make_gains.py)
    for (size_t i = 0; i < out.size() && i < (size_t) kPresetGainCount; ++i)
    {
        auto& p = out[i];
        const float g = kPresetGainDb[i];
        if (p.isMulti)
            for (auto& l : p.layers) l.vol = juce::jlimit (-36.0f, 6.0f, l.vol + g);
        else
        {
            bool found = false;
            for (auto& [id, v] : p.values)
                if (juce::String (id) == "master") { v = juce::jlimit (-24.0f, 6.0f, v + g); found = true; }
            if (! found && g != 0.0f) p.values.push_back ({ "master", juce::jlimit (-24.0f, 6.0f, -6.0f + g) });
        }
    }
    return out;
}

inline const std::vector<TitanPreset>& getFactoryPresets()
{
    static const std::vector<TitanPreset> library = buildPresetLibrary();
    return library;
}

inline const juce::StringArray& getPresetCategories()
{
    static const juce::StringArray cats { "Multi", "Strings", "Brass", "Choir", "Cinematic", "Ambient", "Pad", "Piano", "Keys", "Chords",
                                          "Lead", "Pluck", "Bass", "808", "FX", "Classic" };
    return cats;
}
