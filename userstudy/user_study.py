import ctypes
import os
import json

# Importing module
from pymongo import MongoClient


def run_simulation(mode, challenge_num):
	# Set the path to the library
	dir_path = os.path.dirname(os.path.realpath(__file__))
	parent_dir = os.path.dirname(dir_path)
	lib_path = os.path.join(parent_dir, "result/bin/libvolsim.so")
	handle = ctypes.CDLL(lib_path)

	# Specify the types of the input parameters and the return type for runSimulation
	handle.runSimulation.argtypes = [Mode, ctypes.c_int]
	handle.runSimulation.restype = ctypes.c_char_p

	# Call the function
	result = handle.runSimulation(mode, challenge_num)
	output = result.decode('utf-8')  # Decode the result from bytes to string

	# Convert the JSON string to a Python dictionary
	return json.loads(output)
 
def connect_to_mongo():
	# Creation of MongoClient
	client = MongoClient()

	# Connect with the portnumber and host
	client = MongoClient("mongodb://localhost:27017/")

	# Access database
	mydatabase = client['user_study_db']

	# Access collection of the database
	mycollection = mydatabase['myTable']
	return mycollection

# Define the Mode enumeration in Python
class Mode(ctypes.c_int):
    TRACKER = 0
    TRACKEROFFSET = 1
    STATIC = 2

#################### Main ####################


data = run_simulation(Mode.TRACKER, 1)

mycollection = connect_to_mongo()

# Inserting the data in the database
result = mycollection.insert_one(data)

# Print the ID of the new document
print("Inserted record id:", result.inserted_id)

# To find() all the entries inside collection name 'myTable' 
cursor = mycollection.find() 
for record in cursor: 
    print(record) 