#pragma once

// The MACHINE section: what makes MONOLITHE sound like an old box.
//   DÉRIVE    - analog instability: each hit comes out slightly different,
//               and each voice slowly wanders, like circuits warming up.
//   POUSSIÈRE - lo-fi: fewer bits, lower sample rate, darker tone, hiss and crackle.

#include "Blocks.h"
#include "Voices.h"

#include <algorithm>
#include <array>

namespace mono
{
    class Drift
    {
    public:
        struct Hit
        {
            float accord, duree, velocity;
        };

        void prepare (double sampleRate)
        {
            // the slow wander settles over ~1.5 s and picks a new direction every ~2 s
            smoothing = decayCoef (1.5, sampleRate);
            retargetSamples = (int) (2.0 * sampleRate);
        }

        // call once per block: moves each voice's slow wander
        void advance (int numSamples)
        {
            const auto blockCoef = std::pow (smoothing, (float) numSamples);
            for (size_t i = 0; i < wander.size(); ++i)
            {
                countdown[i] -= numSamples;
                if (countdown[i] <= 0)
                {
                    target[i] = noise.next();
                    countdown[i] = retargetSamples / 2 + (int) ((noise.next() * 0.5f + 0.5f) * (float) retargetSamples);
                }
                wander[i] = target[i] + (wander[i] - target[i]) * blockCoef;
            }
        }

        // the hit actually played, given the knob settings and DÉRIVE (0..1)
        Hit apply (int voice, float accord, float duree, float velocity, float amount)
        {
            if (amount <= 0.0f)
                return { accord, duree, velocity };

            const auto w = wander[(size_t) voice];
            return {
                std::clamp (accord + amount * (0.05f * w + 0.02f * noise.next()), 0.0f, 1.0f),
                std::clamp (duree + amount * 0.10f * noise.next(), 0.0f, 1.0f),
                std::clamp (velocity * (1.0f + amount * 0.15f * noise.next()), 0.0f, 1.0f)
            };
        }

    private:
        std::array<float, numVoices> wander {}, target {};
        std::array<int, numVoices> countdown {};
        float smoothing = 0.0f;
        int retargetSamples = 96000;
        Noise noise { 0xd1f7u };
    };

    class Dust
    {
    public:
        void prepare (double newSampleRate)
        {
            sampleRate = newSampleRate;
            hissFilter.set (5000.0, 0.5, sampleRate);
            crackleDecay = decayCoef (0.002, sampleRate);
            setAmount (amount);
        }

        // 0..1, call once per block
        void setAmount (float newAmount)
        {
            amount = newAmount;
            const auto shaped = std::pow (amount, 1.5f);
            holdRatio = std::pow (6000.0 / sampleRate, (double) shaped);       // down to a 6 kHz sample rate
            step = std::pow (2.0f, -(15.0f - 8.0f * std::pow (amount, 1.2f))); // 16 bits down to 8 bits
            tone.set (std::min (expMap (amount, 18000.0, 3500.0), 0.45 * sampleRate), 0.6, sampleRate);
            hissGain = 0.006f * amount;
            crackleChance = 8.0f * amount * amount / (float) sampleRate;
        }

        float process (float x)
        {
            if (amount <= 0.001f)
                return x;

            // sample-rate reduction: hold the sample until the slower clock ticks
            holdPhase += holdRatio;
            if (holdPhase >= 1.0)
            {
                holdPhase -= 1.0;
                held = std::round (x / step) * step; // and fewer bits
            }
            tone.process (held);

            // tape hiss and the odd crackle
            hissFilter.process (noise.next());
            if ((noise.next() * 0.5f + 0.5f) < crackleChance)
                crackle = (0.03f + 0.09f * (noise.next() * 0.5f + 0.5f)) * amount * (noise.next() > 0.0f ? 1.0f : -1.0f);
            const auto click = crackle;
            crackle *= crackleDecay;

            return tone.low + hissFilter.band * hissGain + click;
        }

    private:
        double sampleRate = 48000.0, holdRatio = 1.0, holdPhase = 0.0;
        float amount = 0.0f, step = 0.0f, held = 0.0f;
        float hissGain = 0.0f, crackleChance = 0.0f, crackle = 0.0f, crackleDecay = 0.0f;
        Svf tone, hissFilter;
        Noise noise { 0xc0ffeeu };
    };
}
