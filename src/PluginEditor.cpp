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

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Warnings.h"
#include <array>

PUSH_WARNINGS

namespace breakov
{

namespace
{
StringArray sliceNames()
{
  StringArray sliceNames;
  for (int i = 1; i <= maxNumSlices; ++i)
  {
    sliceNames.add(String(i));
  }
  return sliceNames;
}
}

void NiceLook::drawButtonBackground(
  Graphics& g, Button& b, const Colour& backgroundColour, bool, bool isButtonDown)
{
  g.setColour(backgroundColour.interpolatedWith(Colours::white, isButtonDown ? 0.2f : 0));
  g.fillRect(0, 0, b.getWidth(), b.getHeight());
  g.setColour(Colours::white);
  g.drawRect(0, 0, b.getWidth(), b.getHeight());
}

void NiceLook::drawComboBox(
  Graphics& g, int width, int height, bool, int, int, int, int, ComboBox&)
{
  g.setColour(Colours::white);
  g.drawRect(0, 0, width, height);
}

Editor::Editor(Processor& p)
  : AudioProcessorEditor(&p)
  , mProcessor(p)
  , mFadeSlider(Slider::SliderStyle::LinearBar, Slider::TextEntryBoxPosition::NoTextBox)
{
  textButtonSetup(mOpenButton, "open audio file");

  comboBoxSetup(mNumSlicesBox, sliceNames());
  mNumSlicesBox.setSelectedId(mProcessor.getNumSlices(),
                              NotificationType::dontSendNotification);

  sliderSetup(mFadeSlider);
  mFadeSlider.setValue(mProcessor.getFadeDuration(),
                       NotificationType::dontSendNotification);

  mProcessor.mParameters.addParameterListener("numSlices", this);
  mProcessor.mParameters.addParameterListener("fade", this);

  setSize(400, 300);
}

Editor::~Editor()
{
  mProcessor.mParameters.removeParameterListener("numSlices", this);
  mProcessor.mParameters.removeParameterListener("fade", this);
}

void Editor::paint(Graphics& g)
{
  g.fillAll(Colours::darkgrey);
  g.setColour(Colours::white);
  g.setFont(Font("Arial", 8.0f, Font::plain));
  g.drawText("number of slices", getWidth() - 70, 35, 60, 10, Justification::left);
  g.drawText("fade duration", getWidth() - 70, 70, 60, 10, Justification::left);
}

void Editor::resized()
{
  mOpenButton.setBounds(getWidth() - 70, 10, 60, 20);
  mNumSlicesBox.setBounds(getWidth() - 70, 45, 60, 20);
  mFadeSlider.setBounds(getWidth() - 70, 80, 60, 20);
}

void Editor::textButtonSetup(TextButton& button, String text)
{
  button.setButtonText(text);
  button.setColour(TextButton::ColourIds::textColourOffId, Colours::white);
  button.setColour(TextButton::ColourIds::buttonColourId, Colours::darkgrey);
  button.setLookAndFeel(&mNiceLook);
  addAndMakeVisible(&button);
  button.addListener(this);
}

void Editor::comboBoxSetup(ComboBox& box, StringArray items)
{
  box.addItemList(items, 1);
  box.setColour(ComboBox::ColourIds::textColourId, Colours::white);
  box.setColour(ComboBox::ColourIds::outlineColourId, Colours::white);
  box.setColour(ComboBox::ColourIds::backgroundColourId, Colours::darkgrey);
  box.setColour(ComboBox::ColourIds::arrowColourId, Colours::white);
  box.setLookAndFeel(&mNiceLook);
  box.addListener(this);
  addAndMakeVisible(box);
}

void Editor::sliderSetup(Slider& slider)
{
  slider.setSkewFactor(0.5);
  slider.setRange(0, 100);
  slider.setColour(Slider::ColourIds::thumbColourId, Colours::white);
  slider.setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::white);
  slider.setLookAndFeel(&mNiceLook);
  slider.addListener(this);
  addAndMakeVisible(slider);
}

void Editor::parameterChanged(const String& parameterID, float newValue)
{
  MessageManagerLock lock;

  if (parameterID == "numSlices")
  {
    mNumSlicesBox.setSelectedId(static_cast<int>(newValue),
                                NotificationType::dontSendNotification);
  }
  else if (parameterID == "fade")
  {
    mFadeSlider.setValue(static_cast<double>(newValue),
                         NotificationType::dontSendNotification);
  }
}

void Editor::buttonClicked(Button*)
{
  openFile();
}

void Editor::comboBoxChanged(ComboBox* box)
{
  const float value =
    static_cast<float>(box->getSelectedId()) / static_cast<float>(maxNumSlices);
  mProcessor.mParameters.getParameter("numSlices")->setValueNotifyingHost(value);
}

void Editor::sliderValueChanged(Slider* slider)
{
  mProcessor.mParameters.getParameter("fade")->setValueNotifyingHost(
    static_cast<float>(slider->getValue()) / 100.f);
}

void Editor::openFile()
{
  FileChooser chooser("Select an Audio File", File::nonexistent, "*.wav, *.aif, *.aiff");
  if (chooser.browseForFileToOpen())
  {
    mProcessor.openFile(chooser.getResult());
  }
}

} // namespace breakov

POP_WARNINGS
