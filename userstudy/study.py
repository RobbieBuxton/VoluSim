import ctypes
import os
import sys

import utility

# Camera Offser
camera_x, camera_y, camera_z, camera_rot = -5.0, 68.0, 31.0, 114

mainMonitor = 2
offsetMonitor = 3

# Define the Mode enumeration in Python using a dictionary for simplicity
mode_map = {"t": "TRACKER", "s": "STATIC", "to": "TRACKER_OFFSET", "so": "STATIC_OFFSET"}
mode_map_inverse = {v: k for k, v in mode_map.items()}

# Map from shorthand mode to an integer for ctypes
mode_ctypes_map = {"TRACKER": 0,  "TRACKER_OFFSET": 1, "STATIC": 2, "STATIC_OFFSET": 3}

def run_simulation(mode, challenge_num, debug=False, timeout=60, beep=True):
    if beep:
        utility.play_beep()
    # Convert input mode shorthand to full mode
    mode_full = mode_map[mode]
    # Convert full mode to ctypes
    mode_ctypes = mode_ctypes_map[mode_full]

    # Set the path to the library
    dir_path = os.path.dirname(os.path.realpath(__file__))
    parent_dir = os.path.dirname(dir_path)
    lib_path = os.path.join(parent_dir, "result/bin/libvolsim.so")
    handle = ctypes.CDLL(lib_path)

    # Specify the types of the input parameters and the return type for runSimulation
    handle.runSimulation.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_bool]
    handle.runSimulation.restype = ctypes.c_char_p

    result = handle.runSimulation(mode_ctypes, challenge_num, camera_x, camera_y, camera_z, camera_rot, mainMonitor, offsetMonitor, timeout, debug)

    return result.decode("utf-8")  # Decode the result from bytes to string