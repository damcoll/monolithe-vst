#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

// Step 1 placeholder sound: a short low "thump" on C1 (MIDI note 36).
// It proves the MIDI -> audio path; the real voices come in step 2.
class TestThump
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        // per-sample coefficients: pitch ~30 ms, amplitude ~400 ms (time to -60 dB)
        pitchDecay = (float) std::exp (-6.9 / (0.03 * sampleRate));
        ampDecay   = (float) std::exp (-6.9 / (0.4 * sampleRate));
    }

    void trigger (float velocity)
    {
        phase = 0.0;
        amplitude = velocity;
        pitchEnv = 1.0f;
        active = true;
    }

    float nextSample()
    {
        if (! active)
            return 0.0f;

        // pitch falls from 2x to 1x, amplitude decays exponentially
        const auto freq = 55.0 * (1.0 + pitchEnv);
        phase += juce::MathConstants<double>::twoPi * freq / sampleRate;
        if (phase > juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;

        const auto out = (float) std::sin (phase) * amplitude;

        pitchEnv *= pitchDecay;
        amplitude *= ampDecay;
        if (amplitude < 1.0e-4f)
            active = false;

        return out;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    float amplitude = 0.0f, pitchEnv = 0.0f;
    float pitchDecay = 0.0f, ampDecay = 0.0f;
    bool active = false;
};

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

    juce::AudioProcessorValueTreeState parameters;

    // read by the editor (message thread) to flash the MIDI LED
    std::atomic<int> noteCounter { 0 };

    // set by the editor's TEST button, consumed by the audio thread
    std::atomic<bool> testTrigger { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    TestThump thump;
    juce::SmoothedValue<float> masterGain;
    std::atomic<float>* masterParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonolitheProcessor)
};
