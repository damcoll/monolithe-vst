#include "PluginEditor.h"

using namespace ui;

namespace
{
    // layout of the mockup (px on the 1100 x 520 panel)
    constexpr int topBar = 64, stripsLeft = 230, stripsRight = 930;
    constexpr float stripWidth = 87.5f;
    constexpr int knobRows[] = { 148, 222, 296 };
    constexpr int knobSize = 44, muteTop = 346, padTop = 382, padSize = 56;

    float stripCentre (int voice) { return (float) stripsLeft + stripWidth * ((float) voice + 0.5f); }

    // Ableton names middle C (60) C3, so 36 is C1
    juce::String noteName (int note) { return juce::MidiMessage::getMidiNoteName (note, true, true, 3); }

    juce::String voiceTitle (int voice) { return fr (mono::voiceNames[(size_t) voice]); }

    float defaultOf (MonolitheProcessor& p, const juce::String& id)
    {
        return p.parameters.getParameter (id)->getDefaultValue();
    }

    void centredText (juce::Graphics& g, const juce::String& text, float centreX, int y, int height, juce::Font f, juce::Colour c)
    {
        g.setFont (f);
        g.setColour (c);
        g.drawText (text, juce::Rectangle<float> (centreX - 60.0f, (float) y, 120.0f, (float) height), juce::Justification::centred);
    }
}

MonolitheEditor::Strip::Strip (MonolitheProcessor& p, int voice)
    : niveau (defaultOf (p, MonolitheProcessor::paramId (voice, "niveau"))),
      accord (defaultOf (p, MonolitheProcessor::paramId (voice, "accord"))),
      duree (defaultOf (p, MonolitheProcessor::paramId (voice, "duree"))),
      pad (fr ("Écouter ") + voiceTitle (voice), (voice / 4) % 2 == 0),
      niveauAttachment (p.parameters, MonolitheProcessor::paramId (voice, "niveau"), niveau),
      accordAttachment (p.parameters, MonolitheProcessor::paramId (voice, "accord"), accord),
      dureeAttachment (p.parameters, MonolitheProcessor::paramId (voice, "duree"), duree),
      muteAttachment (p.parameters, MonolitheProcessor::paramId (voice, "mute"), mute)
{
    pad.onClick = [&p, voice] { p.auditionMask.fetch_or (1u << voice); };
}

MonolitheEditor::Panel::Panel (MonolitheProcessor& p)
    : processor (p),
      master (defaultOf (p, "master")),
      derive (defaultOf (p, "derive")),
      poussiere (defaultOf (p, "poussiere")),
      accent (defaultOf (p, "accent")),
      masterAttachment (p.parameters, "master", master),
      deriveAttachment (p.parameters, "derive", derive),
      poussiereAttachment (p.parameters, "poussiere", poussiere),
      accentAttachment (p.parameters, "accent", accent)
{
    for (int i = 0; i < mono::numVoices; ++i)
    {
        auto& s = *(strips[(size_t) i] = std::make_unique<Strip> (p, i));
        for (juce::Component* c : { (juce::Component*) &s.niveau, (juce::Component*) &s.accord, (juce::Component*) &s.duree,
                                    (juce::Component*) &s.mute, (juce::Component*) &s.pad })
            addAndMakeVisible (c);
    }

    for (auto* k : { &master, &derive, &poussiere, &accent })
        addAndMakeVisible (k);
    addAndMakeVisible (vu);
}

void MonolitheEditor::Panel::resized()
{
    for (int i = 0; i < mono::numVoices; ++i)
    {
        auto& s = *strips[(size_t) i];
        const auto cx = (int) std::round (stripCentre (i));
        ui::Knob* knobs[] = { &s.niveau, &s.accord, &s.duree };
        for (int row = 0; row < 3; ++row)
            knobs[row]->setBounds (cx - knobSize / 2, knobRows[row] - knobSize / 2, knobSize, knobSize);
        s.mute.setBounds (cx - padSize / 2, muteTop, padSize, 24);
        s.pad.setBounds (cx - padSize / 2, padTop, padSize, padSize);
    }

    vu.setBounds (20, 84, 190, 132);
    master.setBounds (115 - 38, 300 - 38, 76, 76);
    derive.setBounds (1015 - 26, 150 - 26, 52, 52);
    poussiere.setBounds (1015 - 26, 250 - 26, 52, 52);
    accent.setBounds (1015 - 26, 350 - 26, 52, 52);
}

