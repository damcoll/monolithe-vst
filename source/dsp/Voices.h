#pragma once

// The eight MONOLITHE voices. Each voice is monophonic and retriggerable;
// ACCORD and DURÉE are read at trigger time, like on a drum machine.

#include "Blocks.h"

#include <array>
#include <memory>

namespace mono
{
    constexpr int numVoices = 8;
    constexpr int firstNote = 36; // C1: Ableton Drum Rack layout

    // each voice peaks around -6 dBFS on a hard hit, leaving room for the mix
    constexpr float headroom = 0.5f;

    // UTF-8, numbered as on the hardware front panel
    constexpr std::array<const char*, numVoices> voiceNames {
        "GRAVE", "PEAU", "BRUIT", "MÉTAL", "BOIS", "SOUFFLE", "CLOCHE", "DRONE"
    };

    class Voice
    {
    public:
        virtual ~Voice() = default;

        virtual void prepare (double newSampleRate) { sampleRate = newSampleRate; }
        virtual void trigger (float velocity) = 0;
        virtual void release() {}
        virtual float next() = 0;

        // both 0..1, 0.5 = default sound
        void setControls (float newAccord, float newDuree)
        {
            accord = newAccord;
            duree = newDuree;
        }

    protected:
        double sampleRate = 48000.0;
        float accord = 0.5f, duree = 0.5f;
    };

    // 01 GRAVE: struck low resonator with a fast pitch drop, slightly saturated
    class Grave final : public Voice
    {
    public:
        void trigger (float velocity) override
        {
            baseFreq = expMap (accord, 36.0, 87.0);
            body.setDecay (expMap (duree, 0.15, 1.6), sampleRate);
            sweep.setTime (0.03, sampleRate);
            sweep.trigger (0.8f + 1.2f * velocity); // harder hits start higher
            click.setTime (0.004, sampleRate);
            click.trigger (0.12f * velocity);
            strike = velocity;
        }

        float next() override
        {
            body.setFreq (baseFreq * (1.0 + sweep.next()), sampleRate);
            const auto y = body.next (strike) + click.next() * noise.next();
            strike = 0.0f;
            return headroom * softClip (y, 1.6f);
        }

    private:
        Resonator body;
        Decay sweep, click;
        Noise noise { 0x1234567u };
        double baseFreq = 55.0;
        float strike = 0.0f;
    };

    // 02 PEAU: two body modes plus a noise snap through a high-pass
    class Peau final : public Voice
    {
    public:
        void prepare (double newSampleRate) override
        {
            Voice::prepare (newSampleRate);
            snapFilter.set (1800.0, 0.8, sampleRate);
        }

        void trigger (float velocity) override
        {
            const auto f = expMap (accord, 120.0, 260.0);
            const auto bodyTime = expMap (duree, 0.05, 0.25);
            mode1.setDecay (bodyTime, sampleRate);
            mode2.setDecay (bodyTime * 0.7, sampleRate);
            mode1.setFreq (f, sampleRate);
            mode2.setFreq (f * 1.59, sampleRate); // first inharmonic mode of a membrane
            snap.setTime (expMap (duree, 0.08, 0.45), sampleRate);
            snap.trigger (0.35f + 0.35f * velocity); // accent brings the wires forward
            strike = velocity;
        }

        float next() override
        {
            const auto bodyOut = mode1.next (strike) + 0.5f * mode2.next (strike);
            strike = 0.0f;
            snapFilter.process (noise.next());
            const auto y = 0.55f * bodyOut + snapFilter.high * snap.next();
            return headroom * softClip (y, 1.3f);
        }

    private:
        Resonator mode1, mode2;
        Svf snapFilter;
        Decay snap;
        Noise noise { 0x2468aceu };
        float strike = 0.0f;
    };

    // 03 BRUIT: white noise through a tuned band-pass, harder hits are brighter
    class Bruit final : public Voice
    {
    public:
        void trigger (float velocity) override
        {
            const auto freq = expMap (accord, 500.0, 9000.0) * (0.85 + 0.3 * velocity);
            filter.set (freq, 2.2, sampleRate);
            env.setTime (expMap (duree, 0.03, 0.6), sampleRate);
            env.trigger (velocity);
        }

        float next() override
        {
            filter.process (noise.next());
            // a little of the high-pass keeps some air above the band
            const auto y = (1.6f * filter.band + 0.25f * filter.high) * env.next();
            return headroom * softClip (y, 1.2f);
        }

    private:
        Svf filter;
        Decay env;
        Noise noise { 0x13579bdu };
    };

    // 04 MÉTAL: six squares at the 808 cymbal ratios, two band-passes, then a high-pass
    class Metal final : public Voice
    {
    public:
        void prepare (double newSampleRate) override
        {
            Voice::prepare (newSampleRate);
            low.set (3440.0, 3.0, sampleRate);
            high.set (7100.0, 3.0, sampleRate);
            air.set (6000.0, 0.7, sampleRate);
            // spread the starting phases so the six edges don't line up
            for (size_t i = 0; i < oscillators.size(); ++i)
                oscillators[i].phase = 0.137 * (double) i;
        }

        void trigger (float velocity) override
        {
            const auto scale = expMap (accord, 0.5, 2.0);
            for (size_t i = 0; i < oscillators.size(); ++i)
                oscillators[i].setFreq (ratios808[i] * scale, sampleRate);
            env.setTime (expMap (duree, 0.03, 0.9), sampleRate);
            env.trigger (velocity);
        }

        float next() override
        {
            float sum = 0.0f;
            for (auto& o : oscillators)
                sum += o.next();

            low.process (sum);
            high.process (sum);
            air.process (low.band + high.band);
            const auto y = 0.9f * air.high * env.next();
            return headroom * softClip (y, 1.5f);
        }

    private:
        static constexpr std::array<double, 6> ratios808 { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
        std::array<Square, 6> oscillators;
        Svf low, high, air;
        Decay env;
    };

    // returns nullptr for the voices not written yet
    inline std::unique_ptr<Voice> makeVoice (int index)
    {
        switch (index)
        {
            case 0: return std::make_unique<Grave>();
            case 1: return std::make_unique<Peau>();
            case 2: return std::make_unique<Bruit>();
            case 3: return std::make_unique<Metal>();
            default: return nullptr;
        }
    }
}
