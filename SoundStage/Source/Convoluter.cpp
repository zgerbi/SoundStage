/*
  ==============================================================================

    Convoluter.cpp
    Created: 22 Apr 2021 11:17:04am
    Author:  Eric

  ==============================================================================
*/

#include "Convoluter.h"
#include <BinaryData.h>

Convoluter::Convoluter() {
  currSamplesPerBlock = -1;
  bufferSize = -1;
  // inputBuffer = juce::AudioBuffer<float>();
  // outputBuffer = juce::AudioBuffer<float>();
  convolutedSignal = juce::AudioBuffer<float>();
  overflowStorage = juce::AudioBuffer<float>(2, 200);
  overflowStorage.clear();

  // inputPos = 0;
  // outputPos = 0;
  elevation = 0;
  azimuth = 0;

  load_hrir_l();
  load_hrir_r();
}

Convoluter::~Convoluter() {}

void Convoluter::setSamplesPerBlock(int samplesPerBlock) {

  if (samplesPerBlock != currSamplesPerBlock) {
    bufferSize = samplesPerBlock;

    convolutedSignal = juce::AudioBuffer<float>(2, bufferSize + overflowSize);
    convolutedSignal.clear();

    overflowStorage = juce::AudioBuffer<float>(2, overflowSize);
    overflowStorage.clear();

    currSamplesPerBlock = samplesPerBlock;
  }
}

void Convoluter::convolute(juce::AudioBuffer<float> &buffer) {
  const int numSamples = buffer.getNumSamples();
  const int num_conv = numSamples + overflowSize;
  int i, j, k;
  float temp;

  if (numSamples > bufferSize) {
    setSamplesPerBlock(numSamples);
  }

  // Channel Loop 1
  // For each channel, convolute the signal based on the current azimuth and
  // elevation, then add the samples from the overflow storage to the begining
  // of the covoluted signal
  for (int channel = 0; channel < convolutedSignal.getNumChannels();
       channel++) {
    const int azimuth_index = closest_azimuth_index((correctAzimuth(azimuth)));
    const int elev_index =
        closest_elevation_index((correctElevation(elevation, azimuth)));

    auto *readPointer = buffer.getReadPointer(channel);
    auto *writePointer = convolutedSignal.getWritePointer(channel);
    float *hrtf;

    if (channel == 0) {
      hrtf = get_hrir_l(azimuth_index, elev_index);
    } else if (channel == 1) {
      hrtf = get_hrir_r(azimuth_index, elev_index);
    }

    for (i = 0; i < num_conv; i++) {
      k = i;
      temp = 0.0f;

      // for each sample in filter
      for (j = 0; j < overflowSize; j++) {
        if (k >= 0 && k < numSamples) {
          temp += readPointer[k] * hrtf[j];
        }
        k--;
      }
      writePointer[i] = temp;
    }

    // Add overflow storage to the beginning of the convoluted signal.
    auto *overflowPointer = overflowStorage.getReadPointer(channel);
    for (int z = 0; z < overflowSize; z++) {
      writePointer[z] += overflowPointer[z];
    }
  }

  overflowStorage.clear();

  // Channel Loop 2
  // Push convoluted signal to the output buffer and generate the overflow
  // storage from the convoluted signal
  for (int channel = 0; channel < convolutedSignal.getNumChannels();
       channel++) {
    auto *readPointer = convolutedSignal.getReadPointer(channel);
    buffer.copyFrom(channel, 0, readPointer, numSamples);

    auto *readPointerOverflow =
        convolutedSignal.getReadPointer(channel, numSamples);
    overflowStorage.copyFrom(channel, 0, readPointerOverflow, overflowSize);
  }

  convolutedSignal.clear();
}

