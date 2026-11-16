/*
  ==============================================================================

    Convoluter.cpp
    Created: 22 Apr 2021 11:17:04am
    Author:  Eric

  ==============================================================================
*/

#include "Convoluter.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <BinaryData.h>
#include <arm_neon.h>
#include <cstring>

Convoluter::Convoluter() {
  elevation = 0;
  azimuth = 0;

  load_hrir_l();
  load_hrir_r();

  load_itd();
}

Convoluter::~Convoluter() {}

void Convoluter::convolute(juce::AudioBuffer<float> &buffer) {
  const int numSamples = buffer.getNumSamples();

  const int azimuth_index = closest_azimuth_index(correctAzimuth(azimuth));
  const int elev_index =
      closest_elevation_index(correctElevation(elevation, azimuth));

  if (azimuth_index != prevAzIndex || elev_index != prevElIndex) {
    float *newHrir_L = get_hrir_l(azimuth_index, elev_index);
    float *newHrir_R = get_hrir_r(azimuth_index, elev_index);

    // Initial setting
    if (prevAzIndex == -1 || prevElIndex == -1) {
      memcpy(prevHrir_L, newHrir_L, sizeof(prevHrir_L));
      memcpy(prevHrir_R, newHrir_R, sizeof(prevHrir_R));
      memcpy(currHrir_L, newHrir_L, sizeof(currHrir_L));
      memcpy(currHrir_R, newHrir_R, sizeof(currHrir_R));
      alpha = 1.0f;
    } else {
      // Position has changed
      // Load new filter into current
      // Blend filter into prev filter in case filter changes again before fully
      // blended
      for (int i = 0; i < 200; i++) {
        prevHrir_L[i] = (1.0f - alpha) * prevHrir_L[i] + (alpha)*currHrir_L[i];
        prevHrir_R[i] = (1.0f - alpha) * prevHrir_R[i] + (alpha)*currHrir_R[i];
        currHrir_L[i] = newHrir_L[i];
        currHrir_R[i] = newHrir_R[i];
      }
      alpha = 0.0f;
    }

    prevAzIndex = azimuth_index;
    prevElIndex = elev_index;
  }

  const float newItd = get_itd(azimuth_index, elev_index);
  // ITD values are stored regardless of direction, so we will
  // only apply the delay if they come from the opposite side
  // Azimuth index 12 is equivalent to an angle of 0, or dead center ahead
  float newItd_L = (azimuth_index <= 12) ? 0.0f : newItd;
  float newItd_R = (azimuth_index >= 12) ? 0.0f : newItd;

  auto *input = buffer.getReadPointer(0);

  auto *out_l = buffer.getWritePointer(0);
  auto *out_r = buffer.getWritePointer(1);

  // Process loop
  for (int i = 0; i < numSamples; i++) {
    historyBuff[write_index] = input[i];

    // Crossfade over 
    if (alpha < 1.0f) {
      alpha = std::min(1.0f, alpha + 0.0005f);
    }

    float conv_l = 0.0f;
    float conv_r = 0.0f;

    // Moving backward from current sample in circular buffer
    for (int j = 0; j < 200; j++) {
      int pastIndex = write_index - j;
      if (pastIndex < 0) {
        pastIndex += 200;
      }

      float pastSample = historyBuff[pastIndex];

      float hrtf_l = (1.0f - alpha) * prevHrir_L[j] + (alpha)*currHrir_L[j];
      float hrtf_r = (1.0f - alpha) * prevHrir_R[j] + (alpha)*currHrir_R[j];

      conv_l += hrtf_l * pastSample;
      conv_r += hrtf_r * pastSample;
    }

    write_index = (write_index + 1) % 200;

    itdBuff_L[itd_write_index] = conv_l;
    itdBuff_R[itd_write_index] = conv_r;

    // Ramping delay to avoid clicking
    currItd_L += 0.001f * (newItd_L - currItd_L);
    currItd_R += 0.001f * (newItd_R - currItd_R);

    out_l[i] = getDelay(itdBuff_L, itd_write_index, currItd_L);
    out_r[i] = getDelay(itdBuff_R, itd_write_index, currItd_R);

    itd_write_index = (itd_write_index + 1) % 64;
  }
}

float Convoluter::correctAzimuth(float azimuth) {
  // this function takes in an angle from 0-360 and spits out the azimuth
  // translated to what our HRTF can use.

  // The HRIR data only covers -80 to 80 for left and right ears, respectively.
  // It makes a sphere using the elevation, where -45* to 90 are in front/below
  // to above, and 90 to ~230 are above to behind/below Therefore, we correct
  // the azimuth to lay only "in front", while adjusting the elevation in
  // Convoluter::correctElevation to account for behind

  if (azimuth > 90.0) {
    return 180.0 - azimuth;
  } else if (azimuth < -90.0) {
    return -180.0 - azimuth;
  } else {
    return azimuth;
  }
}

float Convoluter::correctElevation(float elevation, float azimuth) {
  // this function takes in an elevation from -45 to 90 and an azimuth and spits
  // out the corrected elevation for the HRIR coordinate system

  if (std::abs(azimuth) > 90.0) {
    return 180.0 - elevation;
  }

  return elevation;
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
  jassert(BinaryData::hrir_l_binSize == sizeof(hrir_l));
  std::memcpy(hrir_l, BinaryData::hrir_l_bin, BinaryData::hrir_l_binSize);
  return 0;
}

int Convoluter::load_hrir_r() {
  jassert(BinaryData::hrir_r_binSize == sizeof(hrir_r));
  std::memcpy(hrir_r, BinaryData::hrir_r_bin, BinaryData::hrir_r_binSize);
  return 0;
}

int Convoluter::load_itd() {
  jassert(BinaryData::ITD_binSize == sizeof(ITD));
  std::memcpy(ITD, BinaryData::ITD_bin, BinaryData::ITD_binSize);
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

float Convoluter::get_itd(int az, int elevation) {
  float value = ITD[az][elevation];
  return value;
}

// Delays samples by some fractional amount delaySample
float Convoluter::getDelay(float *itdBuff, int writeIndex, float delaySample) {
  float readSample = (float)writeIndex - delaySample;
  if (readSample < 0.0f) {
    readSample += 64.0f;
  }

  int lhs = (int)readSample;
  float dec = readSample - (float)lhs;
  // readSample can sometimes end up barely negative, and
  // could end up getting rounded to 64.0 when 64 is added.
  // Mod lhs with 64 to ensure that we're not reading out of bounds.
  lhs = lhs % 64;
  int rhs = (lhs + 1) % 64;

  return (1.0f - dec) * itdBuff[lhs] + dec * itdBuff[rhs];
}
