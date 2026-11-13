/*
  ==============================================================================

        This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "Convoluter.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_data_structures/juce_data_structures.h"
#include <JuceHeader.h>
#include <cmath>
#include <math.h>
#include <string>
#include <typeinfo>
#include <vector>

//==============================================================================
/**
 */
class SoundStageAudioProcessor : public juce::AudioProcessor {
public:
  //==============================================================================
  SoundStageAudioProcessor();
  ~SoundStageAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //===================================st===========================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  void process(juce::dsp::ProcessContextReplacing<float> context);
  void updateParameters();

  // For mapping MIDI and saving parameters
  juce::AudioProcessorValueTreeState apvts;
  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

  // real params
  std::atomic<float> *elevation;
  std::atomic<float> *azimuth;
  Convoluter *convoluter;

  // end read params

private:
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundStageAudioProcessor)

  void applyHRTF(float *channelData, float *hrtf, int numSamples);
  float correctAzimuth(float azimuth);
  float correctElevation(float elevation, float azimuth);
};
