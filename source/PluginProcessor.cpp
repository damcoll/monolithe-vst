#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int noteGrave = 36; // C1, first voice of the Ableton Drum Rack layout
}

MonolitheProcessor::MonolitheProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Sortie", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "MONOLITHE", createLayout())
{
    masterParam = parameters.getRawParameterValue ("master");
}

juce::AudioProcessorValueTreeState::ParameterLayout MonolitheProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master", 1 }, "MASTER", juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));
    return layout;
}

void MonolitheProcessor::prepareToPlay (double sampleRate, int)
{
    thump.prepare (sampleRate);
    masterGain.reset (sampleRate, 0.02);
    masterGain.setCurrentAndTargetValue (masterParam->load());
}

bool MonolitheProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MonolitheProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    masterGain.setTargetValue (masterParam->load());

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    int sample = 0;

    if (testTrigger.exchange (false))
    {
        thump.trigger (0.8f);
        noteCounter.fetch_add (1, std::memory_order_relaxed);
    }

    const auto renderUntil = [&] (int end)
    {
        for (; sample < end; ++sample)
        {
            const auto out = thump.nextSample() * masterGain.getNextValue();
            left[sample] = out;
            right[sample] = out;
        }
    };

    // render sample-accurately between MIDI events
    for (const auto meta : midi)
    {
        renderUntil (meta.samplePosition);
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && msg.getNoteNumber() == noteGrave)
        {
            thump.trigger (msg.getFloatVelocity());
            noteCounter.fetch_add (1, std::memory_order_relaxed);
        }
    }
    renderUntil (buffer.getNumSamples());
}

juce::AudioProcessorEditor* MonolitheProcessor::createEditor()
{
    return new MonolitheEditor (*this);
}

void MonolitheProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MonolitheProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MonolitheProcessor();
}
