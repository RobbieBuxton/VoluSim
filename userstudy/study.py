import ctypes
import os
import sys

# Camera Offser
camera_x, camera_y, camera_z, camera_rot = -0.25, 65.0, 30.0, 117.5

# Define the Mode enumeration in Python using a dictionary for simplicity
mode_map = {"t": "TRACKER", "s": "STATIC", "to": "TRACKER_OFFSET", "so": "STATIC_OFFSET"}
mode_map_inverse = {v: k for k, v in mode_map.items()}

# Map from shorthand mode to an integer for ctypes
mode_ctypes_map = {"TRACKER": 0,  "TRACKER_OFFSET": 1, "STATIC": 2, "STATIC_OFFSET": 3}

def run_simulation(mode, challenge_num, debug=False):
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
    handle.runSimulation.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_bool]
    handle.runSimulation.restype = ctypes.c_char_p

    # Save the current stdout and stderr file descriptors
    original_stdout_fd = sys.stdout.fileno()
    original_stderr_fd = sys.stderr.fileno()

    # Create a null device file
    null_file = open(os.devnull, 'w')
    null_fd = null_file.fileno()

    try:
        # Redirect stdout and stderr to the null device
        os.dup2(null_fd, original_stdout_fd)
        os.dup2(null_fd, original_stderr_fd)

        # Call the function
        result = handle.runSimulation(mode_ctypes, challenge_num, camera_x, camera_y, camera_z, camera_rot, debug)
    finally:
        # Restore the original stdout and stderr file descriptors
        os.dup2(original_stdout_fd, original_stdout_fd)
        os.dup2(original_stderr_fd, original_stderr_fd)
        null_file.close()

    return result.decode("utf-8")  # Decode the result from bytes to string