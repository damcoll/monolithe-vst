#pragma once

#include "PluginProcessor.h"

// Temporary window for step 2: standard controls to tune the voices.
// The real front panel from the mockup comes in step 3.
class MonolitheEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit MonolitheEditor (MonolitheProcessor&);
    ~MonolitheEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // one column: three knobs, MUTE, and a button to hear the voice
    struct Strip final : public juce::Component
    {
        Strip (MonolitheProcessor&, int voiceIndex);
        void paint (juce::Graphics&) override;
        void resized() override;

        juce::String title;
        bool available;
        juce::Slider niveau, accord, duree;
        juce::ToggleButton mute { "MUTE" };
        juce::TextButton listen;
        SliderAttachment niveauAttachment, accordAttachment, dureeAttachment;
        ButtonAttachment muteAttachment;
    };

    void timerCallback() override;

    MonolitheProcessor& processor;
    std::array<std::unique_ptr<Strip>, mono::numVoices> strips;
    juce::Slider master, accent;
    SliderAttachment masterAttachment, accentAttachment;

    int lastNoteCount = 0;
    int shownNote = -1;
    float ledLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonolitheEditor)
};
