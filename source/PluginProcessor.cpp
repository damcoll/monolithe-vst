#include "PluginProcessor.h"
#include "PluginEditor.h"

MonolitheProcessor::MonolitheProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Sortie", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "MONOLITHE", createLayout())
{
    for (int i = 0; i < mono::numVoices; ++i)
    {
        auto& v = voices[(size_t) i];
        v.dsp = mono::makeVoice (i);
        v.niveau = parameters.getRawParameterValue (paramId (i, "niveau"));
        v.accord = parameters.getRawParameterValue (paramId (i, "accord"));
        v.duree = parameters.getRawParameterValue (paramId (i, "duree"));
        v.mute = parameters.getRawParameterValue (paramId (i, "mute"));
    }
    masterParam = parameters.getRawParameterValue ("master");
    accentParam = parameters.getRawParameterValue ("accent");
}

juce::AudioProcessorValueTreeState::ParameterLayout MonolitheProcessor::createLayout()
{
    using Float = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const juce::NormalisableRange<float> unit (0.0f, 1.0f);

    for (int i = 0; i < mono::numVoices; ++i)
    {
        const auto prefix = juce::String::formatted ("%02d ", i + 1) + juce::String::fromUTF8 (mono::voiceNames[(size_t) i]) + " ";
        layout.add (std::make_unique<Float> (juce::ParameterID { paramId (i, "niveau"), 1 }, prefix + "NIVEAU", unit, 0.8f));
        layout.add (std::make_unique<Float> (juce::ParameterID { paramId (i, "accord"), 1 }, prefix + "ACCORD", unit, 0.5f));
        layout.add (std::make_unique<Float> (juce::ParameterID { paramId (i, "duree"), 1 }, prefix + juce::String::fromUTF8 ("DURÉE"), unit, 0.5f));
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { paramId (i, "mute"), 1 }, prefix + "MUTE", false));
    }

    // DÉRIVE and POUSSIÈRE are declared now so the parameter list stays stable; they act from step 4
    layout.add (std::make_unique<Float> (juce::ParameterID { "derive", 1 }, juce::String::fromUTF8 ("DÉRIVE"), unit, 0.2f));
    layout.add (std::make_unique<Float> (juce::ParameterID { "poussiere", 1 }, juce::String::fromUTF8 ("POUSSIÈRE"), unit, 0.2f));
    layout.add (std::make_unique<Float> (juce::ParameterID { "accent", 1 }, "ACCENT", unit, 0.7f));
    layout.add (std::make_unique<Float> (juce::ParameterID { "master", 1 }, "MASTER", unit, 0.8f));
    return layout;
}

void MonolitheProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& v : voices)
    {
        if (v.dsp != nullptr)
            v.dsp->prepare (sampleRate);
        v.gain.reset (sampleRate, 0.01);
        v.gain.setCurrentAndTargetValue (v.mute->load() > 0.5f ? 0.0f : v.niveau->load());
    }
    masterGain.reset (sampleRate, 0.02);
    masterGain.setCurrentAndTargetValue (masterParam->load());
}

bool MonolitheProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MonolitheProcessor::trigger (int voice, float velocity)
{
    auto& v = voices[(size_t) voice];
    if (v.dsp == nullptr)
        return;

    // ACCENT = how much the velocity matters: 0 -> every hit is full, 1 -> fully dynamic
    const auto accent = accentParam->load();
    v.dsp->setControls (v.accord->load(), v.duree->load());
    v.dsp->trigger (1.0f - accent * (1.0f - velocity));
}

void MonolitheProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    for (auto& v : voices)
        v.gain.setTargetValue (v.mute->load() > 0.5f ? 0.0f : v.niveau->load());
    masterGain.setTargetValue (masterParam->load());

    if (const auto mask = auditionMask.exchange (0); mask != 0)
    {
        for (int i = 0; i < mono::numVoices; ++i)
            if ((mask >> i) & 1u)
                trigger (i, 0.8f);
        noteCounter.fetch_add (1, std::memory_order_relaxed);
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    int sample = 0;

    const auto renderUntil = [&] (int end)
    {
        for (; sample < end; ++sample)
        {
            float mix = 0.0f;
            for (auto& v : voices)
            {
                const auto g = v.gain.getNextValue();
                if (v.dsp != nullptr)
                    mix += v.dsp->next() * g;
            }
            const auto out = mix * masterGain.getNextValue();
            left[sample] = out;
            right[sample] = out;
        }
    };

    // render sample-accurately between MIDI events
    for (const auto meta : midi)
    {
        renderUntil (meta.samplePosition);
        const auto msg = meta.getMessage();
        const auto voice = msg.getNoteNumber() - mono::firstNote;
        const auto inRange = voice >= 0 && voice < mono::numVoices;

        if (msg.isNoteOn())
        {
            lastNote.store (msg.getNoteNumber(), std::memory_order_relaxed);
            noteCounter.fetch_add (1, std::memory_order_relaxed);
            if (inRange)
                trigger (voice, msg.getFloatVelocity());
        }
        else if (msg.isNoteOff() && inRange && voices[(size_t) voice].dsp != nullptr)
        {
            voices[(size_t) voice].dsp->release();
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
