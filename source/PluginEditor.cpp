#include "PluginEditor.h"

namespace colours
{
    const juce::Colour noirAbri  { 0xff0e0e0e };
    const juce::Colour graphite  { 0xff2a2927 };
    const juce::Colour beton     { 0xff5a5853 };
    const juce::Colour poussiere { 0xffc9c7c1 };
    const juce::Colour craie     { 0xffe9e7e2 };
    const juce::Colour ambre     { 0xffffb000 };
}

namespace
{
    constexpr int stripsLeft = 230, stripWidth = 87, topBar = 64;

    void styleKnob (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setColour (juce::Slider::rotarySliderFillColourId, colours::craie);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, colours::graphite);
        s.setColour (juce::Slider::thumbColourId, colours::craie);
    }

    juce::String fr (const char* utf8) { return juce::String::fromUTF8 (utf8); }

    // Ableton names middle C (60) C3, so 36 is C1
    juce::String noteName (int note)
    {
        return juce::MidiMessage::getMidiNoteName (note, true, true, 3);
    }
}

MonolitheEditor::Strip::Strip (MonolitheProcessor& p, int voiceIndex)
    : title (juce::String::formatted ("%02d ", voiceIndex + 1) + juce::String::fromUTF8 (mono::voiceNames[(size_t) voiceIndex])),
      available (mono::makeVoice (voiceIndex) != nullptr),
      listen (noteName (mono::firstNote + voiceIndex)),
      niveauAttachment (p.parameters, MonolitheProcessor::paramId (voiceIndex, "niveau"), niveau),
      accordAttachment (p.parameters, MonolitheProcessor::paramId (voiceIndex, "accord"), accord),
      dureeAttachment (p.parameters, MonolitheProcessor::paramId (voiceIndex, "duree"), duree),
      muteAttachment (p.parameters, MonolitheProcessor::paramId (voiceIndex, "mute"), mute)
{
    for (auto* s : { &niveau, &accord, &duree })
    {
        styleKnob (*s);
        s->setEnabled (available);
        addAndMakeVisible (*s);
    }

    mute.setColour (juce::ToggleButton::textColourId, colours::poussiere);
    mute.setColour (juce::ToggleButton::tickColourId, colours::ambre);
    addAndMakeVisible (mute);

    listen.setColour (juce::TextButton::buttonColourId, available ? colours::poussiere : colours::graphite);
    listen.setColour (juce::TextButton::textColourOffId, colours::noirAbri);
    listen.setEnabled (available);
    listen.onClick = [&p, voiceIndex] { p.auditionMask.fetch_or (1u << voiceIndex); };
    addAndMakeVisible (listen);
}

void MonolitheEditor::Strip::paint (juce::Graphics& g)
{
    g.setColour (available ? colours::craie : colours::beton);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText (title, 0, 6, getWidth(), 20, juce::Justification::centred);

    g.setColour (available ? colours::poussiere : colours::beton);
    g.setFont (juce::FontOptions (10.0f));
    const auto labels = { "NIVEAU", "ACCORD", "DURÉE" };
    int y = 92;
    for (auto* l : labels)
    {
        g.drawText (fr (l), 0, y, getWidth(), 12, juce::Justification::centred);
        y += 74;
    }
    if (! available)
        g.drawText (fr ("À VENIR"), 0, 26, getWidth(), 12, juce::Justification::centred);
}

void MonolitheEditor::Strip::resized()
{
    const auto x = (getWidth() - 56) / 2;
    niveau.setBounds (x, 36, 56, 56);
    accord.setBounds (x, 110, 56, 56);
    duree.setBounds (x, 184, 56, 56);
    mute.setBounds (x - 4, 262, 70, 24);
    listen.setBounds (x, 296, 56, 56);
}

MonolitheEditor::MonolitheEditor (MonolitheProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      masterAttachment (p.parameters, "master", master),
      accentAttachment (p.parameters, "accent", accent)
{
    for (int i = 0; i < mono::numVoices; ++i)
    {
        strips[(size_t) i] = std::make_unique<Strip> (p, i);
        addAndMakeVisible (*strips[(size_t) i]);
    }

    styleKnob (master);
    styleKnob (accent);
    addAndMakeVisible (master);
    addAndMakeVisible (accent);

    setSize (1100, 520);
    startTimerHz (30);
}

void MonolitheEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::noirAbri);

    g.setColour (colours::craie);
    g.setFont (juce::FontOptions (40.0f, juce::Font::bold));
    g.drawText ("MONOLITHE", 24, 12, 400, 44, juce::Justification::centredLeft);

    g.setColour (colours::graphite);
    g.drawHorizontalLine (topBar, 0.0f, (float) getWidth());
    g.drawVerticalLine (stripsLeft, (float) topBar, (float) getHeight());
    g.drawVerticalLine (stripsLeft + stripWidth * mono::numVoices, (float) topBar, (float) getHeight());

    // MIDI LED + last note received, to check what a controller sends
    g.setColour (colours::graphite.interpolatedWith (colours::ambre, ledLevel));
    g.fillEllipse (640.0f, 26.0f, 12.0f, 12.0f);

    auto noteText = fr ("DERNIÈRE NOTE : aucune");
    if (shownNote >= 0)
    {
        noteText = fr ("DERNIÈRE NOTE : ") + juce::String (shownNote) + " (" + noteName (shownNote) + ")";
        const auto voice = shownNote - mono::firstNote;
        noteText += voice >= 0 && voice < mono::numVoices
                        ? fr ("  →  ") + juce::String::formatted ("%02d ", voice + 1) + juce::String::fromUTF8 (mono::voiceNames[(size_t) voice])
                        : fr ("  →  hors plage (C1 à G1)");
    }
    g.setColour (colours::poussiere);
    g.setFont (juce::FontOptions (14.0f));
    g.drawText (noteText, 662, 22, 430, 20, juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("MASTER", 40, 410, 150, 16, juce::Justification::centred);
    g.drawText ("ACCENT", 940, 246, 150, 16, juce::Justification::centred);
    g.setColour (colours::beton);
    g.drawText (fr ("Fenêtre provisoire — étape 2 : les voix"), 24, 490, 400, 16, juce::Justification::centredLeft);
}

void MonolitheEditor::resized()
{
    for (int i = 0; i < mono::numVoices; ++i)
        strips[(size_t) i]->setBounds (stripsLeft + stripWidth * i, topBar + 10, stripWidth, 380);

    master.setBounds (65, 300, 100, 100);
    accent.setBounds (975, 160, 80, 80);
}

void MonolitheEditor::timerCallback()
{
    const auto count = processor.noteCounter.load (std::memory_order_relaxed);
    if (count != lastNoteCount)
    {
        lastNoteCount = count;
        ledLevel = 1.0f;
    }
    else
    {
        ledLevel *= 0.8f;
    }

    const auto note = processor.lastNote.load (std::memory_order_relaxed);
    if (note != shownNote)
    {
        shownNote = note;
        repaint (630, 16, 470, 32);
    }
    repaint (630, 16, 30, 32);
}
