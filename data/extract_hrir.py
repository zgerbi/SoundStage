import scipy
import numpy as np

# float64[25][50][200]
mat = scipy.io.loadmat("hrir_final.mat")

# Trimming excess precision from float64->float32 for performance.
# Should have no impact on sound. 32-bit float is more than sufficient
# and is likely higher precision than the equipment used to capture the data
hrir_l_data = np.ascontiguousarray(mat["hrir_l"], dtype=np.float32)
hrir_r_data = np.ascontiguousarray(mat["hrir_r"], dtype=np.float32)

hrir_l_data.tofile("hrir_l.bin")
hrir_r_data.tofile("hrir_r.bin")