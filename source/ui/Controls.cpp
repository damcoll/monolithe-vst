#include "Controls.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace ui
{
    namespace
    {
        constexpr float knobStart = -2.35619f; // -135 degrees
        constexpr float knobEnd = 2.35619f;
    }

    Knob::Knob (float defaultValue)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (knobStart, knobEnd, true);
        setDoubleClickReturnValue (true, defaultValue);
        setMouseDragSensitivity (180);
        setVelocityBasedMode (false);
    }

    void Knob::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto disc = bounds.withSizeKeepingCentre (size, size);
        const auto centre = disc.getCentre();
        const auto radius = size * 0.5f;

        g.setColour (colours::graphite);
        g.fillEllipse (disc);
        g.setColour (isMouseOverOrDragging() ? colours::craie : colours::poussiere);
        g.drawEllipse (disc.reduced (1.0f), 2.0f);

        const auto angle = knobStart + (float) valueToProportionOfLength (getValue()) * (knobEnd - knobStart);
        const auto direction = juce::Point<float> (std::sin (angle), -std::cos (angle));
        g.setColour (colours::craie);
        g.drawLine ({ centre + direction * (radius * 0.3f), centre + direction * (radius - 5.0f) }, 2.0f);
    }

    void MuteButton::paintButton (juce::Graphics& g, bool highlighted, bool)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const auto on = getToggleState();

        g.setColour (on ? colours::ambre : colours::graphite);
        g.fillRoundedRectangle (bounds, 2.0f);
        g.setColour (highlighted && ! on ? colours::craie : colours::poussiere);
        g.drawRoundedRectangle (bounds, 2.0f, 2.0f);

        g.setColour (on ? colours::noirAbri : colours::craie);
        g.setFont (font (Face::mono, 10.0f, 0.1f));
        g.drawText ("MUTE", getLocalBounds(), juce::Justification::centred);
    }

    Pad::Pad (const juce::String& name, bool lightGroup)
        : juce::Button (name), light (lightGroup)
    {
        setTriggeredOnMouseDown (true); // a pad plays on the hit, not on release
    }

    void Pad::setGlow (float newGlow)
    {
        if (std::abs (newGlow - glow) > 0.01f)
        {
            glow = newGlow;
            repaint();
        }
    }

    void Pad::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const auto base = light ? colours::poussiere : colours::beton;
        const auto lit = down ? 1.0f : glow;

        g.setColour (base.interpolatedWith (colours::ambre, lit));
        g.fillRoundedRectangle (bounds, 2.0f);
        g.setColour (lit > 0.05f ? colours::ambre : (highlighted ? colours::craie : colours::craie.withAlpha (0.8f)));
        g.drawRoundedRectangle (bounds, 2.0f, 2.0f);
    }

    void VuMeter::update (float peak, const juce::String& voiceName)
    {
        // scale: -40 dB at the left stop, +3 dB at the right stop
        const auto db = juce::Decibels::gainToDecibels (peak, -40.0f);
        const auto target = juce::jlimit (0.0f, 1.0f, (db + 40.0f) / 43.0f);

        // VU-like ballistics at 30 Hz: quick rise, slower fall
        needle += (target - needle) * (target > needle ? 0.45f : 0.12f);

        if (voiceName != name)
            name = voiceName;
        repaint();
    }

    void VuMeter::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (colours::ecran);
        g.fillRect (bounds);
        g.setColour (colours::beton);
        g.drawRect (bounds, 3.0f);

        g.saveState();
        g.reduceClipRegion (getLocalBounds().reduced (3));

        // the dial: a circle whose lower half is hidden by the screen edge, like the hardware OLED
        const auto centre = juce::Point<float> (bounds.getCentreX(), 85.0f);
        const auto radius = 59.0f;
        g.setColour (colours::ambre);
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 2.0f);

        const auto angle = juce::jmap (needle, -0.9f, 0.9f); // about +/- 52 degrees
        const auto tip = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (radius - 4.0f);
        g.drawLine ({ centre, tip }, 2.0f);
        g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (centre));

        g.setFont (font (Face::mono, 10.0f, 0.05f));
        g.drawText ("VU", 10, 7, 40, 12, juce::Justification::centredLeft);
        g.drawText (name, getWidth() - 110, 7, 100, 12, juce::Justification::centredRight);
        g.restoreState();
    }
}
