#include "PluginEditor.h"

namespace colours
{
    const juce::Colour noirAbri  { 0xff0e0e0e };
    const juce::Colour graphite  { 0xff2a2927 };
    const juce::Colour poussiere { 0xffc9c7c1 };
    const juce::Colour craie     { 0xffe9e7e2 };
    const juce::Colour ambre     { 0xffffb000 };
}

MonolitheEditor::MonolitheEditor (MonolitheProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      masterAttachment (p.parameters, "master", master)
{
    master.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    master.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    master.setColour (juce::Slider::rotarySliderFillColourId, colours::craie);
    master.setColour (juce::Slider::rotarySliderOutlineColourId, colours::graphite);
    master.setColour (juce::Slider::thumbColourId, colours::craie);
    addAndMakeVisible (master);

    test.setColour (juce::TextButton::buttonColourId, colours::graphite);
    test.setColour (juce::TextButton::textColourOffId, colours::craie);
    test.onClick = [this] { processor.testTrigger.store (true); };
    addAndMakeVisible (test);

    setSize (1100, 520);
    startTimerHz (30);
}

void MonolitheEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::noirAbri);

    g.setColour (colours::craie);
    g.setFont (juce::FontOptions (40.0f, juce::Font::bold));
    g.drawText ("MONOLITHE", 24, 12, 400, 44, juce::Justification::centredLeft);

    g.setColour (colours::poussiere);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("SQUELETTE v0.1 - une note C1 joue un son de test", 24, 70, 700, 20,
                juce::Justification::centredLeft);
    g.drawText ("MASTER", 40, 410, 150, 20, juce::Justification::centred);
    g.drawText ("MIDI", 1000, 22, 60, 20, juce::Justification::centredLeft);

    // MIDI LED: amber flash that fades out
    g.setColour (colours::graphite.interpolatedWith (colours::ambre, ledLevel));
    g.fillEllipse (980.0f, 26.0f, 12.0f, 12.0f);
}

void MonolitheEditor::resized()
{
    master.setBounds (65, 300, 100, 100);
    test.setBounds (260, 340, 120, 40);
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
    repaint (970, 16, 30, 32);
}
