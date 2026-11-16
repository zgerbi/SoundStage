/*
  ==============================================================================

    Convoluter.h
    Created: 22 Apr 2021 11:17:04am
    Author:  Eric

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>

class Convoluter {
public:
  Convoluter();
  ~Convoluter();
  void convolute(juce::AudioBuffer<float> &buffer);

  float elevation;
  float azimuth;

private:
  float hrir_l[25][50][200];
  float hrir_r[25][50][200];

  float ITD[25][50];

  float historyBuff[200] = {0.0f};
  int   write_index = 0;

  // Previous states for crossfading
  // Initialize to -1 for instant cold startup
  int   prevAzIndex = -1;
  int   prevElIndex = -1;
  float prevHrir_L[200] = {0.0f}, prevHrir_R[200] = {0.0f};
  float currHrir_L[200] = {0.0f}, currHrir_R[200] = {0.0f};
  float alpha = 1.0;

  // Manual ITD handling
  // The maximum ITD is <30 samples so 64 sample circular buffer is acceptable
  static const int itdBuffSize = 64;
  float itdBuff_L[itdBuffSize] = {0.0f}, itdBuff_R[itdBuffSize] = {0.0f};
  int   itd_write_index = 0;
  float currItd_L = 0.0f, currItd_R = 0.0f;
  //float itdSmoothingCoeff = 0.005f;

  float elevation_values[50] = {
      -45., -39.375, -33.75, -28.125, -22.5, -16.875, -11.25, -5.625,
      0.,   5.625,   11.25,  16.875,  22.5,  28.125,  33.75,  39.375,
      45.,  50.625,  56.25,  61.875,  67.5,  73.125,  78.75,  84.375,
      90.,  95.625,  101.25, 106.875, 112.5, 118.125, 123.75, 129.375,
      135., 140.625, 146.25, 151.875, 157.5, 163.125, 168.75, 174.375,
      180., 185.625, 191.25, 196.875, 202.5, 208.125, 213.75, 219.375,
      225., 230.625};
  float azimuth_values[25] = {
      -80., -65., -55., -45., -40., -35., -30., -25., -20., -15., -10., -5., 0.,
      5.,   10.,  15.,  20.,  25.,  30.,  35.,  40.,  45.,  55.,  65.,  80};

  int load_hrir_l();
  int load_hrir_r();
  int load_itd();
  float *get_hrir_l(int az, int elevation);
  float *get_hrir_r(int az, int elevation);
  float get_itd(int az, int elevation);
  float getDelay(float *itdBuff, int writeIndex, float delaySample);
  juce::AudioBuffer<float> get_hrir(int az, int elevation);
  int closest_elevation_index(float elevation);
  int closest_azimuth_index(float azimuth);
  float correctAzimuth(float azimuth);
  float correctElevation(float elevation, float azimuth);
};
