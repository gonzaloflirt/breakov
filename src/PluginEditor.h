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

private:
  void textButtonSetup(TextButton& button, String text);
  void comboBoxSetup(ComboBox& box, StringArray items);
  void sliderSetup(Slider& slider);
  void parameterChanged(const String& parameterID, float newValue) override;
  void buttonClicked(Button* button) override;
  void comboBoxChanged(ComboBox* box) override;
  void sliderValueChanged(Slider* slider) override;
  void openFile();

  Processor& mProcessor;
  NiceLook mNiceLook;
  TextButton mOpenButton;
  ComboBox mNumSlicesBox;
  ComboBox mSliceDurBox;
  Slider mFadeSlider;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

} // namespace breakov

POP_WARNINGS
