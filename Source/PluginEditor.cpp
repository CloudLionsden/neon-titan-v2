#include "PluginEditor.h"
#include "Params.h"
#include "Presets.h"

namespace Neon
{
    const juce::Colour bg      { 0xff07040f };
    const juce::Colour panel   { 0xff120c22 };
    const juce::Colour text    { 0xffe8e6f0 };
    const juce::Colour dim     { 0xff8a84a3 };
    const juce::Colour cyan    { 0xff00f0ff };
    const juce::Colour magenta { 0xffff2bd6 };
    const juce::Colour purple  { 0xffb026ff };
    const juce::Colour yellow  { 0xffffe600 };
    const juce::Colour green   { 0xff39ff88 };
    const juce::Colour orange  { 0xffff7a3d };
}

// ---------------- Look and feel ----------------
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, Neon::panel.brighter (0.15f));
    setColour (juce::ComboBox::textColourId, Neon::text);
    setColour (juce::ComboBox::outlineColourId, Neon::cyan.withAlpha (0.5f));
    setColour (juce::ComboBox::arrowColourId, Neon::cyan);
    setColour (juce::PopupMenu::backgroundColourId, Neon::panel);
    setColour (juce::PopupMenu::textColourId, Neon::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Neon::magenta.withAlpha (0.5f));
    setColour (juce::TextButton::buttonColourId, Neon::panel.brighter (0.15f));
    setColour (juce::TextButton::textColourOffId, Neon::cyan);
    setColour (juce::Label::textColourId, Neon::dim);
    setColour (juce::BubbleComponent::backgroundColourId, Neon::panel);
    setColour (juce::BubbleComponent::outlineColourId, Neon::magenta);
    setColour (juce::TooltipWindow::textColourId, Neon::text);
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto colour = s.findColour (juce::Slider::rotarySliderFillColourId);
    const float thick = juce::jmax (2.5f, radius * 0.16f);

    // body
    g.setColour (juce::Colour (0xff1a1230));
    g.fillEllipse (juce::Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre));
    g.setColour (juce::Colour (0xff2a2245));
    g.drawEllipse (juce::Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre), 1.0f);

    // track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2245));
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc with glow
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (colour.withAlpha (0.25f));
    g.strokePath (arc, juce::PathStrokeType (thick * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colour);
    g.strokePath (arc, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // pointer
    const auto tip = centre.getPointOnCircumference (radius * 0.62f, angle);
    g.setColour (Neon::text);
    g.drawLine ({ centre.getPointOnCircumference (radius * 0.2f, angle), tip }, 2.0f);
}

void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (4.0f, 8.0f);
    const bool on = b.getToggleState();
    const auto c = Neon::magenta;
    if (on)
    {
        g.setColour (c.withAlpha (0.25f));
        g.fillRoundedRectangle (r.expanded (2.0f), 8.0f);
        g.setColour (c);
        g.fillRoundedRectangle (r, 6.0f);
    }
    else
    {
        g.setColour (Neon::panel.brighter (0.2f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (c.withAlpha (highlighted ? 0.9f : 0.5f));
        g.drawRoundedRectangle (r, 6.0f, 1.2f);
    }
    g.setColour (on ? juce::Colours::black : Neon::text);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

// ---------------- Level meter ----------------
void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1230));
    g.fillRoundedRectangle (r, 3.0f);
    const float lv[2] = { proc.meterL.load(), proc.meterR.load() };
    for (int c = 0; c < 2; ++c)
    {
        auto bar = r.reduced (2.0f).withHeight ((r.getHeight() - 6.0f) / 2.0f).withY (r.getY() + 2.0f + c * (r.getHeight() - 2.0f) / 2.0f);
        const float db = juce::Decibels::gainToDecibels (lv[c], -48.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        juce::ColourGradient cg (Neon::cyan, bar.getX(), 0, Neon::magenta, bar.getRight(), 0, false);
        cg.addColour (0.85, Neon::yellow);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * frac), 2.0f);
    }
}

// ---------------- Editor ----------------
static constexpr int knobW = 64, bigW = 92, choiceW = 100, toggleW = 78, rowH = 128, headerH = 58, stripH = 74, keysH = 64, margin = 10, editorW = 1340;

