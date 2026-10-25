// Renders every finished voice to a .wav (soft and hard hit), without a DAW,
// and prints the peak level and ring time to check the tuning.
//
//   monolithe_render <output folder>

#include <juce_audio_formats/juce_audio_formats.h>

#include "dsp/Voices.h"

#include <iostream>

namespace
{
    constexpr double renderRate = 48000.0;
    constexpr double hitSpacing = 1.0; // seconds between the two hits
    constexpr double length = 2.5;

    bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer)
    {
        file.deleteFile();
        std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
        if (stream == nullptr)
            return false;

        juce::WavAudioFormat wav;
        const auto options = juce::AudioFormatWriterOptions{}
                                 .withSampleRate (renderRate)
                                 .withNumChannels (1)
                                 .withBitsPerSample (24);
        auto writer = wav.createWriterFor (stream, options);
        return writer != nullptr && writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
    }
}

int main (int argc, char* argv[])
{
    const auto outDir = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "renders");
    outDir.createDirectory();

    for (int i = 0; i < mono::numVoices; ++i)
    {
        auto voice = mono::makeVoice (i);
        if (voice == nullptr)
            continue;

        voice->prepare (renderRate);
        juce::AudioBuffer<float> buffer (1, (int) (length * renderRate));
        auto* out = buffer.getWritePointer (0);

        // a soft hit (velocity 0.5) then a hard one (1.0), default ACCORD and DURÉE
        const int secondHit = (int) (hitSpacing * renderRate);
        float peakSoft = 0.0f, peakHard = 0.0f;
        int lastLoud = 0;
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            if (n == 0) voice->trigger (0.5f);
            if (n == secondHit) voice->trigger (1.0f);
            if (n == secondHit / 2 || n == secondHit + secondHit / 2) voice->release(); // held voices (DRONE)
            out[n] = voice->next();

            const auto a = std::abs (out[n]);
            (n < secondHit ? peakSoft : peakHard) = std::max (n < secondHit ? peakSoft : peakHard, a);
            if (n >= secondHit && a > 0.001f) // -60 dB
                lastLoud = n;
            if (! std::isfinite (out[n]))
            {
                std::cerr << "NaN/inf in voice " << i + 1 << "\n";
                return 1;
            }
        }

        const auto name = juce::String::formatted ("%02d_", i + 1) + juce::String::fromUTF8 (mono::voiceNames[(size_t) i]);
        const auto file = outDir.getChildFile (name + ".wav");
        if (! writeWav (file, buffer))
        {
            std::cerr << "cannot write " << file.getFullPathName() << "\n";
            return 1;
        }

        std::cout << name << "  peak soft " << juce::String (juce::Decibels::gainToDecibels (peakSoft), 1)
                  << " dB, hard " << juce::String (juce::Decibels::gainToDecibels (peakHard), 1)
                  << " dB, rings " << juce::String ((lastLoud - secondHit) / renderRate * 1000.0, 0) << " ms\n";
    }
    return 0;
}
