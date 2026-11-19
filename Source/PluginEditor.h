#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

// Stereo level meter for the master output
class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (NeonTitanProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    NeonTitanProcessor& proc;
};

class NeonTitanEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NeonTitanEditor (NeonTitanProcessor&);
    ~NeonTitanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob   { juce::String id; std::unique_ptr<juce::Slider> slider; std::unique_ptr<juce::Label> label; std::unique_ptr<SliderAttach> attach; bool big = false; };
    struct Choice { juce::String id; std::unique_ptr<juce::ComboBox> box; std::unique_ptr<juce::Label> label; std::unique_ptr<ComboAttach> attach; };
    struct Toggle { juce::String id; std::unique_ptr<juce::ToggleButton> button; std::unique_ptr<ButtonAttach> attach; };
    struct Item { int kind; int index; };   // 0 knob, 1 choice, 2 toggle
    struct Section { juce::String title; juce::Colour colour; std::vector<Item> items; juce::Rectangle<int> bounds; };

    void addKnob (Section&, const juce::String& id, const juce::String& text, bool big = false);
    void addChoice (Section&, const juce::String& id, const juce::String& text, const juce::StringArray& items);
    void addToggle (Section&, const juce::String& id, const juce::String& text);
    void selectLayer (int layer);
    void attachLayerControls();
    void timerCallback() override;
    void refreshPresetBox();
    void rebuildPresetList();
    void stepPreset (int delta);
    void loadPreset (int index);
    void refreshLayerTabs();

    NeonTitanProcessor& proc;
    NeonLookAndFeel lnf;
    std::vector<Knob> knobs;
    std::vector<Choice> choices;
    std::vector<Toggle> toggles;
    std::vector<Section> sections;

    // Layer strip
    std::array<juce::TextButton, 4> layerTabs;
    std::array<juce::ToggleButton, 4> layerPower;
    std::array<std::unique_ptr<ButtonAttach>, 4> powerAttach;
    juce::Slider layerVol, layerPan, layerTranspose;
    juce::Label layerVolLabel { {}, "LAYER VOL" }, layerPanLabel { {}, "PAN" }, layerTransLabel { {}, "TRANSPOSE" };
    std::unique_ptr<SliderAttach> volAttach, panAttach, transAttach;
    juce::Rectangle<int> stripBounds;

    // Master
    juce::Slider masterVol;
    juce::Label masterLabel { {}, "MASTER" };
    std::unique_ptr<SliderAttach> masterAttach;
    LevelMeter meter;

    // Browser
    juce::ComboBox categoryBox, presetBox;
    juce::TextEditor searchBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, randomButton { "RANDOM" };
    std::vector<int> filtered;
    juce::Random random;

    juce::MidiKeyboardComponent keyboard;
    int shownProgram = -2, shownLayer = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonTitanEditor)
};
