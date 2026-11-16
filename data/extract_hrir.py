import math
import scipy
import scipy.signal
import numpy as np

#TODO: File overview

# Walk backwards from the recorded onset to find the preceding zero-crossing
# of the main impulse lobe, returning the worst-case delay in samples
def get_max_zero_crossing_delay(hrir:np.ndarray, on:np.ndarray) -> float:
    max_delay_8x = 0
    for az in range(25):
        for el in range(50):
            on_8x = int(round(on[az, el] * 8))
            hrir_8x = scipy.signal.resample_poly(hrir[az, el], up=8, down=1)
            lobe_sign = 1 if hrir_8x[on_8x] >= 0 else -1
            idx_8x = on_8x
            while idx_8x > 0 and (hrir_8x[idx_8x] * lobe_sign > 0):
                idx_8x -= 1
            max_delay_8x = max(max_delay_8x, on_8x - idx_8x)
    return max_delay_8x / 8.0

# Time offsets were recorded to 1/8th of a sample
# So 8x upsample for accurate time shifting
# Then 8x downsample to get back to original format
def shift_sample(hrir:np.ndarray, on:np.ndarray, target_onset:int) -> np.ndarray:
    hrir_data = np.zeros_like(hrir)
    for az in range(25):
        for el in range(50):
            on_8x = int(round((on[az, el] - target_onset) * 8))
            hrir_8x = scipy.signal.resample_poly(hrir[az, el], up=8, down=1)
            hrir_shifted = np.pad(hrir_8x[on_8x:], (0, on_8x), mode='constant')
            hrir_data[az, el] = hrir_shifted[::8]
    return hrir_data

# float64[25][50][200]
mat = scipy.io.loadmat("hrir_final.mat")

# Trimming excess precision from float64->float32 for performance.
# Will have no impact on sound. Original samples were only recorded 
# at 16-bit depth, so we are still retaining plenty of dynamic range
hrir_l_raw = np.ascontiguousarray(mat["hrir_l"], dtype=np.float32)
hrir_r_raw = np.ascontiguousarray(mat["hrir_r"], dtype=np.float32)

# Values measured in samples, down to 1/8th of a sample.
# Stored as float64 in hrir_final.mat, but float32 retains more than
# enough precision within the data's range of 0 < ITD < 30
ITD_data = np.ascontiguousarray(mat["ITD"], dtype=np.float32)

# Values measured in samples, down to 1/8th of a sample.
OnL_data = np.ascontiguousarray(mat["OnL"], dtype=np.float32)
OnR_data = np.ascontiguousarray(mat["OnR"], dtype=np.float32)

# Keep enough leading samples before the onset to preserve the main lobe
# zero-crossing across all impulse responses without clipping
max_delay_l = get_max_zero_crossing_delay(hrir_l_raw, OnL_data)
max_delay_r = get_max_zero_crossing_delay(hrir_r_raw, OnR_data)
target_onset = math.ceil(max(max_delay_l, max_delay_r))

hrir_l_data = shift_sample(hrir_l_raw, OnL_data, target_onset)
hrir_r_data = shift_sample(hrir_r_raw, OnR_data, target_onset)

hrir_l_data.tofile("hrir_l.bin")
hrir_r_data.tofile("hrir_r.bin")
ITD_data.tofile("ITD.bin")