import ctypes
import os
import json
import click
from pymongo import MongoClient

# Define the Mode enumeration in Python using a dictionary for simplicity
mode_map = {"t": "TRACKER", "to": "TRACKEROFFSET", "s": "STATIC"}

# Map from shorthand mode to an integer for ctypes
mode_ctypes_map = {"TRACKER": 0, "TRACKEROFFSET": 1, "STATIC": 2}


@click.group()
def cli():
    pass

@click.command()
@click.option(
    "--mode",
    type=click.Choice(["t", "to", "s"]),
    help="Mode, t: Tracker, to: TrackerOffset, s: Static",
)
@click.option("--num", type=int, help="Challenge number")
def run(mode, num):
    
    print("Starting")
    
    mycollection = connect_to_mongo()
    
    if mycollection is None:
        return

    mode_full = mode_map[mode]  # Convert input mode shorthand to full mode
    mode_ctypes = mode_ctypes_map[mode_full]  # Convert full mode to ctypes

    # Set the path to the library
    dir_path = os.path.dirname(os.path.realpath(__file__))
    parent_dir = os.path.dirname(dir_path)
    lib_path = os.path.join(parent_dir, "result/bin/libvolsim.so")
    handle = ctypes.CDLL(lib_path)

    # Specify the types of the input parameters and the return type for runSimulation
    handle.runSimulation.argtypes = [ctypes.c_int, ctypes.c_int]
    handle.runSimulation.restype = ctypes.c_char_p

    # Call the function
    result = handle.runSimulation(mode_ctypes, num)
    output = result.decode("utf-8")  # Decode the result from bytes to string

    # Convert the JSON string to a Python dictionary
    data = json.loads(output)

    result = mycollection.insert_one(data)
    print("Inserted record id:", result.inserted_id)


def connect_to_mongo():
    # Creation of MongoClient
    client = MongoClient("mongodb://localhost:27017/", serverSelectionTimeoutMS=1)
    try:
        client.server_info()
    except Exception:
        print("Unable to connect to the server.")
        return None
    # Access database
    mydatabase = client["user_study_db"]

    # Access collection of the database
    mycollection = mydatabase["myTable"]
    return mycollection


cli.add_command(run)

if __name__ == "__main__":
    cli()
