/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "PluginProcessor.h"
#include "juce_graphics/juce_graphics.h"
#include <JuceHeader.h>
#include <memory>

//==============================================================================
/**
 */
class SoundStageAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Slider::Listener {
public:
  SoundStageAudioProcessorEditor(SoundStageAudioProcessor &,
                                 juce::AudioProcessorValueTreeState &);
  ~SoundStageAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

  void sliderValueChanged(juce::Slider *slider) override;

private:
  // Controls

  juce::Slider azimuthControl;
  juce::Slider elevationControl;

  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      azimuthControlAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      elevationControlAttachment;

  juce::Label azLabel;
  juce::Label elLabel;

  SoundStageAudioProcessor &audioProcessor;
  juce::AudioProcessorValueTreeState &audioProcessorValueTreeState;

  // UI design
  juce::Image azRefImg;
  juce::Image elRefImg;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundStageAudioProcessorEditor)
};