NeonTitanEditor::NeonTitanEditor (NeonTitanProcessor& p)
    : AudioProcessorEditor (&p), proc (p), meter (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    knobs.reserve (96); choices.reserve (12); toggles.reserve (6);

    auto makeOsc = [this] (const juce::String& id, const juce::String& title, juce::Colour c, bool withMod)
    {
        Section s { title, c, {}, {} };
        addKnob (s, id + "_wave", "WAVE"); addKnob (s, id + "_oct", "OCT"); addKnob (s, id + "_semi", "SEMI"); addKnob (s, id + "_fine", "FINE");
        addKnob (s, id + "_level", "LEVEL"); addKnob (s, id + "_uni", "UNISON"); addKnob (s, id + "_detune", "DETUNE"); addKnob (s, id + "_spread", "SPREAD");
        if (withMod) { addKnob (s, "fm", "FM B>A"); addKnob (s, "ring", "RING"); }
        sections.push_back (std::move (s));
    };
    makeOsc ("oscA", "OSC A", Neon::cyan, false);
    makeOsc ("oscB", "OSC B  (also modulates A: FM / RING)", Neon::magenta, true);

    { Section s { "SUB + NOISE", Neon::purple, {}, {} };
      addKnob (s, "sub_level", "SUB"); addChoice (s, "sub_oct", "SUB OCT", Params::subOctaves); addKnob (s, "noise_level", "NOISE"); addKnob (s, "click", "CLICK");
      sections.push_back (std::move (s)); }
    { Section s { "FILTER", Neon::yellow, {}, {} };
      addChoice (s, "flt_type", "TYPE", Params::filterTypes);
      addKnob (s, "flt_cutoff", "CUTOFF"); addKnob (s, "flt_res", "RES"); addKnob (s, "flt_drive", "DRIVE"); addKnob (s, "flt_env", "ENV AMT");
      addKnob (s, "flt_key", "KEY TRK"); addKnob (s, "vel_cut", "VEL>CUT"); addKnob (s, "vowel", "VOWEL");
      sections.push_back (std::move (s)); }
    { Section s { "FILTER ENV", Neon::yellow, {}, {} };
      addKnob (s, "fenv_a", "ATTACK"); addKnob (s, "fenv_d", "DECAY"); addKnob (s, "fenv_s", "SUSTAIN"); addKnob (s, "fenv_r", "RELEASE");
      sections.push_back (std::move (s)); }
    { Section s { "AMP ENV", Neon::green, {}, {} };
      addKnob (s, "amp_a", "ATTACK"); addKnob (s, "amp_d", "DECAY"); addKnob (s, "amp_s", "SUSTAIN"); addKnob (s, "amp_r", "RELEASE");
      sections.push_back (std::move (s)); }
    { Section s { "LFO", Neon::orange, {}, {} };
      addChoice (s, "lfo_shape", "SHAPE", Params::lfoShapes); addChoice (s, "lfo_sync", "SYNC", Params::lfoSyncs);
      addKnob (s, "lfo_rate", "RATE"); addKnob (s, "lfo_cut", "> CUTOFF"); addKnob (s, "lfo_pitch", "> PITCH"); addKnob (s, "lfo_amp", "> VOLUME");
      addKnob (s, "lfo_fade", "FADE IN"); addToggle (s, "lfo_retrig", "RETRIG");
      sections.push_back (std::move (s)); }
    { Section s { "VOICE", Neon::purple, {}, {} };
      addToggle (s, "mono", "MONO"); addChoice (s, "chord", "CHORD", Params::chordTypes); addKnob (s, "glide", "GLIDE");
      addKnob (s, "pitch_env", "P.ENV"); addKnob (s, "pitch_dec", "P.DECAY");
      sections.push_back (std::move (s)); }
    { Section s { "FX", Neon::cyan, {}, {} };
      addKnob (s, "fx_dist", "DISTORT"); addKnob (s, "fx_squash", "SQUASH"); addKnob (s, "fx_crush", "CRUSH"); addKnob (s, "fx_chorus", "CHORUS");
      addKnob (s, "fx_phaser", "PHASER"); addKnob (s, "dly_mix", "DELAY"); addChoice (s, "dly_time", "D.TIME", Params::delayTimes); addKnob (s, "dly_fb", "FEEDBK");
      addKnob (s, "fx_reverb", "REVERB"); addKnob (s, "fx_size", "SIZE"); addKnob (s, "fx_width", "WIDTH"); addKnob (s, "master", "OUTPUT");
      sections.push_back (std::move (s)); }
    { Section s { "MACROS", Neon::magenta, {}, {} };
      addKnob (s, "m_size", "SIZE", true); addKnob (s, "m_growl", "GROWL", true); addKnob (s, "m_wobble", "WOBBLE", true); addKnob (s, "m_bright", "BRIGHT", true);
      sections.push_back (std::move (s)); }

    // ---- Layer strip ----
    static const juce::Colour layerColours[4] = { Neon::cyan, Neon::magenta, Neon::yellow, Neon::green };
    for (int l = 0; l < 4; ++l)
    {
        auto& tab = layerTabs[(size_t) l];
        tab.setClickingTogglesState (false);
        tab.setColour (juce::TextButton::textColourOffId, layerColours[l]);
        tab.onClick = [this, l] { selectLayer (l); };
        addAndMakeVisible (tab);
        auto& pw = layerPower[(size_t) l];
        pw.setButtonText ("ON");
        addAndMakeVisible (pw);
        powerAttach[(size_t) l] = std::make_unique<ButtonAttach> (proc.apvts, Params::layerPrefix (l) + "on", pw);
    }
    for (auto* s : { &layerVol, &layerPan, &layerTranspose, &masterVol })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->setPopupDisplayEnabled (true, true, this);
        s->setColour (juce::Slider::rotarySliderFillColourId, Neon::cyan);
        addAndMakeVisible (s);
    }
    masterVol.setColour (juce::Slider::rotarySliderFillColourId, Neon::magenta);
    masterAttach = std::make_unique<SliderAttach> (proc.apvts, "master_vol", masterVol);
    for (auto* lb : { &layerVolLabel, &layerPanLabel, &layerTransLabel, &masterLabel })
    {
        lb->setJustificationType (juce::Justification::centred);
        lb->setFont (juce::FontOptions (10.5f, juce::Font::bold));
        addAndMakeVisible (lb);
    }
    masterLabel.setColour (juce::Label::textColourId, Neon::magenta);
    addAndMakeVisible (meter);

    // ---- Browser ----
    categoryBox.addItemList (getPresetCategories(), 1);
    categoryBox.onChange = [this] { searchBox.clear(); rebuildPresetList(); };
    searchBox.setTextToShowWhenEmpty ("search sounds", Neon::dim);
    searchBox.setColour (juce::TextEditor::backgroundColourId, Neon::panel.brighter (0.15f));
    searchBox.setColour (juce::TextEditor::textColourId, Neon::text);
    searchBox.setColour (juce::TextEditor::outlineColourId, Neon::cyan.withAlpha (0.5f));
    searchBox.setColour (juce::TextEditor::focusedOutlineColourId, Neon::magenta);
    searchBox.setJustification (juce::Justification::centredLeft);
    searchBox.setIndents (8, 0);
    searchBox.onTextChange = [this] { rebuildPresetList(); };
    presetBox.onChange = [this] { const int idx = presetBox.getSelectedId() - 1; if (idx >= 0) loadPreset (idx); };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    randomButton.onClick = [this] { if (! filtered.empty()) loadPreset (filtered[(size_t) random.nextInt ((int) filtered.size())]); };
    randomButton.setColour (juce::TextButton::textColourOffId, Neon::magenta);
    for (auto* comp : std::initializer_list<juce::Component*> { &categoryBox, &searchBox, &presetBox, &prevButton, &nextButton, &randomButton })
        addAndMakeVisible (comp);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xff1c1530));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff05030a));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff3a2f5a));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Neon::magenta.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Neon::cyan.withAlpha (0.3f));
    keyboard.setAvailableRange (24, 96);
    addAndMakeVisible (keyboard);

    selectLayer (proc.selectedLayer.load());
    setSize (editorW, headerH + stripH + rowH * 4 + keysH + margin * 3);
    startTimerHz (20);
}

