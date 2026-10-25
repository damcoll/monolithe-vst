#pragma once

#include "PluginProcessor.h"
#include "ui/Controls.h"

// The MONOLITHE front panel, drawn at 1100 x 520 and scaled to the window size.
class MonolitheEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit MonolitheEditor (MonolitheProcessor&);
    ~MonolitheEditor() override = default;

    void resized() override;

    static constexpr int panelWidth = 1100, panelHeight = 520;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // one voice column: three knobs, MUTE, pad
    struct Strip
    {
        Strip (MonolitheProcessor&, int voiceIndex);

        ui::Knob niveau, accord, duree;
        ui::MuteButton mute;
        ui::Pad pad;
        SliderAttachment niveauAttachment, accordAttachment, dureeAttachment;
        ButtonAttachment muteAttachment;
        float glow = 0.0f;
    };

    // everything lives in this fixed-size panel, scaled as one block
    struct Panel final : public juce::Component
    {
        explicit Panel (MonolitheProcessor&);
        void paint (juce::Graphics&) override;
        void resized() override;

        MonolitheProcessor& processor;
        std::array<std::unique_ptr<Strip>, mono::numVoices> strips;
        ui::VuMeter vu;
        ui::Knob master, derive, poussiere, accent;
        SliderAttachment masterAttachment, deriveAttachment, poussiereAttachment, accentAttachment;

        float ledLevel = 0.0f;
        int shownNote = -1;
    };

    void timerCallback() override;

    MonolitheProcessor& processor;
    ui::FontsLease fonts;
    Panel panel;
    int lastNoteCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonolitheEditor)
};
