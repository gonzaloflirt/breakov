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

Colour getSliceColour(const int slice, const int numSlices)
{
  const uint8 fac = static_cast<uint8>(static_cast<int>(UINT8_MAX) / numSlices * slice);
  return Colour(UINT8_MAX - fac, 4 * fac, fac);
}

} // namespace

WaveDisplay::WaveDisplay(Editor& e)
  : mEditor(e)
{
  this->addMouseListener(&mouseListener, true);
}

void WaveDisplay::paint(Graphics& g)
{
  const int numSlices = mEditor.processor().getNumSlices();
  StatePtr state = mEditor.state();
  const double sliceWidth =
    static_cast<double>(getWidth()) / static_cast<double>(numSlices);
  const int iSliceWidth = static_cast<int>(sliceWidth + 1);

  g.fillAll(Colours::black);

  g.setColour(Colours::grey);
  g.fillRect(static_cast<int>(mEditor.slice() * sliceWidth), 0, iSliceWidth, getHeight());

  if (state)
  {
    paintBuffer(g, state, numSlices);
  }
  else
  {
    paintEmpty(g, numSlices);
  }

  paintGrid(g, numSlices);

  if (state)
  {
    g.setColour(Colours::lightgrey);
    const int x = static_cast<int>(state->currentSliceIndex * sliceWidth + 1);
    g.drawRect(x, 0, iSliceWidth, getHeight());
  }
}

void WaveDisplay::paintGrid(Graphics& g, const int numSlices)
{
  const double sliceWidth =
    static_cast<double>(getWidth()) / static_cast<double>(numSlices);

  g.setColour(Colours::lightgrey);
  double i = 0;
  while (i < getWidth())
  {
    g.drawVerticalLine(static_cast<int>(i), 0, getHeight());
    i += sliceWidth;
  }
  g.drawVerticalLine(getWidth() - 1, 0, getHeight());
}

void WaveDisplay::paintBuffer(Graphics& g, StatePtr state, const int numSlices)
{
  const double sliceWidth =
    static_cast<double>(getWidth()) / static_cast<double>(numSlices);

  int samplesPerLine = state->buffer.getNumSamples() / getWidth();
  for (int i = 0; i < state->buffer.getNumSamples() && i < getWidth(); ++i)
  {
    float amp = 0;
    for (int j = 0; j < samplesPerLine; j++)
    {
      const float sample = fabsf(state->buffer.getSample(0, (i * samplesPerLine) + j));
      amp = std::max(amp, sample);
    }
    amp = amp / 2 * getHeight();


    g.setColour(getSliceColour(
      static_cast<int>(floor((static_cast<double>(i) / sliceWidth))), numSlices));
    g.drawVerticalLine(i, getHeight() / 2 - amp, getHeight() / 2 + amp);
  }
}

void WaveDisplay::paintEmpty(Graphics& g, const int numSlices)
{
  const double sliceWidth =
    static_cast<double>(getWidth()) / static_cast<double>(numSlices);
  const int iSliceWidth = static_cast<int>(sliceWidth + 1);
  for (int i = 0; i < numSlices; ++i)
  {
    g.setColour(getSliceColour(i, numSlices));
    g.drawHorizontalLine(getHeight() / 2, static_cast<int>(i * sliceWidth),
                         (i + 1) * iSliceWidth);
  }
}

void WaveDisplay::mouseDown(const MouseEvent& event)
{
  mEditor.setSlice(event.x * mEditor.processor().getNumSlices() / getWidth());
}

template <typename Parameters, typename GetterUtil>
MultiSlider<Parameters, GetterUtil>::MultiSlider(Parameters& p, GetterUtil g)
  : mParameters(p)
  , mGetterUtil(g)
{
  this->addMouseListener(&mMouseListener, true);
}

template <typename Parameters, typename GetterUtil>
void MultiSlider<Parameters, GetterUtil>::paint(Graphics& g)
{
  g.fillAll(Colours::grey);

  const int numSliders = mGetterUtil.numSliders();
  const int slice = mGetterUtil.slice();

  const float sliderWidth =
    static_cast<float>(getWidth()) / static_cast<float>(numSliders);
  const float height = getHeight();
  for (int i = 0; i < numSliders; ++i)
  {
    g.setColour(mGetterUtil.colour(i));
    const float val =
      mParameters[static_cast<std::size_t>(slice)][static_cast<std::size_t>(i)]
        ->getValue();
    g.fillRect(i * sliderWidth, (1 - val) * height, sliderWidth, height * val);
  }

  g.setColour(Colours::darkgrey);
  for (int i = 1; i < numSliders; ++i)
  {
    g.drawVerticalLine(static_cast<int>(i * sliderWidth), 0, height);
  }
}

template <typename Parameters, typename GetterUtil>
void MultiSlider<Parameters, GetterUtil>::mouseDown(const MouseEvent& event)
{
  handleMouse(event.x, event.y);
}

template <typename Parameters, typename GetterUtil>
void MultiSlider<Parameters, GetterUtil>::mouseDrag(const MouseEvent& event)
{
  handleMouse(event.x, event.y);
}

template <typename Parameters, typename GetterUtil>
void MultiSlider<Parameters, GetterUtil>::handleMouse(const int x, const int y)
{
  const int numSliders = mGetterUtil.numSliders();
  const int slice = mGetterUtil.slice();
  const int slider = x * numSliders / getWidth();
  const float val = 1.f - static_cast<float>(y) / static_cast<float>(getHeight());
  mParameters[static_cast<std::size_t>(slice)][static_cast<std::size_t>(slider)]
    ->setValueNotifyingHost(val);
}