NeonTitanEditor::~NeonTitanEditor()
{
    stopTimer();
    for (auto& k : knobs) k.attach.reset();
    for (auto& c : choices) c.attach.reset();
    for (auto& t : toggles) t.attach.reset();
    setLookAndFeel (nullptr);
}

void NeonTitanEditor::addKnob (Section& s, const juce::String& id, const juce::String& text, bool big)
{
    Knob k;
    k.id = id; k.big = big;
    k.slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    k.slider->setColour (juce::Slider::rotarySliderFillColourId, s.colour);
    k.slider->setPopupDisplayEnabled (true, true, this);
    k.label = std::make_unique<juce::Label> (id, text);
    k.label->setJustificationType (juce::Justification::centred);
    k.label->setFont (juce::FontOptions (big ? 13.0f : 11.0f, juce::Font::bold));
    k.label->setColour (juce::Label::textColourId, big ? s.colour : Neon::dim);
    addAndMakeVisible (*k.slider);
    addAndMakeVisible (*k.label);
    knobs.push_back (std::move (k));
    s.items.push_back ({ 0, (int) knobs.size() - 1 });
}

void NeonTitanEditor::addChoice (Section& s, const juce::String& id, const juce::String& text, const juce::StringArray& items)
{
    Choice c;
    c.id = id;
    c.box = std::make_unique<juce::ComboBox>();
    c.box->addItemList (items, 1);
    c.label = std::make_unique<juce::Label> (id, text);
    c.label->setJustificationType (juce::Justification::centred);
    c.label->setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (*c.box);
    addAndMakeVisible (*c.label);
    choices.push_back (std::move (c));
    s.items.push_back ({ 1, (int) choices.size() - 1 });
}

