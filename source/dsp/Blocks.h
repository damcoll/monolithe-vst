#pragma once

// Small DSP building blocks shared by the voices. Header-only, no allocation,
// safe to call from the audio thread.

#include <cmath>
#include <cstdint>

namespace mono
{
    constexpr double twoPi = 6.283185307179586;

    // -60 dB reached after `seconds`
    inline float decayCoef (double seconds, double sampleRate)
    {
        return (float) std::exp (-6.907755278982137 / (seconds * sampleRate));
    }

    // maps 0..1 onto lo..hi on an exponential curve (frequencies, times)
    inline double expMap (float x, double lo, double hi)
    {
        return lo * std::pow (hi / lo, (double) x);
    }

    struct Noise
    {
        explicit Noise (uint32_t seed = 0x9e3779b9u) : state (seed) {}

        float next()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return (float) (int32_t) state * (1.0f / 2147483648.0f);
        }

        uint32_t state;
    };

    // Exponential decay from the trigger value
    struct Decay
    {
        void setTime (double seconds, double sampleRate) { coef = decayCoef (seconds, sampleRate); }
        void trigger (float startValue) { value = startValue; }

        float next()
        {
            const auto v = value;
            value *= coef;
            return v;
        }

        float value = 0.0f, coef = 0.0f;
    };

    // Two-pole resonator that rings when struck, like the bridged-T of a 808.
    // An impulse of amplitude A gives a decaying sine of amplitude ~A.
    struct Resonator
    {
        void setDecay (double seconds, double sampleRate) { r = std::exp (-6.907755278982137 / (seconds * sampleRate)); }

        void setFreq (double freq, double sampleRate)
        {
            const auto w = twoPi * freq / sampleRate;
            a1 = 2.0 * r * std::cos (w);
            a2 = -r * r;
            inGain = std::sin (w);
        }

        float next (float input)
        {
            const auto y = input * inGain + a1 * y1 + a2 * y2;
            y2 = y1;
            y1 = y;
            return (float) y;
        }

        double r = 0.0, a1 = 0.0, a2 = 0.0, inGain = 0.0;
        double y1 = 0.0, y2 = 0.0;
    };

    // Topology-preserving state-variable filter (Cytomic), low/band/high outputs
    struct Svf
    {
        void set (double freq, double q, double sampleRate)
        {
            const auto g = std::tan (twoPi * 0.5 * freq / sampleRate);
            k = 1.0 / q;
            a1 = 1.0 / (1.0 + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        void process (float input)
        {
            const auto v3 = input - ic2;
            const auto v1 = a1 * ic1 + a2 * v3;
            const auto v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0 * v1 - ic1;
            ic2 = 2.0 * v2 - ic2;
            low = (float) v2;
            band = (float) v1;
            high = (float) (input - k * v1 - v2);
        }

        double k = 1.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
        double ic1 = 0.0, ic2 = 0.0;
        float low = 0.0f, band = 0.0f, high = 0.0f;
    };

    // soft saturation normalised so that +/-1 stays +/-1
    inline float softClip (float x, float drive)
    {
        return std::tanh (x * drive) / std::tanh (drive);
    }
}
