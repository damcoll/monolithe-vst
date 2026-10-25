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

    namespace
    {
        // Classic VU scale: the needle moves linearly with the signal voltage, +3 VU at the right stop.
        // 0 VU is set to -6 dBFS, so a hard hit lands around 0 and the loudest peaks reach the top.
        constexpr float zeroVuGain = 0.5f;
        constexpr float fullScaleGain = zeroVuGain * 1.4125f; // +3 VU

        float scalePosition (float vuDb) { return juce::Decibels::decibelsToGain (vuDb) * zeroVuGain / fullScaleGain; }

        constexpr float sweep = 0.87f; // half the scale angle, ~50 degrees
    }

    void VuMeter::update (float peak, const juce::String& voiceName)
    {
        const auto target = juce::jlimit (0.0f, 1.0f, peak / fullScaleGain);

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

        // the needle pivots on the bottom edge of the screen
        const auto pivot = juce::Point<float> (bounds.getCentreX(), bounds.getBottom() - 4.0f);
        const auto radius = 98.0f;
        const auto pointAt = [&] (float position, float r)
        {
            const auto angle = (position * 2.0f - 1.0f) * sweep;
            return pivot + juce::Point<float> (std::sin (angle), -std::cos (angle)) * r;
        };

        g.setColour (colours::ambre);

        // scale line, thicker above 0 VU
        juce::Path arc, hot;
        arc.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, -sweep, (scalePosition (0.0f) * 2.0f - 1.0f) * sweep, true);
        hot.addCentredArc (pivot.x, pivot.y, radius + 1.5f, radius + 1.5f, 0.0f, (scalePosition (0.0f) * 2.0f - 1.0f) * sweep, sweep, true);
        g.strokePath (arc, juce::PathStrokeType (1.0f));
        g.strokePath (hot, juce::PathStrokeType (4.0f));

        // minor ticks every dB from -10 to +3, major ticks with numbers
        for (int db = -10; db <= 3; ++db)
        {
            const auto p = scalePosition ((float) db);
            g.drawLine ({ pointAt (p, radius), pointAt (p, radius + 4.0f) }, 1.0f);
        }

        g.setFont (font (Face::mono, 9.0f));
        const std::pair<int, const char*> marks[] = { { -20, "20" }, { -10, "10" }, { -7, "7" }, { -5, "5" }, { -3, "3" }, { 0, "0" }, { 3, "+3" } };
        for (const auto& [db, label] : marks)
        {
            const auto p = scalePosition ((float) db);
            g.drawLine ({ pointAt (p, radius - 4.0f), pointAt (p, radius + 7.0f) }, 1.5f);
            const auto at = pointAt (p, radius + 14.0f);
            g.drawText (label, juce::Rectangle<float> (24.0f, 10.0f).withCentre (at), juce::Justification::centred);
        }

        // needle, and the hub hidden half under the bottom edge
        g.drawLine ({ pivot, pointAt (needle, radius + 6.0f) }, 1.8f);
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (pivot));

        g.setFont (font (Face::mono, 10.0f, 0.05f));
        g.drawText ("VU", 10, getHeight() - 20, 40, 12, juce::Justification::centredLeft);
        g.drawText (name, getWidth() - 110, getHeight() - 20, 100, 12, juce::Justification::centredRight);
        g.restoreState();
    }
}