void NeonTitanEditor::addToggle (Section& s, const juce::String& id, const juce::String& text)
{
    Toggle t;
    t.id = id;
    t.button = std::make_unique<juce::ToggleButton> (text);
    addAndMakeVisible (*t.button);
    toggles.push_back (std::move (t));
    s.items.push_back ({ 2, (int) toggles.size() - 1 });
}

// Every knob shows the SELECTED layer: switching layers re-points all controls at that layer's parameters.
void NeonTitanEditor::attachLayerControls()
{
    const auto prefix = Params::layerPrefix (shownLayer);
    for (auto& k : knobs)
    {
        k.attach.reset();
        const auto id = prefix + k.id;
        if (auto* p = proc.apvts.getParameter (id))
            k.slider->setDoubleClickReturnValue (true, (double) proc.apvts.getParameterRange (id).convertFrom0to1 (p->getDefaultValue()));
        k.attach = std::make_unique<SliderAttach> (proc.apvts, id, *k.slider);
    }
    for (auto& c : choices) { c.attach.reset(); c.attach = std::make_unique<ComboAttach> (proc.apvts, prefix + c.id, *c.box); }
    for (auto& t : toggles) { t.attach.reset(); t.attach = std::make_unique<ButtonAttach> (proc.apvts, prefix + t.id, *t.button); }
    volAttach.reset(); panAttach.reset(); transAttach.reset();
    volAttach   = std::make_unique<SliderAttach> (proc.apvts, prefix + "vol", layerVol);
    panAttach   = std::make_unique<SliderAttach> (proc.apvts, prefix + "pan", layerPan);
    transAttach = std::make_unique<SliderAttach> (proc.apvts, prefix + "transpose", layerTranspose);
}

void NeonTitanEditor::selectLayer (int layer)
{
    layer = juce::jlimit (0, 3, layer);
    proc.selectedLayer.store (layer);
    if (layer != shownLayer)
    {
        shownLayer = layer;
        attachLayerControls();
    }
    refreshLayerTabs();
    refreshPresetBox();
    repaint();
}

void NeonTitanEditor::refreshLayerTabs()
{
    const auto& presets = getFactoryPresets();
    for (int l = 0; l < 4; ++l)
    {
        const int p = proc.getLayerPreset (l);
        const juce::String name = juce::isPositiveAndBelow (p, (int) presets.size()) ? presets[(size_t) p].name : juce::String ("(empty)");
        layerTabs[(size_t) l].setButtonText ("LAYER " + juce::String (l + 1) + ":  " + name);
        layerTabs[(size_t) l].setToggleState (l == shownLayer, juce::dontSendNotification);
        layerTabs[(size_t) l].setColour (juce::TextButton::buttonColourId, l == shownLayer ? Neon::panel.brighter (0.45f) : Neon::panel.brighter (0.1f));
    }
}

void NeonTitanEditor::loadPreset (int index)
{
    proc.loadPresetIntoLayer (index, shownLayer);
    refreshLayerTabs();
    refreshPresetBox();
}

void NeonTitanEditor::rebuildPresetList()
{
    const auto& presets = getFactoryPresets();
    const auto query = searchBox.getText().trim().toLowerCase();
    const auto category = categoryBox.getText();
    filtered.clear();
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        const auto& p = presets[(size_t) i];
        const bool match = query.isEmpty() ? p.category == category
                                           : (p.name + " " + p.subtype + " " + p.category).toLowerCase().contains (query);
        if (! match) continue;
        filtered.push_back (i);
        presetBox.addItem (p.name + "   (" + (query.isEmpty() ? p.subtype : p.category + " / " + p.subtype) + ")", i + 1);
        if (filtered.size() >= 1500) break;
    }
    presetBox.setTextWhenNothingSelected (juce::String ((int) filtered.size()) + " sounds");
    presetBox.setSelectedId (proc.getLayerPreset (shownLayer) + 1, juce::dontSendNotification);
}

