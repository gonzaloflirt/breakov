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

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "Warnings.h"
#include <array>

PUSH_WARNINGS

AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace breakov
{
const static int maxNumSlices = 32;

const static std::array<double, 7> sliceDurs()
{
  return {{4, 2, 1, 0.5, 0.25, 0.125, 0.0625}};
}

static StringArray sliceDurNames()
{
  return {"4", "2", "1", "1/2", "1/4", "1/8", "1/16"};
}

struct State
{
  State(AudioBuffer<float> b, double sr, int numSlices, double fade);

  void makeSlices(int numSlices, double fade);

  AudioBuffer<float> buffer;
  std::vector<AudioBuffer<float>> slices;
  double sampleRate;
  double currentSliceProgress;
  int currentSliceIndex;
};

class Processor : public AudioProcessor, private AudioProcessorValueTreeState::Listener
{
public:
  Processor();
  ~Processor();

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

  void processBlock(AudioSampleBuffer&, MidiBuffer&) override;

  AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const String getProgramName(int index) override;
  void changeProgramName(int index, const String& newName) override;

  void getStateInformation(MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  void openFile(const File& file);
  int getNumSlices() const;
  double getFadeDuration() const;
  int getSliceDurationIndex() const;
  double getSliceDuration() const;

  AudioProcessorValueTreeState mParameters;

private:
  using StatePtr = std::shared_ptr<State>;

  void parameterChanged(const String& parameterID, float newValue) override;
  void startSlice(StatePtr state, int slice);

  StatePtr pState;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};

} // namespace breakov

POP_WARNINGS
