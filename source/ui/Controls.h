#pragma once

#include "Theme.h"

namespace ui
{
    // Graphite disc with a dust ring and a chalk index line.
    // Vertical drag, mouse wheel, double-click returns to the default value.
    class Knob final : public juce::Slider
    {
    public:
        explicit Knob (float defaultValue);
        void paint (juce::Graphics&) override;
    };

    // Square MUTE key, filled with amber while the voice is muted
    class MuteButton final : public juce::ToggleButton
    {
    public:
        MuteButton() : juce::ToggleButton ("MUTE") {}
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    };

    // Rubber pad: lights up in amber while its voice sounds, plays the voice when pressed
    class Pad final : public juce::Button
    {
    public:
        Pad (const juce::String& name, bool lightGroup);

        // 0..1, driven by the editor from the voice level
        void setGlow (float newGlow);
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;

    private:
        bool light;
        float glow = 0.0f;
    };

    // Amber screen with a VU needle and the name of the last voice played
    class VuMeter final : public juce::Component
    {
    public:
        // peak since last call (linear) and the voice name to show
        void update (float peak, const juce::String& voiceName);
        void paint (juce::Graphics&) override;

    private:
        float needle = 0.0f; // 0..1 across the scale
        juce::String name;
    };
}