FollowGetterUtil::FollowGetterUtil(Editor& e)
  : mEditor(e)
{
}

int FollowGetterUtil::slice()
{
  return mEditor.slice();
}

int FollowGetterUtil::numSliders()
{
  return mEditor.processor().getNumSlices();
}

Colour FollowGetterUtil::colour(const int slice)
{
  return getSliceColour(slice, numSliders());
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
  , mSlice(0)
  , mWaveDisplay(*this)
  , mFollowSlider(p.pFollowProps, FollowGetterUtil(*this))
  , mFadeSlider(Slider::SliderStyle::LinearBar, Slider::TextEntryBoxPosition::NoTextBox)
{
  addAndMakeVisible(mWaveDisplay);
  addAndMakeVisible(mFollowSlider);

  textButtonSetup(mOpenButton, "open audio file");

  comboBoxSetup(mNumSlicesBox, sliceNames());
  mNumSlicesBox.setSelectedId(mProcessor.getNumSlices(),
                              NotificationType::dontSendNotification);

  comboBoxSetup(mSliceDurBox, sliceDurNames());
  mSliceDurBox.setSelectedId(
    static_cast<int>(*mProcessor.mParameters.getRawParameterValue("sliceDur")) + 1,
    NotificationType::dontSendNotification);

  sliderSetup(mFadeSlider);
  mFadeSlider.setValue(mProcessor.getFadeDuration(),
                       NotificationType::dontSendNotification);

  mProcessor.mParameters.addParameterListener("numSlices", this);
  mProcessor.mParameters.addParameterListener("sliceDur", this);
  mProcessor.mParameters.addParameterListener("fade", this);

  for (int i = 0; i < maxNumSlices; ++i)
  {
    for (int j = 0; j < maxNumSlices; ++j)
    {
      mProcessor.mParameters.addParameterListener(followProbId(i, j), this);
    }
  }

  setSize(600, 300);
  startTimer(30);
}

Editor::~Editor()
{
  mProcessor.mParameters.removeParameterListener("numSlices", this);
  mProcessor.mParameters.removeParameterListener("sliceDur", this);
  mProcessor.mParameters.removeParameterListener("fade", this);

  for (int i = 0; i < maxNumSlices; ++i)
  {
    for (int j = 0; j < maxNumSlices; ++j)
    {
      mProcessor.mParameters.removeParameterListener(followProbId(i, j), this);
    }
  }
}

void Editor::paint(Graphics& g)
{
  g.fillAll(Colours::darkgrey);
  const int numSlices = mProcessor.getNumSlices();
  g.setColour(getSliceColour(mSlice, numSlices));
  g.setFont(Font("Arial", 8.0f, Font::plain));
  g.drawHorizontalLine(150, 10, getWidth() - 10);
  g.setColour(Colours::white);
  g.drawText("follow propabilities slice " + String(mSlice + 1), 10, 140, 200, 10,
             Justification::left);
  g.drawText("number of slices", getWidth() - 70, 35, 60, 10, Justification::left);
  g.drawText("beats per slice", getWidth() - 70, 70, 60, 10, Justification::left);
  g.drawText("fade duration", getWidth() - 70, 105, 60, 10, Justification::left);
}

void Editor::resized()
{
  mWaveDisplay.setBounds(10, 10, getWidth() - 100, 125);
  mFollowSlider.setBounds(10, 155, getWidth() - 100, 100);
  mOpenButton.setBounds(getWidth() - 70, 10, 60, 20);
  mNumSlicesBox.setBounds(getWidth() - 70, 45, 60, 20);
  mSliceDurBox.setBounds(getWidth() - 70, 80, 60, 20);
  mFadeSlider.setBounds(getWidth() - 70, 115, 60, 20);
}

StatePtr Editor::state() const
{
  return mProcessor.pState;
}

const Processor& Editor::processor() const
{
  return mProcessor;
}

int Editor::slice()
{
  return mSlice;
}

void Editor::setSlice(int slice)
{
  mSlice = slice;
  repaint();
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

    if (mSlice >= newValue)
    {
      mSlice = static_cast<int>(newValue) - 1;
    }
    repaint();
  }
  if (parameterID == "sliceDur")
  {
    mSliceDurBox.setSelectedId(static_cast<int>(newValue) + 1,
                               NotificationType::dontSendNotification);
  }
  else if (parameterID == "fade")
  {
    mFadeSlider.setValue(static_cast<double>(newValue),
                         NotificationType::dontSendNotification);
  }
  else if (parameterID.contains("followProb_" + String(mSlice)))
  {
    mFollowSlider.repaint();
  }
}

void Editor::buttonClicked(Button*)
{
  openFile();
}

void Editor::comboBoxChanged(ComboBox* box)
{
  if (box == &mNumSlicesBox)
  {
    const float value =
      static_cast<float>(box->getSelectedId()) / static_cast<float>(maxNumSlices);
    mProcessor.mParameters.getParameter("numSlices")->setValueNotifyingHost(value);
  }
  else if (box == &mSliceDurBox)
  {
    const float value =
      static_cast<float>(box->getSelectedId()) / static_cast<float>(sliceDurs().size());
    mProcessor.mParameters.getParameter("sliceDur")->setValueNotifyingHost(value);
  }
}

void Editor::sliderValueChanged(Slider* slider)
{
  mProcessor.mParameters.getParameter("fade")->setValueNotifyingHost(
    static_cast<float>(slider->getValue()) / 100.f);
}

void Editor::timerCallback()
{
  if (mProcessor.mStateChanged())
  {
    mWaveDisplay.repaint();
  }
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