void NeonTitanEditor::stepPreset (int delta)
{
    if (filtered.empty()) return;
    const auto it = std::find (filtered.begin(), filtered.end(), proc.getLayerPreset (shownLayer));
    int pos = it == filtered.end() ? (delta > 0 ? -1 : 0) : (int) std::distance (filtered.begin(), it);
    pos = (pos + delta + (int) filtered.size()) % (int) filtered.size();
    loadPreset (filtered[(size_t) pos]);
}

void NeonTitanEditor::refreshPresetBox()
{
    shownProgram = proc.getLayerPreset (shownLayer);
    const auto& presets = getFactoryPresets();
    if (searchBox.isEmpty() && juce::isPositiveAndBelow (shownProgram, (int) presets.size()))
        categoryBox.setSelectedId (getPresetCategories().indexOf (presets[(size_t) shownProgram].category) + 1, juce::dontSendNotification);
    else if (categoryBox.getSelectedId() == 0)
        categoryBox.setSelectedId (1, juce::dontSendNotification);
    rebuildPresetList();
}

void NeonTitanEditor::timerCallback()
{
    meter.repaint();
    if (proc.selectedLayer.load() != shownLayer)
        selectLayer (proc.selectedLayer.load());
    else if (proc.getLayerPreset (shownLayer) != shownProgram)
    {
        refreshLayerTabs();
        refreshPresetBox();
    }
}

void NeonTitanEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bgGrad (juce::Colour (0xff0b0518), 0, 0, juce::Colour (0xff04020a), 0, (float) getHeight(), false);
    g.setGradientFill (bgGrad);
    g.fillAll();
    g.setColour (Neon::purple.withAlpha (0.05f));
    for (int x = 0; x < getWidth(); x += 24) g.drawVerticalLine (x, 0.0f, (float) getHeight());
    for (int y = 0; y < getHeight(); y += 24) g.drawHorizontalLine (y, 0.0f, (float) getWidth());

    auto title = juce::Rectangle<int> (margin + 4, 8, 260, 40);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    juce::ColourGradient tg (Neon::magenta, (float) title.getX(), 0, Neon::cyan, (float) title.getRight() - 60, 0, false);
    g.setGradientFill (tg);
    g.drawText ("NEON TITAN", title, juce::Justification::centredLeft);
    g.setColour (Neon::dim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("v2.0  //  4-layer", title.withX (title.getX() + 196).withWidth (110), juce::Justification::centredLeft);

    // Layer strip panel
    static const juce::Colour layerColours[4] = { Neon::cyan, Neon::magenta, Neon::yellow, Neon::green };
    g.setColour (Neon::panel.withAlpha (0.92f));
    g.fillRoundedRectangle (stripBounds.toFloat(), 10.0f);
    g.setColour (layerColours[juce::jlimit (0, 3, shownLayer)].withAlpha (0.8f));
    g.drawRoundedRectangle (stripBounds.toFloat(), 10.0f, 1.4f);

    for (const auto& s : sections)
    {
        const auto r = s.bounds.toFloat();
        g.setColour (Neon::panel.withAlpha (0.92f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (s.colour.withAlpha (0.12f));
        g.drawRoundedRectangle (r.expanded (1.5f), 11.0f, 3.0f);
        g.setColour (s.colour.withAlpha (0.75f));
        g.drawRoundedRectangle (r, 10.0f, 1.2f);
        g.setColour (s.colour);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (s.title, s.bounds.reduced (12, 6).removeFromTop (16), juce::Justification::centredLeft);
    }
    // Label telling you which layer the knobs are editing
    g.setColour (layerColours[juce::jlimit (0, 3, shownLayer)]);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("EDITING LAYER " + juce::String (shownLayer + 1), stripBounds.withTrimmedLeft (stripBounds.getWidth() - 140).withHeight (16).translated (-12, 4), juce::Justification::centredRight);
}

void NeonTitanEditor::resized()
{
    // Header: browser on the left of centre, master + meter on the right
    auto header = getLocalBounds().removeFromTop (headerH).reduced (margin, 10);
    header.removeFromLeft (310);
    auto masterArea = header.removeFromRight (190);
    masterVol.setBounds (masterArea.removeFromLeft (44));
    masterLabel.setBounds (masterArea.removeFromLeft (52));
    meter.setBounds (masterArea.reduced (2, 8));
    header.removeFromRight (10);
    randomButton.setBounds (header.removeFromRight (74));
    header.removeFromRight (6);
    nextButton.setBounds (header.removeFromRight (34));
    header.removeFromRight (6);
    categoryBox.setBounds (header.removeFromLeft (120));
    header.removeFromLeft (6);
    searchBox.setBounds (header.removeFromLeft (150));
    header.removeFromLeft (6);
    prevButton.setBounds (header.removeFromLeft (34));
    header.removeFromLeft (6);
    presetBox.setBounds (header);

    // Layer strip
    stripBounds = juce::Rectangle<int> (margin, headerH, getWidth() - margin * 2, stripH - margin);
    auto strip = stripBounds.reduced (10, 8);
    auto knobsArea = strip.removeFromRight (330).withTrimmedTop (14);
    for (auto [s, lb] : { std::pair<juce::Slider*, juce::Label*> { &layerVol, &layerVolLabel }, { &layerPan, &layerPanLabel }, { &layerTranspose, &layerTransLabel } })
    {
        auto cell = knobsArea.removeFromLeft (100);
        lb->setBounds (cell.removeFromRight (56));
        s->setBounds (cell);
    }
    strip.removeFromRight (10);
    const int tabW = strip.getWidth() / 4;
    for (int l = 0; l < 4; ++l)
    {
        auto cell = strip.removeFromLeft (tabW).reduced (4, 2);
        layerPower[(size_t) l].setBounds (cell.removeFromLeft (52));
        layerTabs[(size_t) l].setBounds (cell);
    }

    keyboard.setBounds (getLocalBounds().removeFromBottom (keysH + margin).reduced (margin, 0).withTrimmedBottom (margin));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 43.0f);
    keyboard.setLowestVisibleKey (24);

    auto itemWidth = [this] (const Item& it) { return it.kind == 0 ? (knobs[(size_t) it.index].big ? bigW : knobW) : it.kind == 1 ? choiceW : toggleW; };
    const std::vector<std::vector<int>> rows { { 0, 1 }, { 2, 3, 4 }, { 5, 6, 7 }, { 8, 9 } };
    const int available = getWidth() - margin * 2;
    int y = headerH + stripH;
    for (const auto& row : rows)
    {
        std::vector<int> natural;
        int total = 0;
        for (int si : row)
        {
            int w = 20;
            for (const auto& it : sections[(size_t) si].items) w += itemWidth (it);
            natural.push_back (w);
            total += w;
        }
        const float scale = (float) (available - margin * ((int) row.size() - 1)) / (float) total;
        int x = margin;
        for (size_t k = 0; k < row.size(); ++k)
        {
            auto& s = sections[(size_t) row[k]];
            const int w = (k == row.size() - 1) ? (margin + available - x) : (int) std::round (natural[k] * scale);
            s.bounds = { x, y, w, rowH - margin };
            x += w + margin;
            auto inner = s.bounds.reduced (10, 0).withTrimmedTop (24).withTrimmedBottom (6);
            int itemsW = 0;
            for (const auto& it : s.items) itemsW += itemWidth (it);
            const float extra = juce::jmax (0.0f, (float) (inner.getWidth() - itemsW) / (float) s.items.size());
            float ix = (float) inner.getX();
            for (const auto& it : s.items)
            {
                const int iw = itemWidth (it);
                auto cell = juce::Rectangle<int> ((int) (ix + extra * 0.5f), inner.getY(), iw, inner.getHeight());
                ix += (float) iw + extra;
                if (it.kind == 0) { auto& kn = knobs[(size_t) it.index]; kn.label->setBounds (cell.removeFromBottom (16)); kn.slider->setBounds (cell); }
                else if (it.kind == 1) { auto& ch = choices[(size_t) it.index]; ch.label->setBounds (cell.removeFromBottom (16)); ch.box->setBounds (cell.withSizeKeepingCentre (iw - 6, 26)); }
                else toggles[(size_t) it.index].button->setBounds (cell.withSizeKeepingCentre (iw - 4, 40));
            }
        }
        y += rowH;
    }
}
