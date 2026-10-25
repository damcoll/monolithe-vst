// Plays a short pattern through the whole plugin (voices + MACHINE section)
// at several DÉRIVE / POUSSIÈRE settings, one .wav each.
//
//   monolithe_demo <output folder>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    constexpr double renderRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr double bpm = 108.0;
    constexpr int bars = 4;

    // 16 steps per bar, one row per voice (notes 36..43): 'X' accent, 'x' normal hit, '-' held (DRONE)
    const char* pattern[] = {
        "X-----x---x-----", // GRAVE
        "----X-------X--x", // PEAU
        "--x---x---x---x-", // BRUIT
        "x-x-x-x-x-x-xXx-", // MÉTAL
        "-------x------x-", // BOIS
        "X---------------", // SOUFFLE
        "--------------x-", // CLOCHE
        "x-------x-------", // DRONE (held half a bar)
    };

    void setParam (juce::AudioProcessor& p, const juce::String& id, float value)
    {
        for (auto* param : p.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param); ranged != nullptr && ranged->getParameterID() == id)
                ranged->setValueNotifyingHost (value);
    }
}

int main (int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI juce;
    const auto outDir = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "renders");
    outDir.createDirectory();

    const std::pair<float, const char*> settings[] = { { 0.0f, "propre" }, { 0.2f, "defaut" }, { 0.6f, "use" }, { 1.0f, "ruine" } };
    const auto samplesPerStep = (int) (renderRate * 60.0 / bpm / 4.0);
    const auto totalSamples = samplesPerStep * 16 * bars + (int) renderRate;

    for (const auto& [amount, label] : settings)
    {
        std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
        plugin->setPlayConfigDetails (0, 2, renderRate, blockSize);
        plugin->prepareToPlay (renderRate, blockSize);
        setParam (*plugin, "derive", amount);
        setParam (*plugin, "poussiere", amount);

        juce::AudioBuffer<float> out (2, totalSamples);
        juce::AudioBuffer<float> block (2, blockSize);

        for (int start = 0; start < totalSamples; start += blockSize)
        {
            const auto n = std::min (blockSize, totalSamples - start);
            juce::MidiBuffer midi;
            for (int s = start; s < start + n; ++s)
            {
                if (s % samplesPerStep != 0 || s / samplesPerStep >= 16 * bars)
                    continue;
                const auto step = (s / samplesPerStep) % 16;
                for (int v = 0; v < 8; ++v)
                {
                    const auto c = pattern[v][step];
                    if (c == 'x' || c == 'X')
                        midi.addEvent (juce::MidiMessage::noteOn (10, 36 + v, (juce::uint8) (c == 'X' ? 127 : 80)), s - start);
                    if (v == 7 && step % 8 == 4) // DRONE released after half a bar
                        midi.addEvent (juce::MidiMessage::noteOff (10, 36 + v), s - start);
                }
            }

            block.setSize (2, n, false, false, true);
            plugin->processBlock (block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, start, block, ch, 0, n);
        }

        const auto file = outDir.getChildFile (juce::String ("motif_") + label + ".wav");
        file.deleteFile();
        std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (renderRate).withNumChannels (2).withBitsPerSample (24));
        if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (out, 0, out.getNumSamples()))
            return 1;

        std::cout << label << " (DERIVE/POUSSIERE " << amount << ")  peak "
                  << juce::Decibels::gainToDecibels (out.getMagnitude (0, out.getNumSamples())) << " dB\n";
    }
    return 0;
}
