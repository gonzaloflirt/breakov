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

State::State(AudioBuffer<float> b,
             const double sr,
             const int numSlices,
             const double fade)
  : buffer(b)
  , sampleRate(sr)
  , currentSliceProgress(0)
  , currentSliceIndex(0)
{
  makeSlices(numSlices, fade);
}

void State::makeSlices(const int numSlices, const double fade)
{
  slices.clear();
  const float fNumSamples =
    static_cast<float>(buffer.getNumSamples()) / static_cast<float>(numSlices);
  const int iNumSamples = static_cast<int>(fNumSamples);
  const int numChannels = buffer.getNumChannels();
  const int fadeSamples =
    std::min(static_cast<int>(sampleRate / 1000 * fade), iNumSamples - 1);
  currentSliceIndex %= numSlices;

  for (int i = 0; i < numSlices; ++i)
  {
    slices.push_back({numChannels, iNumSamples});
    for (int j = 0; j < numChannels; ++j)
    {
      const int read = static_cast<int>(fNumSamples * static_cast<float>(i));
      memcpy(slices.back().getWritePointer(j), buffer.getReadPointer(j, read),
             sizeof(float) * static_cast<std::size_t>(iNumSamples));
    }
    slices.back().applyGainRamp(0, fadeSamples, 0.f, 1.f);
    slices.back().applyGainRamp(iNumSamples - fadeSamples - 1, fadeSamples, 1.f, 0.f);
  }
}

StateChanged::StateChanged()
  : mFlag(true)
{
}

void StateChanged::set()
{
  mFlag.clear();
}

bool StateChanged::operator()()
{
  return mFlag.test_and_set();
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
  mParameters.createAndAddParameter(
    "sliceDur", "Beats per Slice", "", NormalisableRange<float>(0.f, 6.f), 2.f,
    [](float x) { return sliceDurNames()[static_cast<int>(x)]; }, nullptr);
  mParameters.createAndAddParameter(
    "fade", "Fade", "",
    NormalisableRange<float>(0.f, static_cast<float>(100.f), 0.f, 0.5f), 1.f,
    [](float x) { return String{x}; }, nullptr);

  for (int i = 0; i < maxNumSlices; ++i)
  {
    for (int j = 0; j < maxNumSlices; ++j)
    {
      const String parameterID = followProbId(i, j);
      mParameters.createAndAddParameter(
        parameterID, "Follow " + String(i + 1) + " -> " + String(j + 1), "",
        NormalisableRange<float>(0.f, 100.f), 10.f, [](float x) { return String{x}; },
        nullptr);
      pFollowProps[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
        mParameters.getParameter(parameterID);
    }
  }

  mParameters.addParameterListener("numSlices", this);
  mParameters.addParameterListener("fade", this);
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

  AudioPlayHead* playHead = AudioProcessor::getPlayHead();
  AudioPlayHead::CurrentPositionInfo positionInfo;

  if (state && playHead->getCurrentPosition(positionInfo))
  {
    const double sliceDuration = getSliceDuration();
    const double hostProgress =
      fmod(positionInfo.ppqPosition, sliceDuration) / sliceDuration;
    const double beatsPerSample = (positionInfo.bpm / 60.) / getSampleRate();
    const double slicePerSample = beatsPerSample / sliceDuration;
    double driftCompesation =
      positionInfo.isPlaying ? (1 - state->currentSliceProgress) / (1 - hostProgress) : 1;
    driftCompesation = driftCompesation < 0.5 ? driftCompesation + 1 : driftCompesation;

    AudioBuffer<float>& sliceBuffer =
      state->slices[static_cast<std::size_t>(state->currentSliceIndex)];

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
      const double index =
        state->currentSliceProgress * (sliceBuffer.getNumSamples() - 1);
      const float x = fmodf(static_cast<float>(index), 1);
      const int loIndex = static_cast<int>(floor(index));
      const int hiIndex =
        std::min(static_cast<int>(ceil(index)), sliceBuffer.getNumSamples() - 1);

      for (int channel = 0; channel < totalNumOutputChannels; ++channel)
      {
        const int bufChannel = channel & buffer.getNumChannels();
        const float a = sliceBuffer.getSample(bufChannel, loIndex);
        const float b = sliceBuffer.getSample(bufChannel, hiIndex);
        const float sample = a + x * (b - a);
        buffer.setSample(channel, i, sample);
      }

      state->currentSliceProgress += slicePerSample * driftCompesation;

      if (state->currentSliceProgress >= 1.)
      {
        driftCompesation = 1.;
        startNextSlice(state);
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
    pState = std::make_shared<State>(buffer, reader->sampleRate, getNumSlices(),
                                     getFadeDuration());
    mStateChanged.set();
  }
}

int Processor::getNumSlices() const
{
  return static_cast<int>(*mParameters.getRawParameterValue("numSlices"));
}

double Processor::getFadeDuration() const
{
  return static_cast<double>(*mParameters.getRawParameterValue("fade"));
}

int Processor::getSliceDurationIndex() const
{
  return static_cast<int>(*mParameters.getRawParameterValue("sliceDur"));
}

double Processor::getSliceDuration() const
{
  return sliceDurs()[static_cast<std::size_t>(getSliceDurationIndex())];
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

void Processor::parameterChanged(const String& parameterID, float)
{
  const int numSlices = getNumSlices();
  const double fade = getFadeDuration();

  StatePtr currentState = pState;

  if (currentState
      && (static_cast<int>(currentState->slices.size()) != numSlices
          || parameterID == "fade"))
  {
    StatePtr state = std::make_shared<State>(*currentState);
    state->makeSlices(numSlices, fade);
    pState = state;
  }
}

void Processor::startNextSlice(StatePtr state)
{
  const int numSlices = getNumSlices();
  const int slice = state->currentSliceIndex;
  std::vector<float> weights;
  for (int i = 0; i < numSlices; ++i)
  {
    weights.push_back(
      pFollowProps[static_cast<std::size_t>(slice)][static_cast<std::size_t>(i)]
        ->getValue());
  }
  std::discrete_distribution<> distribution(weights.begin(), weights.end());
  startSlice(state, distribution(randomGenerator));
}

void Processor::startSlice(StatePtr state, const int slice)
{
  state->currentSliceIndex = slice;
  state->currentSliceProgress = 0.;
  mStateChanged.set();
}

} // namespace breakov

POP_WARNINGS
