#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/Voices.h"

#include <atomic>

class MonolitheProcessor final : public juce::AudioProcessor
{
public:
    MonolitheProcessor();
    ~MonolitheProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // parameter IDs are stable: Ableton automation depends on them
    static juce::String paramId (int voice, const char* control) { return "v" + juce::String (voice + 1) + "_" + control; }

    juce::AudioProcessorValueTreeState parameters;

    // read by the editor (message thread): MIDI LED and last note received
    std::atomic<int> noteCounter { 0 };
    std::atomic<int> lastNote { -1 };
    std::atomic<int> lastVoice { -1 };

    // loudest sample since the editor last read them (read with exchange (0)): pads and VU needle
    std::array<std::atomic<float>, mono::numVoices> voicePeaks {};
    std::atomic<float> outputPeak { 0.0f };

    // set by the editor's audition buttons (one bit per voice), consumed by the audio thread
    std::atomic<uint32_t> auditionMask { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void trigger (int voice, float velocity);

    struct VoiceSlot
    {
        std::unique_ptr<mono::Voice> dsp;
        std::atomic<float>* niveau = nullptr;
        std::atomic<float>* accord = nullptr;
        std::atomic<float>* duree = nullptr;
        std::atomic<float>* mute = nullptr;
        juce::SmoothedValue<float> gain;
        int auditionSamplesLeft = 0; // audition buttons send no note-off: release after a while
    };

    std::array<VoiceSlot, mono::numVoices> voices;
    juce::SmoothedValue<float> masterGain;
    int auditionLength = 0;
    std::atomic<float>* masterParam = nullptr;
    std::atomic<float>* accentParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonolitheProcessor)
};
