# NEON TITAN

An original, big-sounding synth plugin (VST3) for Maschine 2 and any other plugin host. Built with JUCE.

**Sound engine (v2.0)**
- **4 layers**, each a complete synth with its own preset, ON button, volume, pan and transpose, plus a **master volume** and level meter
- 2 oscillators per layer with 8 wave shapes (sine, triangle, saw, square, pulse, vocal, organ, hollow), up to 7 unison voices, detune and stereo spread
- FM (osc B bends osc A) and ring modulation for bells, electric pianos and metallic tones
- Filter: low pass, band pass, high pass, notch and **vowel** (A-E-I-O-U, for talking basses and choirs), with drive, envelope, key tracking and velocity
- LFO in Hz or synced to Maschine's tempo, with fade-in for natural string and vocal vibrato
- Mono/glide, pitch envelope (808 punch), hammer click, and chord mode (one key or pad plays a full chord)
- FX per layer: distortion, squash (OTT-style), bitcrusher, chorus, phaser, synced ping-pong delay, reverb, width
- 4 macro knobs per layer (SIZE, GROWL, WOBBLE, BRIGHT)

**2,888 presets, all different from each other**

| Category | Count | Category | Count |
|---|---|---|---|
| Multi (4-layer stacks) | 275 | Bass | 385 |
| Strings | 178 | Lead | 247 |
| Brass | 119 | Pluck & Bell | 259 |
| Choir | 112 | Keys | 143 |
| Cinematic | 151 | Chords | 197 |
| Ambient | 306 | Piano | 111 |
| Pad | 225 | 808 | 55 |
| FX | 113 | Classic | 12 |

### How the library was made (and why it's smaller than v1)
1. **Recipes** (`Source/PresetGenerator.h`) describe ~70 sound styles with wide ranges: different wave shapes, filter types, FM, ring mod and effects, not just knob nudges.
2. They generate **7,350 candidates**.
3. Every candidate is rendered and given an **audio fingerprint** (24-band tone colour, brightness and its movement, noisiness, attack, sustain, modulation rate and depth, stereo width).
4. `Tools/dedupe.py` keeps a sound only if it's clearly different from every sound already kept in its category. **2,601** passed; the rest were near-copies. The list is `Source/PresetKeep.h`.
5. **Multis** combine those sounds across 4 layers.
6. Every preset was checked for clean audio and volume-matched to its category.

808s came out lowest (55) because an 808 is essentially a sine wave with a pitch drop, so most variations genuinely sound alike.

## Putting it on another computer
The plug-in is a single folder, and all 6,212 presets are built into it, so there's nothing else to copy.

**Another Windows PC:** copy `NEON TITAN.vst3` (from your download, or from `C:\Program Files\Common Files\VST3\` on the first PC) to a USB stick or cloud drive, then paste it into `C:\Program Files\Common Files\VST3\` on the new PC. Open Maschine (updated) and rescan plug-ins.

**A Mac:** the GitHub build also makes a Mac version. Download **NEON-TITAN-Mac** from the same Actions run and follow `INSTALL_MAC.txt`.

**Your own sounds:** sounds you tweak and save in Maschine live in Maschine's User Content folder (Documents > Native Instruments > User Content) and inside your Maschine projects. Copy those too, and they'll load on the new computer once NEON TITAN is installed there.

## Maschine compatibility (read first)
Older Maschine 2 versions only load third-party plugins in the old VST2 format, which can no longer be built. Third-party **VST3** support came in later Maschine 2 updates. Update Maschine 2 to the latest version in Native Access (or use Maschine 3) before installing.

---

## Route 1: Let GitHub build it for you (no coding tools)
1. Create a free account at github.com.
2. Click **+ → New repository**, name it `neon-titan`, and create it.
3. Click **uploading an existing file**, drag in everything from this folder **including the hidden `.github` folder**, and commit.
   (If the `.github` folder doesn't upload by dragging, create the file `.github/workflows/build.yml` with **Add file → Create new file** and paste its contents.)
4. Open the **Actions** tab. The "Build NEON TITAN" workflow starts automatically and takes about 10–15 minutes.
5. When it shows a green check, open the run and download **NEON-TITAN-Windows** at the bottom.
6. Unzip it and follow `INSTALL_WINDOWS.txt`.

Every time you change the code and commit, GitHub builds a fresh copy.

## Route 2: Compile it yourself on Windows
Install once:
- **Visual Studio 2022 Community** with the "Desktop development with C++" workload
- **CMake** (cmake.org, tick "Add to PATH")
- **Git** (git-scm.com)

Then in a terminal, inside this folder:
```bat
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```
The first run downloads JUCE automatically. The plugin appears in:
`build\NeonTitan_artefacts\Release\VST3\NEON TITAN.vst3`

Copy that folder to `C:\Program Files\Common Files\VST3\`, then rescan plugins in Maschine.

## Route 3: Source code map
| File | What it does |
|---|---|
| `Source/Params.h` | Every knob: name, range and default |
| `Source/TitanVoice.h` | The voice: oscillators, wave shapes, FM, ring mod, filters incl. vowel, envelopes, LFO, glide |
| `Source/TitanEngine.h` | One layer: 16 voices + its effects chain + chord mode |
| `Source/PresetKeep.h` | Which generated candidates passed the sound-alike check |
| `Tools/dedupe.py` | The audio de-duplication |
| `Source/PluginProcessor.cpp` | Reads the knobs, macros, tempo sync, FX chain, presets, saving/loading |
| `Source/Presets.h` | The 12 classic presets and the library builder |
| `Source/PresetGenerator.h` | The recipes and name words behind the 5,000 generated presets. Add a recipe to make more |
| `Source/PresetGains.h` | Per-preset loudness matching |
| `Source/PluginEditor.cpp` | The neon interface and on-screen keyboard |
| `Tools/Render.cpp` | Test tool: checks every preset (`qc`), renders WAV previews (`demo`), saves a screenshot (`shot`) |
| `Tools/make_gains.py` | Builds the loudness-matching table from a `qc` run |
| `.github/workflows/build.yml` | Cloud build for Windows |

## Using it in Maschine
- **Layers:** click a LAYER tab to choose which layer the knobs and browser edit. Pick a sound for it, then click another tab and pick a second sound to stack. Use each layer's ON button, LAYER VOL, PAN and TRANSPOSE to balance them, and MASTER for the overall level.
- **Presets:** pick a category, then a sound, or search (try "strings", "choir", "braam", "talking", "bell"). Singles load into the selected layer; **Multi** presets set up all 4 layers at once.
- **Hardware knobs:** in Maschine's plugin parameter pages, assign the 4 macros first. They're the most fun to play live.
- **Wobble bass:** load *Neon Wobble 1/8*, play low notes, turn **WOBBLE**. Change LFO **SYNC** to 1/16 or 1/4 T for different rhythms.
- **808s:** load *Sub 808 Glide*. Overlap notes to hear the glide; turn **GLIDE** for longer slides.

## Notes
- Windows may warn that the plugin is from an unknown publisher because it isn't code-signed. That's normal for a self-built plugin.
- JUCE and the VST3 SDK have their own licenses. They're fine for personal use; check both before selling the plugin.
