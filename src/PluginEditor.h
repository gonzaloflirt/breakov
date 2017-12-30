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
#include "PluginProcessor.h"
#include "Warnings.h"

PUSH_WARNINGS

namespace breakov
{
class Editor;

struct WaveDisplay : public Component
{
  WaveDisplay(Editor& e);

  void paint(Graphics& g) override;
  void paintGrid(Graphics& g, int numSlices);
  void paintEmpty(Graphics& g, int numSlices);
  void paintBuffer(Graphics& g, StatePtr state, int numSlices);
  void mouseDown(const MouseEvent& event) override;

  Editor& mEditor;
  MouseListener mouseListener;
};

template <typename Parameters, typename GetterUtil>
struct MultiSlider : public Component
{
  MultiSlider(Parameters&, GetterUtil);

  void paint(Graphics& g) override;
  void mouseDown(const MouseEvent& event) override;
  void mouseDrag(const MouseEvent& event) override;
  void handleMouse(int x, int y);

  Parameters& mParameters;
  GetterUtil mGetterUtil;
  MouseListener mMouseListener;
};

struct FollowGetterUtil
{
  FollowGetterUtil(Editor& editor);

  int slice();
  int numSliders();
  Colour colour(int slice);

  Editor& mEditor;
};

struct WarpGetterUtil
{
  WarpGetterUtil(Editor&);

  int slice();
  int numSliders();
  Colour colour(int slice);

  Editor& mEditor;
};

struct WarpDisplay : public Component
{
  WarpDisplay(Warp w);

  void paint(Graphics& g) override;

  Warp mWarp;
};

using Warps = std::array<Warp, numWarps>;

struct WarpDisplays : public Component
{
  WarpDisplays(Warps&);

  void paint(Graphics& g) override;

  Warps& mWarps;
  std::array<std::unique_ptr<WarpDisplay>, numWarps> mDisplays;
};

struct NiceLook : public LookAndFeel_V3
{
  void drawButtonBackground(Graphics&,
                            Button&,
                            const Colour& backgroundColour,
                            bool isMouseOverButton,
                            bool isButtonDown) override;

  void drawComboBox(Graphics& g,
                    int width,
                    int height,
                    bool isButtonDown,
                    int buttonX,
                    int buttonY,
                    int buttonW,
                    int buttonH,
                    ComboBox&) override;
};

class Editor : public AudioProcessorEditor,
               private Timer,
               private AudioProcessorValueTreeState::Listener,
               private Button::Listener,
               private ComboBox::Listener,
               private Slider::Listener
{
public:
  Editor(Processor&);
  ~Editor();

  void paint(Graphics&) override;
  void resized() override;

  StatePtr state() const;
  const Processor& processor() const;
  int slice();
  void setSlice(int);

private:
  void textButtonSetup(TextButton& button, String text);
  void comboBoxSetup(ComboBox& box, StringArray items);
  void sliderSetup(Slider& slider);
  void parameterChanged(const String& parameterID, float newValue) override;
  void buttonClicked(Button* button) override;
  void comboBoxChanged(ComboBox* box) override;
  void sliderValueChanged(Slider* slider) override;
  void timerCallback() override;
  void openFile();

  Processor& mProcessor;
  int mSlice;
  NiceLook mNiceLook;
  WaveDisplay mWaveDisplay;
  MultiSlider<FollowProbs, FollowGetterUtil> mFollowSlider;
  WarpDisplays mWarpDisplays;
  MultiSlider<WarpProbs, WarpGetterUtil> mWarpSlider;
  TextButton mOpenButton;
  ComboBox mNumSlicesBox;
  ComboBox mSliceDurBox;
  Slider mFadeSlider;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

} // namespace breakov

POP_WARNINGS