float Convoluter::correctAzimuth(float azimuth) {
  // this function takes in an angle from 0-360 and spits out the azimuth
  // translated to what our HRTF can use
  /*Structure:*/
  if (azimuth >= 270.0 && azimuth <= 360.0) {
    return -1.0 * (360.0 - azimuth);
  } else if (azimuth >= 0.0 && azimuth <= 90.0) {
    return azimuth;
  } else if (azimuth > 90.0 && azimuth <= 180.0) {
    return (180 - azimuth);
  } else if (azimuth > 180.0 && azimuth < 270.0) {
    return -1.0 * (azimuth - 180.0);
  } else
    return 0.0;
}

float Convoluter::correctElevation(float elevation, float azimuth) {
  // this function takes in an angle from -45 to 90 and spits out the correct
  // elevation based on the azimuth
  float elevbuff = 0.0f;
  if (azimuth >= 270.0 && azimuth <= 360.0) {
    return elevation;
  } else if (azimuth >= 0.0 && azimuth <= 90.0) {
    return elevation;
  } else if (azimuth > 90.0 && azimuth < 270.0) {
    elevbuff = elevation + (180 - 2 * elevation);
    if (elevbuff > 230.625)
      elevbuff = 230.625;
    return elevbuff;
  } else
    return 0.0;
}

int Convoluter::closest_elevation_index(float elevation) {
  float min_diff = std::numeric_limits<float>::max();
  int index_found = -1;

  for (int i = 0; i < 50; i++) {
    float temp = abs(elevation - elevation_values[i]);

    if (temp < min_diff) {
      min_diff = temp;
      index_found = i;
    }
  }

  return index_found;
}

int Convoluter::closest_azimuth_index(float azimuth) {
  float min_diff = std::numeric_limits<float>::max();
  int index_found = -1;

  for (int i = 0; i < 25; i++) {
    float temp = abs(azimuth - azimuth_values[i]);

    if (temp < min_diff) {
      min_diff = temp;
      index_found = i;
    }
  }

  return index_found;
}

int Convoluter::load_hrir_l() {

  //	juce::File file = DATA_DIR.getChildFile("SoundStage/hrir_l.txt");

  //	juce::String s = file.loadFileAsString();
  //	if (!file.exists()) {
  //		return -1;
  //	}

  //	int pos = 0;
  juce::String hrir_l_data = juce::String::fromUTF8(BinaryData::hrir_l_txt,
                                                    BinaryData::hrir_l_txtSize);
  juce::StringArray stringArray;
  stringArray.addLines(hrir_l_data);

  //	file.readLines(stringArray);
  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 50; j++) {
      for (int k = 0; k < 200; k++) {
        hrir_l[i][j][k] =
            stringArray[(i * 50 * 200) + (j * 200) + k].getFloatValue();
      }
    }
  }

  return 0;
}

int Convoluter::load_hrir_r() {
  //	juce::File file = DATA_DIR.getChildFile("SoundStage/hrir_r.txt");

  //	juce::String s = file.loadFileAsString();
  //	if (!file.exists()) {
  //		juce::Logger::outputDebugString("failed to load right");
  //		juce::Logger::outputDebugString(juce::String(file.getFullPathName()));
  //		return -1;
  //	}

  //	int pos = 0;
  juce::String hrir_r_data = juce::String::fromUTF8(BinaryData::hrir_r_txt,
                                                    BinaryData::hrir_r_txtSize);
  juce::StringArray stringArray;
  stringArray.addLines(hrir_r_data);

  //	file.readLines(stringArray);
  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 50; j++) {
      for (int k = 0; k < 200; k++) {
        hrir_r[i][j][k] =
            stringArray[(i * 50 * 200) + (j * 200) + k].getFloatValue();
      }
    }
  }

  return 0;
}

float *Convoluter::get_hrir_l(int az, int elevation) {
  float *value = hrir_l[az][elevation];

  return value;
}

float *Convoluter::get_hrir_r(int az, int elevation) {
  float *value = hrir_r[az][elevation];

  return value;
}
