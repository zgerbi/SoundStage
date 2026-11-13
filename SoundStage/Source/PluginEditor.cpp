/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "PluginProcessor.h"
#include "juce_graphics/juce_graphics.h"
#include "math.h"

//==============================================================================
SoundStageAudioProcessorEditor::SoundStageAudioProcessorEditor(
    SoundStageAudioProcessor &p, juce::AudioProcessorValueTreeState &apvts)
    : AudioProcessorEditor(&p), audioProcessor(p),
      audioProcessorValueTreeState(apvts) {
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.

  // TYPEFACE
  auto font = juce::Typeface::createSystemTypefaceFor(
      BinaryData::Fraunces_72ptSemiBold_ttf,
      BinaryData::Fraunces_72ptSemiBold_ttfSize);
  juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface(font);

  // COLO(U)R SCHEME SETTINGS
  getLookAndFeel().setColour(juce::Slider::thumbColourId,
                             juce::Colours::firebrick);
  getLookAndFeel().setColour(juce::Slider::trackColourId,
                             juce::Colours::saddlebrown);
  getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId,
                             juce::Colours::transparentBlack);
  getLookAndFeel().setColour(juce::Slider::backgroundColourId,
                             juce::Colours::ivory);
  getLookAndFeel().setColour(juce::Slider::rotarySliderOutlineColourId,
                             juce::Colours::sienna);
  getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId,
                             juce::Colours::darkgrey);

  juce::Colour text_colour = juce::Colours::darkgrey;

  getLookAndFeel().setColour(juce::Label::textColourId, text_colour);
  getLookAndFeel().setColour(juce::Slider::textBoxTextColourId, text_colour);
  getLookAndFeel().setColour(juce::Slider::textBoxBackgroundColourId,
                             juce::Colours::transparentBlack);
  getLookAndFeel().setColour(juce::Slider::textBoxOutlineColourId,
                             juce::Colours::transparentBlack);
  getLookAndFeel().setColour(juce::Slider::textBoxHighlightColourId,
                             juce::Colours::crimson);

  // ELEVATION SLIDER SETTINGS
  elevationControl.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
  elevationControl.setRange(-45.f, 90.f, 5.f);
  elevationControl.setValue(0.f);
  elevationControl.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 28);
  elevationControl.addListener(this);
  addAndMakeVisible(elevationControl);
  elevationControlAttachment.reset(
      new juce::AudioProcessorValueTreeState::SliderAttachment(
          apvts, "elevation", elevationControl));

  // AZIMUTH SLIDER SETTINGS
  azimuthControl.setSliderStyle(juce::Slider::SliderStyle::Rotary);
  azimuthControl.setRange(-180.f, 180.f, 1.f);
  azimuthControl.setRotaryParameters(2 * acos(0.0f), 6 * acos(0.0f), false);
  azimuthControl.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 28);
  azimuthControl.addListener(this);
  addAndMakeVisible(azimuthControl);
  azimuthControlAttachment.reset(
      new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "azimuth",
                                                               azimuthControl));

  // LABEL SETTINGS
  azLabel.setText("Azimuth", juce::NotificationType::dontSendNotification);
  elLabel.setText("Elevation", juce::NotificationType::dontSendNotification);
  azLabel.setFont(azLabel.getFont().withHeight(18.0f));
  elLabel.setFont(elLabel.getFont().withHeight(18.0f));
  azLabel.setEditable(false);
  elLabel.setEditable(false);
  azLabel.setJustificationType(juce::Justification::centred);
  elLabel.setJustificationType(juce::Justification::bottomLeft);
  azLabel.attachToComponent(&azimuthControl, false);
  elLabel.attachToComponent(&elevationControl, true);
  addAndMakeVisible(azLabel);
  addAndMakeVisible(elLabel);

  setSize(400, 300);

  // IMAGES
  azRefImg = juce::ImageCache::getFromMemory(
      BinaryData::azimuth_reference_png, BinaryData::azimuth_reference_pngSize);
  elRefImg =
      juce::ImageCache::getFromMemory(BinaryData::elevation_reference_png,
                                      BinaryData::elevation_reference_pngSize);
}

SoundStageAudioProcessorEditor::~SoundStageAudioProcessorEditor() {}

//==============================================================================
void SoundStageAudioProcessorEditor::paint(juce::Graphics &g) {

  // Solid background
  juce::ColourGradient grad1(juce::Colours::linen, 200, 150, juce::Colours::tan,
                             600, 400, 1);
  g.setGradientFill(grad1);
  g.fillAll();

  // Azimuth reference PNG
  auto azRingBounds = azimuthControl.getBounds().toFloat();
  auto azRefBounds =
      azRingBounds.withSizeKeepingCentre(100, 100).translated(0.0f, -15.0f);
  g.drawImage(azRefImg, azRefBounds, juce::RectanglePlacement::centred);

  // Elevation reference PNG
  auto elSlideBounds = elevationControl.getBounds().toFloat();
  juce::Rectangle<float> elRefBounds(elSlideBounds.getX() - 70.0f,
                                     elSlideBounds.getCentreY() - 20.0f, 100.0f,
                                     100.0f);
  g.drawImage(elRefImg, elRefBounds, juce::RectanglePlacement::centred);
}

void SoundStageAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..

  elevationControl.setBounds(3 * getWidth() / 4, getHeight() / 8, 100,
                             3 * getHeight() / 4);
  azimuthControl.setBounds(0, 65, 200, 200);
}

void SoundStageAudioProcessorEditor::sliderValueChanged(juce::Slider *slider) {
  if (slider == &elevationControl) {
    audioProcessor.elevation->store(elevationControl.getValue());
  }

  if (slider == &azimuthControl) {
    audioProcessor.azimuth->store(azimuthControl.getValue());
  }
}
