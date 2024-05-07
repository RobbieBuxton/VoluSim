import ctypes
import os
import json
from pymongo import MongoClient

# Define the Mode enumeration in Python
class Mode(ctypes.c_int):
    TRACKER = 0
    TRACKEROFFSET = 1
    STATIC = 2

# Set the path to the library
dir_path = os.path.dirname(os.path.realpath(__file__))
parent_dir = os.path.dirname(dir_path)
lib_path = os.path.join(parent_dir, "result/bin/libvolsim.so")
handle = ctypes.CDLL(lib_path)

# Specify the types of the input parameters and the return type for runSimulation
handle.runSimulation.argtypes = [Mode, ctypes.c_int]
handle.runSimulation.restype = ctypes.c_char_p

# Example usage of the runSimulation function
trackerMode = Mode.TRACKER  # Change this as needed for different modes
challengeNum = 4  # Example challenge number

# Call the function
result = handle.runSimulation(trackerMode, challengeNum)
output = result.decode('utf-8')  # Decode the result from bytes to string

# Convert the JSON string to a Python dictionary
data = json.loads(output)

# Connect to MongoDB (ensure MongoDB is running and accessible)
client = MongoClient('mongodb://localhost:27017/')
db = client['simulation_results']
collection = db['results']

# Insert data into MongoDB
insertion_result = collection.insert_one(data)

# Print the MongoDB insertion result
print('Data inserted with ID:', insertion_result.inserted_id)