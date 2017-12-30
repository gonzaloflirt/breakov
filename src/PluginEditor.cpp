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
#include <random>

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

std::default_random_engine& generator()
{
  static std::default_random_engine generator;
  return generator;
}

float getRandomValue()
{
  std::exponential_distribution<float> distribution(2.);
  return fmodf(distribution(generator()), 1.f);
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

WarpGetterUtil::WarpGetterUtil(Editor& e)
  : mEditor(e)
{
}

int WarpGetterUtil::slice()
{
  return mEditor.slice();
}

int WarpGetterUtil::numSliders()
{
  return numWarps;
}

Colour WarpGetterUtil::colour(int)
{
  return getSliceColour(slice(), mEditor.processor().getNumSlices());
}

WarpDisplay::WarpDisplay(Warp w)
  : mWarp(w)
{
}

void WarpDisplay::paint(Graphics& g)
{
  const double numPoints = 1000;
  Path path;
  path.startNewSubPath(0, static_cast<float>(mWarp(0) * numPoints));
  for (int i = 1; i < numPoints; ++i)
  {
    path.lineTo(i, static_cast<float>(mWarp(i / numPoints) * numPoints));
  }
  AffineTransform transform = AffineTransform::fromTargetPoints(
    0, numPoints, 0, 0, 0, 0, 0, getHeight(), numPoints, 0, getWidth(), getHeight());
  path.applyTransform(transform);
  g.setColour(Colours::white);
  g.strokePath(path, PathStrokeType(1.));
}

WarpDisplays::WarpDisplays(Warps& w)
  : mWarps(w)
{
  for (std::size_t i = 0; i < mDisplays.size() - 1; ++i)
  {
    mDisplays[i] = std::unique_ptr<WarpDisplay>(new WarpDisplay(mWarps[i]));
    addAndMakeVisible(*mDisplays[i]);
  }
  mDisplays.back() =
    std::unique_ptr<WarpDisplay>(new WarpDisplay([](double) { return 0.5; }));
  addAndMakeVisible(*mDisplays.back());
}

void WarpDisplays::paint(Graphics& g)
{
  g.fillAll(Colours::grey);

  const double width = static_cast<double>(getWidth()) / static_cast<double>(numWarps);

  for (int i = 0; i < numWarps; ++i)
  {
    mDisplays[static_cast<std::size_t>(i)]->setBounds(
      static_cast<int>(1 + i * width), 0, static_cast<int>(width - 1), getHeight());
    mDisplays[static_cast<std::size_t>(i)]->repaint();
  }

  g.setColour(Colours::darkgrey);
  for (int i = 1; i < numWarps; ++i)
  {
    g.drawVerticalLine(static_cast<int>(i * width), 0, getHeight());
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
  , mSlice(0)
  , mWaveDisplay(*this)
  , mFollowSlider(p.pFollowProps, FollowGetterUtil(*this))
  , mWarpDisplays(p.mWarps)
  , mWarpSlider(p.pWarpProps, WarpGetterUtil(*this))
  , mFadeSlider(Slider::SliderStyle::LinearBar, Slider::TextEntryBoxPosition::NoTextBox)
{
  addAndMakeVisible(mWaveDisplay);
  addAndMakeVisible(mFollowSlider);
  addAndMakeVisible(mWarpDisplays);
  addAndMakeVisible(mWarpSlider);

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

  textButtonSetup(mFollowRandomizeThisButton, "randomize this slice");
  textButtonSetup(mFollowRandomizeAllButton, "randomize all slices");
  textButtonSetup(mFollowCopyToAllButton, "copy to all slices");
  textButtonSetup(mFollowLinearButton, "linear playback");
  textButtonSetup(mWarpRandomizeThisButton, "randomize this slice");
  textButtonSetup(mWarpRandomizeAllButton, "randomize all slices");
  textButtonSetup(mWarpCopyToAllButton, "copy to all slices");

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

  for (int i = 0; i < maxNumSlices; ++i)
  {
    for (int j = 0; j < numWarps; ++j)
    {
      mProcessor.mParameters.addParameterListener(warpProbId(i, j), this);
    }
  }

  setSize(600, 405);
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

  for (int i = 0; i < maxNumSlices; ++i)
  {
    for (int j = 0; j < numWarps; ++j)
    {
      mProcessor.mParameters.removeParameterListener(warpProbId(i, j), this);
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
  g.drawHorizontalLine(270, 10, getWidth() - 10);
  g.setColour(Colours::white);
  g.drawText("follow propabilities slice " + String(mSlice + 1), 10, 140, 200, 10,
             Justification::left);
  g.drawText("warp propabilities slice " + String(mSlice + 1), 10, 260, 200, 10,
             Justification::left);
  g.drawText("number of slices", getWidth() - 70, 35, 60, 10, Justification::left);
  g.drawText("beats per slice", getWidth() - 70, 70, 60, 10, Justification::left);
  g.drawText("fade duration", getWidth() - 70, 105, 60, 10, Justification::left);
}

void Editor::resized()
{
  mWaveDisplay.setBounds(10, 10, getWidth() - 100, 125);
  mFollowSlider.setBounds(10, 155, getWidth() - 100, 100);
  mWarpDisplays.setBounds(10, 275, getWidth() - 100, 20);
  mWarpSlider.setBounds(10, 300, getWidth() - 100, 100);
  mOpenButton.setBounds(getWidth() - 70, 10, 60, 20);
  mNumSlicesBox.setBounds(getWidth() - 70, 45, 60, 20);
  mSliceDurBox.setBounds(getWidth() - 70, 80, 60, 20);
  mFadeSlider.setBounds(getWidth() - 70, 115, 60, 20);
  mFollowRandomizeThisButton.setBounds(getWidth() - 70, 155, 60, 20);
  mFollowRandomizeAllButton.setBounds(getWidth() - 70, 180, 60, 20);
  mFollowCopyToAllButton.setBounds(getWidth() - 70, 205, 60, 20);
  mFollowLinearButton.setBounds(getWidth() - 70, 230, 60, 20);
  mWarpRandomizeThisButton.setBounds(getWidth() - 70, 275, 60, 20);
  mWarpRandomizeAllButton.setBounds(getWidth() - 70, 300, 60, 20);
  mWarpCopyToAllButton.setBounds(getWidth() - 70, 325, 60, 20);
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
  else if (parameterID.contains("warpProb_" + String(mSlice)))
  {
    mWarpSlider.repaint();
  }
}

void Editor::buttonClicked(Button* button)
{
  if (button == &mOpenButton)
  {
    openFile();
  }
  else if (button == &mFollowRandomizeThisButton)
  {
    randomizeThisSlice(mProcessor.pFollowProps);
  }
  else if (button == &mFollowRandomizeAllButton)
  {
    randomizeAllSlices(mProcessor.pFollowProps);
  }
  else if (button == &mFollowCopyToAllButton)
  {
    copyToAllSlices(mProcessor.pFollowProps);
  }
  else if (button == &mFollowLinearButton)
  {
    setFollowChancesToLinear();
  }
  else if (button == &mWarpRandomizeThisButton)
  {
    randomizeThisSlice(mProcessor.pWarpProps);
  }
  else if (button == &mWarpRandomizeAllButton)
  {
    randomizeAllSlices(mProcessor.pWarpProps);
  }
  else if (button == &mWarpCopyToAllButton)
  {
    copyToAllSlices(mProcessor.pWarpProps);
  }
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

template <typename Parameters>
void Editor::randomizeThisSlice(Parameters p)
{
  auto& array = p[static_cast<std::size_t>(mSlice)];
  std::for_each(array.begin(), array.end(), [](AudioProcessorParameter* par) {
    par->setValueNotifyingHost(getRandomValue());
  });
}

template <typename Parameters>
void Editor::randomizeAllSlices(Parameters p)
{
  std::for_each(p.begin(), p.end(), [](decltype(p.front()) pars) {
    std::for_each(pars.begin(), pars.end(), [](AudioProcessorParameter* par) {
      par->setValueNotifyingHost(getRandomValue());
    });
  });
}

template <typename Parameters>
void Editor::copyToAllSlices(Parameters p)
{
  auto& data = p[static_cast<std::size_t>(mSlice)];

  std::for_each(p.begin(), p.end(), [&data](decltype(p.front()) pars) {
    auto it = data.begin();
    std::for_each(pars.begin(), pars.end(), [&it](AudioProcessorParameter* par) {
      par->setValueNotifyingHost((*it++)->getValue());
    });
  });
}

void Editor::setFollowChancesToLinear()
{
  const int numSlices = mProcessor.getNumSlices();
  for (int i = 0; i < maxNumSlices; ++i)
  {
    for (int j = 0; j < maxNumSlices; ++j)
    {
      const bool set = i + 1 == j || (j == 0 && i == numSlices - 1);
      mProcessor.pFollowProps[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]
        ->setValueNotifyingHost(set ? 100 : 0);
    }
  }
}

} // namespace breakov

POP_WARNINGS
