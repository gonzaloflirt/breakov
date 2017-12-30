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

State::State(AudioBuffer<float> b, const int numSlices)
  : buffer(b)
  , currentSlicePosition(0)
  , currentSliceIndex(0)
{
  makeSlices(numSlices);
}

void State::makeSlices(const int numSlices)
{
  slices.clear();
  const float fNumSamples =
    static_cast<float>(buffer.getNumSamples()) / static_cast<float>(numSlices);
  const int iNumSamples = static_cast<int>(fNumSamples);
  const int numChannels = buffer.getNumChannels();
  currentSliceIndex %= numSlices;
  currentSlicePosition %= iNumSamples;

  for (int i = 0; i < numSlices; ++i)
  {
    slices.push_back({numChannels, iNumSamples});
    for (int j = 0; j < numChannels; ++j)
    {
      const int read = static_cast<int>(fNumSamples * static_cast<float>(i));
      memcpy(slices.back().getWritePointer(j), buffer.getReadPointer(j, read),
             sizeof(float) * static_cast<std::size_t>(iNumSamples));
    }
  }
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
  , mParameters(*this, nullptr)
  , pState{}
{
  mParameters.createAndAddParameter(
    "numSlices", "Num Slices", "",
    NormalisableRange<float>(1.f, static_cast<float>(maxNumSlices)), 8.f,
    [](float x) { return String{static_cast<int>(x)}; }, nullptr);

  mParameters.addParameterListener("numSlices", this);
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
    AudioBuffer<float>& sliceBuffer =
      state->slices[static_cast<std::size_t>(state->currentSliceIndex)];

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
      for (int channel = 0; channel < totalNumOutputChannels; ++channel)
      {
        const auto sample = sliceBuffer.getSample(channel % sliceBuffer.getNumChannels(),
                                                  state->currentSlicePosition);
        buffer.setSample(channel, i, sample);
      }

      ++state->currentSlicePosition;

      if (state->currentSlicePosition >= sliceBuffer.getNumSamples())
      {
        startSlice(state, std::rand() % static_cast<int>(state->slices.size()));
      }
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
    pState = std::make_shared<State>(buffer, getNumSlices());
  }
}

int Processor::getNumSlices() const
{
  return static_cast<int>(*mParameters.getRawParameterValue("numSlices"));
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

void Processor::parameterChanged(const String&, float)
{
  const int numSlices = getNumSlices();
  StatePtr currentState = pState;
  if (currentState && static_cast<int>(pState->slices.size()) != numSlices)
  {
    StatePtr state = std::make_shared<State>(*currentState);
    state->makeSlices(numSlices);
    pState = state;
  }
}

void Processor::startSlice(StatePtr state, const int slice)
{
  state->currentSliceIndex = slice;
  state->currentSlicePosition = 0;
}

} // namespace breakov

POP_WARNINGS
