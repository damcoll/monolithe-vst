#pragma once

#include "PluginProcessor.h"

// Step 1 placeholder window: logo, MIDI LED and the MASTER knob.
// The full interface from the mockup comes in step 3.
class MonolitheEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit MonolitheEditor (MonolitheProcessor&);
    ~MonolitheEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MonolitheProcessor& processor;
    juce::Slider master;
    juce::TextButton test { "TEST C1" };
    juce::AudioProcessorValueTreeState::SliderAttachment masterAttachment;

    int lastNoteCount = 0;
    float ledLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonolitheEditor)
};