void MonolitheEditor::Panel::paint (juce::Graphics& g)
{
    g.fillAll (colours::noirAbri);

    // dividers
    g.setColour (colours::joint);
    g.fillRect (0, topBar - 1, panelWidth, 1);
    g.fillRect (stripsLeft, topBar, 1, panelHeight - topBar);
    g.fillRect (stripsRight, topBar, 1, panelHeight - topBar);
    g.setColour (colours::graphite);
    for (int i = 1; i < mono::numVoices; ++i)
        g.fillRect ((int) std::round (stripsLeft + stripWidth * (float) i), 84, 1, 416);

    // top bar: stencil logo, subtitle, last note, MIDI LED
    const auto logoFont = font (Face::stencil, 38.0f);
    const auto logoWidth = (int) std::ceil (juce::GlyphArrangement::getStringWidth (logoFont, "MONOLITHE"));
    g.setColour (colours::craie);
    g.setFont (logoFont);
    g.drawText ("MONOLITHE", 24, 12, logoWidth + 4, 44, juce::Justification::centredLeft);
    g.setColour (colours::gris);
    g.setFont (font (Face::mono, 11.0f, 0.16f));
    g.drawText (fr ("8 VOIX · LO-FI · MIDI"), 24 + logoWidth + 18, 27, 300, 16, juce::Justification::centredLeft);

    if (shownNote >= 0)
        g.drawText (fr ("DERNIÈRE NOTE ") + juce::String (shownNote) + fr (" · ") + noteName (shownNote),
                    600, 24, 290, 16, juce::Justification::centredRight);

    g.setColour (colours::graphite.interpolatedWith (colours::ambre, ledLevel));
    g.fillEllipse (908.0f, 27.0f, 10.0f, 10.0f);
    g.setColour (colours::poussiere);
    g.setFont (font (Face::mono, 11.0f, 0.12f));
    g.drawText (fr ("MIDI · C1 → G1"), 926, 24, 160, 16, juce::Justification::centredLeft);

    // left: output section
    centredText (g, "SORTIE", 115.0f, 222, 12, font (Face::mono, 10.0f, 0.1f), colours::gris);
    centredText (g, "MASTER", 115.0f, 343, 12, font (Face::mono, 10.0f, 0.08f), colours::poussiere);

    // voices
    const char* knobLabels[] = { "NIVEAU", "ACCORD", "DURÉE" };
    for (int i = 0; i < mono::numVoices; ++i)
    {
        const auto cx = stripCentre (i);
        centredText (g, juce::String::formatted ("%02d", i + 1), cx, 78, 12, font (Face::mono, 10.0f), colours::gris);
        centredText (g, voiceTitle (i), cx, 92, 24, font (Face::display, 22.0f, 0.04f), colours::craie);
        for (int row = 0; row < 3; ++row)
            centredText (g, fr (knobLabels[row]), cx, knobRows[row] + knobSize / 2 + 4, 12, font (Face::mono, 10.0f, 0.08f), colours::poussiere);
        centredText (g, noteName (mono::firstNote + i), cx, 444, 14, font (Face::mono, 11.0f), colours::gris);
    }

    // right: machine section
    centredText (g, "MACHINE", 1015.0f, 76, 26, font (Face::display, 22.0f, 0.04f), colours::craie);
    centredText (g, fr ("DÉRIVE"), 1015.0f, 180, 12, font (Face::mono, 10.0f, 0.08f), colours::poussiere);
    centredText (g, fr ("POUSSIÈRE"), 1015.0f, 280, 12, font (Face::mono, 10.0f, 0.08f), colours::poussiere);
    centredText (g, "ACCENT", 1015.0f, 380, 12, font (Face::mono, 10.0f, 0.08f), colours::poussiere);
    centredText (g, fr ("v0.1 · CODE OUVERT"), 1015.0f, 452, 12, font (Face::mono, 10.0f, 0.05f), colours::gris);
}

MonolitheEditor::MonolitheEditor (MonolitheProcessor& p)
    : AudioProcessorEditor (&p), processor (p), panel (p)
{
    addAndMakeVisible (panel);

    // resizable from 75 % to 200 %, keeping the proportions; the size is saved with the set
    setResizable (true, true);
    setResizeLimits (panelWidth * 3 / 4, panelHeight * 3 / 4, panelWidth * 2, panelHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) panelWidth / panelHeight);
    const auto savedWidth = (int) p.parameters.state.getProperty ("uiWidth", panelWidth);
    setSize (savedWidth, savedWidth * panelHeight / panelWidth);

    startTimerHz (30);
}

void MonolitheEditor::resized()
{
    const auto scale = (float) getWidth() / (float) panelWidth;
    panel.setBounds (0, 0, panelWidth, panelHeight);
    panel.setTransform (juce::AffineTransform::scale (scale));
    processor.parameters.state.setProperty ("uiWidth", getWidth(), nullptr);
}

void MonolitheEditor::timerCallback()
{
    // MIDI LED: amber flash on every note, fading out
    const auto count = processor.noteCounter.load (std::memory_order_relaxed);
    const auto newNote = count != lastNoteCount;
    lastNoteCount = count;
    const auto led = newNote ? 1.0f : panel.ledLevel * 0.8f;
    if (std::abs (led - panel.ledLevel) > 0.01f || newNote)
    {
        panel.ledLevel = led;
        panel.repaint (900, 18, 24, 28);
    }

    const auto note = processor.lastNote.load (std::memory_order_relaxed);
    if (note != panel.shownNote)
    {
        panel.shownNote = note;
        panel.repaint (600, 18, 300, 28);
    }

    // pads follow their voice level: instant rise, ~150 ms fall
    for (size_t i = 0; i < panel.strips.size(); ++i)
    {
        auto& s = *panel.strips[i];
        const auto peak = processor.voicePeaks[i].exchange (0.0f, std::memory_order_relaxed);
        const auto target = juce::jlimit (0.0f, 1.0f, std::sqrt (peak / 0.4f));
        s.glow = juce::jmax (target, s.glow * 0.78f);
        s.pad.setGlow (s.glow < 0.02f ? 0.0f : s.glow);
    }

    const auto voice = processor.lastVoice.load (std::memory_order_relaxed);
    panel.vu.update (processor.outputPeak.exchange (0.0f, std::memory_order_relaxed),
                     voice >= 0 ? voiceTitle (voice) : juce::String());
}
