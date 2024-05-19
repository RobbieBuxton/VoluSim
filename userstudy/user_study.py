import ctypes
import os
import json
import click
import tabulate
import pyvista as pv
import numpy as np

from pymongo import MongoClient
from bson import ObjectId

# Camera Offser
camera_x, camera_y, camera_z, camera_rot = 1, 70, 30, 115.0

# Define the Mode enumeration in Python using a dictionary for simplicity
mode_map = {"t": "TRACKER", "o": "OFFSET", "s": "STATIC"}

# Map from shorthand mode to an integer for ctypes
mode_ctypes_map = {"TRACKER": 0,  "OFFSET": 1, "STATIC": 2}

@click.group()
def cli():
    """ Command line interface for managing operations. """
    pass

@cli.command()
def debug():
      # Set the path to the library
    dir_path = os.path.dirname(os.path.realpath(__file__))
    parent_dir = os.path.dirname(dir_path)
    lib_path = os.path.join(parent_dir, "result/bin/libvolsim.so")
    handle = ctypes.CDLL(lib_path)

    # Specify the types of the input parameters and the return type for runSimulation
    handle.runSimulation.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
    handle.runSimulation.restype = ctypes.c_char_p

    # Call the function
    result = handle.runSimulation(0, 1, camera_x, camera_y, camera_z, camera_rot)
    _ = result.decode("utf-8")  # Decode the result from bytes to string
    visualize_point_cloud_pyvista("misc/pointCloud.csv")   
    
@cli.command()
@click.option(
    "-m",
    type=click.Choice(["t", "o", "s"], case_sensitive=False),
    required=True,
    help="Mode, t: Tracker, to: TrackerOffset, s: Static"
)
@click.option(
    "-n",
    type=int,
    required=True,
    help="Challenge number"
)
@click.option(
    "-u",
    type=str,
    required=True,
    help="User ID"
)
@click.option(
    "--test", 
    is_flag=True, 
    default=False, 
    help="Enable testing mode"
)
def run(m, n, u, test):
    mode = m
    num = n
    user = u
    """ Run the simulation with specific mode, challenge number, and user ID. """
    print("Starting")
    
    if not test:
        db = connect_to_mongo()
        users_collection = db["users"]
        results_collection = db["results"]
        
        # Validate user ID exists
        if users_collection.count_documents({"_id": user},limit = 1) == 0:
            print(f"User ID {user} does not exist in the database.")
            return

        # Check if the result for this user and mode already exists
        if results_collection.count_documents({"user_id": user, "mode": mode_map[mode], "challenge_num": num}) > 0:
            print(f"Result for User ID {user} with Mode {mode_map[mode]} and Challenge Number {num} already exists.")
            return

    mode_full = mode_map[mode]  # Convert input mode shorthand to full mode
    mode_ctypes = mode_ctypes_map[mode_full]  # Convert full mode to ctypes

    # Set the path to the library
    dir_path = os.path.dirname(os.path.realpath(__file__))
    parent_dir = os.path.dirname(dir_path)
    lib_path = os.path.join(parent_dir, "result/bin/libvolsim.so")
    handle = ctypes.CDLL(lib_path)

    # Specify the types of the input parameters and the return type for runSimulation
    handle.runSimulation.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
    handle.runSimulation.restype = ctypes.c_char_p

    # Call the function
    result = handle.runSimulation(mode_ctypes, num, camera_x, camera_y, camera_z, camera_rot)
    output = result.decode("utf-8")  # Decode the result from bytes to string
    
    if not test:
        # Convert the JSON string to a Python dictionary
        data = json.loads(output)
        print(data["finished"])
        if (not data["finished"]):
            print("Simulation did not finish successfully: exiting")
            return
        
        data['user_id'] = user  # Add the user ID to the data dictionary
        data['mode'] = mode_map[mode]  # Store the mode as well
        data["challenge_num"] = num  # Store the challenge number as well

        result = results_collection.insert_one(data)
        print("Inserted record id:", result.inserted_id)
    
