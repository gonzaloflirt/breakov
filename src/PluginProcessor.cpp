/* breakov
 * Copyright (C) 2017 Florian Goltz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Warnings.h"

PUSH_WARNINGS

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new breakov::Processor();
}

namespace breakov
{

State::State(AudioBuffer<float> b)
  : buffer(b)
  , position(0)
{
}

Processor::Processor()
#ifndef JucePlugin_PreferredChannelConfigurations
  : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                     .withInput("Input", AudioChannelSet::stereo(), true)
#endif
                     .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
                     )
#endif
  , pState{}
{
}

Processor::~Processor()
{
}

const String Processor::getName() const
{
  return JucePlugin_Name;
}

bool Processor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool Processor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

double Processor::getTailLengthSeconds() const
{
  return 0.0;
}

int Processor::getNumPrograms()
{
  return 1;
}

int Processor::getCurrentProgram()
{
  return 0;
}

void Processor::setCurrentProgram(int)
{
}

const String Processor::getProgramName(int)
{
  return String();
}

void Processor::changeProgramName(int, const String&)
{
}

void Processor::prepareToPlay(double, int)
{
}

void Processor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
  ignoreUnused(layouts);
  return true;
#else
  if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
      && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
    return false;

#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void Processor::processBlock(AudioSampleBuffer& buffer, MidiBuffer&)
{
  const int totalNumInputChannels = getTotalNumInputChannels();
  const int totalNumOutputChannels = getTotalNumOutputChannels();

  for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  StatePtr state = pState;

  if (state)
  {
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
      for (int channel = 0; channel < totalNumOutputChannels; ++channel)
      {
        const auto sample = state->buffer.getSample(
          channel % state->buffer.getNumChannels(), state->position);
        buffer.setSample(channel, i, sample);
      }
      ++state->position %= state->buffer.getNumSamples();
    }
  }
}

void Processor::openFile(const File& file)
{
  AudioFormatManager formatManager;
  formatManager.registerBasicFormats();

  ScopedPointer<AudioFormatReader> reader(formatManager.createReaderFor(file));

  if (reader)
  {
    AudioBuffer<float> buffer(static_cast<int>(reader->numChannels),
                              static_cast<int>(reader->lengthInSamples));
    reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    pState = std::make_shared<State>(buffer);
  }
}

bool Processor::hasEditor() const
{
  return true;
}

AudioProcessorEditor* Processor::createEditor()
{
  return new Editor(*this);
}

void Processor::getStateInformation(MemoryBlock&)
{
}

void Processor::setStateInformation(const void*, int)
{
}

} // namespace breakov

POP_WARNINGS