@cli.command()
@click.argument("first_name")
@click.argument("last_name")
def newuser(first_name, last_name):
    """ Adds a user with first and last name to the database. """
    mydatabase = connect_to_mongo()
    user_collection = mydatabase["users"]

    user_data = {
        "_id": f"{first_name[:3].lower()}{last_name[:3].lower()}", 
        "first_name": first_name,
        "last_name": last_name
    }
    
    result = user_collection.insert_one(user_data)
    print(f"Added user {first_name} {last_name} with ID: {result.inserted_id}")

# Helper function to handle MongoDB ObjectId when serializing to JSON
def json_encoder(o):
    if isinstance(o, ObjectId):
        return str(o)
    return o

@cli.command()
def listusers():
    """ Lists all users in the database. """
    mydatabase = connect_to_mongo()
    user_collection = mydatabase["users"]

    for user in user_collection.find():
        print(json.dumps(user, indent=4, default=json_encoder))

@cli.command()
def listresults():
    """ Displays a table with the completion status of each task for every user. """
    db = connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    users_collection = db["users"]
    results_collection = db["results"]
    
    # Retrieve all users
    users = users_collection.find()
    
    # Define the table headers
    headers = ["UserID", "First Name", "Last Name"]
    modes = ["TRACKER", "OFFSET", "STATIC"]
    challenges = range(1, 6)
    
    # Append mode and challenge number combinations to headers
    for mode in modes:
        for challenge in challenges:
            headers.append(f"{mode[:1]}-{challenge}")

    # Initialize the table rows
    table = []
    
    # Iterate over each user
    for user in users:
        row = [user["_id"], user.get("first_name", ""), user.get("last_name", "")]
        
        # Check each mode and challenge combination
        for mode in modes:
            for challenge in challenges:
                result_exists = results_collection.count_documents({"user_id": user["_id"], "mode": mode, "challenge_num": challenge})
                # Append a tick or cross based on result existence
                row.append("✓" if result_exists else " ")
        
        # Append the row to the table
        table.append(row)
    
    # Print the table using tabulate
    print(tabulate.tabulate(table, headers=headers, tablefmt="grid"))


def connect_to_mongo():
    """ Connects to MongoDB and returns the database object. """
    # Creation of MongoClient
    client = MongoClient("mongodb://localhost:27017/", serverSelectionTimeoutMS=5000)
    try:
        client.server_info()  # Force connection on a request as the driver doesn't connect until you attempt to send some data
    except Exception as e:
        print("Unable to connect to the server:", e)
        return None
    # Access database
    mydatabase = client["user_study_db"]

    return mydatabase

def visualize_point_cloud_pyvista(file_path, z_min=0, z_max=500, distance_threshold=1000):
    data = np.loadtxt(file_path, delimiter=',', skiprows=1)

    # Create a mask that includes points close to the eyes or hands
    filtered_data = data
    
    # Normalize z-values for color mapping
    z_normalized = (filtered_data[:, 2] - z_min) / (z_max - z_min)
    z_normalized = np.clip(z_normalized, 0, 1)  # Ensure values are between 0 and 1
    
    point_cloud = pv.PolyData(filtered_data[:, :3])
    plotter = pv.Plotter()
    
    # Use the 'coolwarm' colormap, with colors based on normalized z-values
    plotter.add_points(point_cloud, scalars=z_normalized, cmap="coolwarm", point_size=1.5)
    
    # Define the four points
    width = 34.0
    height = 52.0
    points = np.array([
        [-width/2, 0, 0],
        [width/2, 0, 0],
        [-width/2, height, 0],
        [width/2, height, 0]
    ])
    
    # Add the points to the plotter
    plotter.add_points(points, color='green', point_size=10)

    # Define the rectangle edges
    edges = np.array([
        [points[0], points[1]],
        [points[1], points[3]],
        [points[3], points[2]],
        [points[2], points[0]]
    ])
    
    # Add the rectangle edges to the plotter
    for edge in edges:
        plotter.add_lines(edge, color='yellow', width=5)
    
    plotter.view_xz()
    plotter.camera.position = (1, 1, 0)
    
    plotter.add_axes(interactive=False, line_width=5, labels_off=False)

    plotter.show()

if __name__ == "__main__":
    cli()