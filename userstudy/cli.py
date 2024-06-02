
import json
import click
import tabulate
import random
from statistics import mean, stdev
import time
import datetime
import math

#User created
import utility
import study
import mongodb
import visualize
import graph

@click.group()
def cli():
    """ Command line interface for managing operations. """
    pass


############
### Run  ###
############
@cli.group
def run():
	"""run"""
	pass

@run.command()
def debug():
    study.run_simulation("t", 1, "True",1,False)
    # study.run_simulation("to", 1, "True",1,False)
    visualize.visualize_point_cloud_pyvista("misc/pointCloud.csv")   
    
@run.command()
@click.argument("user")
@click.argument("distance")
def eval(user, distance):    
    db = mongodb.connect_to_mongo()
    results_collection = db["evaluation"]
    
    output = study.run_simulation("t", 1)
    
    data = json.loads(output)
    print(data["finished"])
    
    data['date'] = datetime.datetime.now()
    data['user'] = user
    data['pydown'] = 1
    data['distance'] = int(distance)
    result = results_collection.insert_one(data)
    print("Inserted record id:", result.inserted_id)
    
@run.command()
def demo():
    for mode in ["t","s","to","so"]:
        for challenge_num in range(1, 3):
            study.run_simulation(mode, -challenge_num)
    utility.play_finished()
	
@run.command()
@click.argument("user_id")
@click.option(
    "-m",
    type=click.Choice(["t", "to", "s", "so"], case_sensitive=False),
    required=True,
    help="Mode, t: Tracker, to: TrackerOffset, s: Static, so: Static Offset"
)
@click.option(
    "-n",
    type=int,
    required=True,
    help="Challenge number"
)
@click.option(
    "--test", 
    is_flag=True, 
    default=False, 
    help="Enable testing mode"
)
def task(m, n, user_id, test):
    mode = m
    num = n
    user = user_id
    """ Run the simulation with specific mode, challenge number, and user ID. """
    print("Starting")
    
    if not test:
        db = mongodb.connect_to_mongo()
        users_collection = db["users"]
        results_collection = db["results_2"]
        
        # Validate user ID exists
        if users_collection.count_documents({"_id": user},limit = 1) == 0:
            print(f"User ID {user} does not exist in the database.")
            return

        # Check if the result for this user and mode already exists
        if results_collection.count_documents({"user_id": user, "mode": study.mode_map[mode], "challenge_num": num}) > 0:
            print(f"Result for User ID {user} with Mode {study.mode_map[mode]} and Challenge Number {num} already exists.")
            return

    output = study.run_simulation(mode,num)
    
    if not test:
        # Convert the JSON string to a Python dictionary
        data = json.loads(output)
        print(data["finished"])
        
        data['user_id'] = user  # Add the user ID to the data dictionary
        data['mode'] = study.mode_map[mode]  # Store the mode as well
        data["challenge_num"] = num  # Store the challenge number as well
        data['date'] = datetime.datetime.now()

        result = results_collection.insert_one(data)
        print("Inserted record id:", result.inserted_id)
    

@run.command()
@click.argument("user_id")
def next(user_id):
    # Connect to the MongoDB database
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    users_collection = db["users"]
    results_collection = db["results_2"]
    
    # Retrieve user document
    user = users_collection.find_one({"_id": user_id})
    if not user:
        print(f"User ID {user_id} does not exist in the database.")
        return
    
    # Get user's task order
    task_order = user.get("order", [])
    if not task_order:
        print(f"No task order found for User ID {user_id}.")
        return
    
    # Iterate over each task in the order and check if results exist
    all_tasks_completed = True
    for mode in task_order:
        print(mode)
        if not all_tasks_completed:
            # Break out of the outer loop once we complete the first incomplete mode
            break
        for challenge_num in range(1, 6):
            result_exists = results_collection.count_documents({
                "user_id": user_id,
                "mode": mode,
                "challenge_num": challenge_num
            }) > 0
            
            if not result_exists:
                all_tasks_completed = False
                print(f"Starting {mode}: {challenge_num}")
                output = study.run_simulation(study.mode_map_inverse[mode], challenge_num)
                
                # Convert the JSON string to a Python dictionary
                data = json.loads(output)            
                data['user_id'] = user_id
                data['mode'] = mode
                data['challenge_num'] = challenge_num
                data['date'] = datetime.datetime.now()
                
                results_collection.insert_one(data)
                break
        
    utility.play_finished()
    print(f"All tasks for User ID {user_id} have been completed.")

@cli.group()
def add():
    """add"""
    pass

@add.command()
@click.argument("first_name")
@click.argument("last_name")
def user(first_name, last_name):
    """ Adds a user with first and last name to the database. """
    mydatabase = mongodb.connect_to_mongo()
    user_collection = mydatabase["users"]
    
    random.seed()
    order = random.sample(["TRACKER", "TRACKER_OFFSET", "STATIC", "STATIC_OFFSET"], k=4)
    user_data = {
        "_id": f"{first_name[:3].lower()}{last_name[:3].lower()}", 
        "first_name": first_name,
        "last_name": last_name,
        "order": order
    }
    
    result = user_collection.insert_one(user_data)
    print(f"Added user {first_name} {last_name} with ID: {result.inserted_id}")

############
### List ###
############
@cli.group()
def list():
    """List various resources."""
    pass

@list.command()
def users():
    """ Lists all users in the database. """
    mydatabase = mongodb.connect_to_mongo()
    user_collection = mydatabase["users"]

    for user in user_collection.find():
        print(json.dumps(user, indent=4, default=utility.json_encoder))

@list.command()
def results():
    """ Displays a table with the completion status of each task for every user. """
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    users_collection = db["users"]
    results_collection = db["valid_results"]
    
    # Retrieve all users
    users = users_collection.find()
    
    # Define the table headers
    headers = ["UserID", "First Name", "Last Name"]
    modes = ["TRACKER", "TRACKER_OFFSET", "STATIC", "STATIC_OFFSET"]
    challenges = range(1, 6)
    
    # Append mode and challenge number combinations to headers
    for mode in modes:
        for challenge in challenges:
            headers.append(f"{study.mode_map_inverse[mode]}-{challenge}")

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

############
### Show ###
############
@cli.group()
def show():
    """List various resources."""
    pass

@show.command()
@click.argument("user_id")
def result(user_id):
    """Displays a table with the average time between completions and the standard deviation for the requested user."""
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    users_collection = db["users"]
    results_collection = db["valid_results"]

    # Retrieve the user
    user = users_collection.find_one({"_id": user_id})
    if not user:
        print(f"User ID {user_id} does not exist in the database.")
        return
    
    # Define the table headers
    headers = ["Metric"]
    modes = ["TRACKER", "TRACKER_OFFSET", "STATIC", "STATIC_OFFSET"]
    challenges = range(1, 6)
    
    # Append mode and challenge number combinations to headers
    for mode in modes:
        for challenge in challenges:
            headers.append(f"{mode[:1]}-{challenge}")
    
    # Initialize the table rows
    avg_row = ["Mean"]
    std_row = ["Std Dev"]
    
    # Check each mode and challenge combination
    for mode in modes:
        for challenge in challenges:
            results = results_collection.find({"user_id": user["_id"], "mode": mode, "challenge_num": challenge})
            avg_time_diff = "N/A"
            std_dev_time_diff = "N/A"
            
            for result in results:
                result_list = result["valid_results"]
                completion_times = [entry["completedTime"] for entry in result_list]
                
                if len(completion_times) > 1:
                    time_differences = [t2 - t1 for t1, t2 in zip(completion_times[:-1], completion_times[1:])]
                    avg_time_diff = mean(time_differences)
                    if len(time_differences) > 1:
                        std_dev_time_diff = stdev(time_differences)
                    else:
                        std_dev_time_diff = 0.0
                else:
                    avg_time_diff = "N/A"
                    std_dev_time_diff = "N/A"

            avg_row.append(f"{avg_time_diff:.2f}" if isinstance(avg_time_diff, float) else avg_time_diff)
            std_row.append(f"{std_dev_time_diff:.2f}" if isinstance(std_dev_time_diff, float) else std_dev_time_diff)
    
    # Print the user info and table using tabulate
    user_info = [["UserID", user["_id"]], ["First Name", user.get("first_name", "")], ["Last Name", user.get("last_name", "")]]
    print(tabulate.tabulate(user_info, tablefmt="grid"))
    print(tabulate.tabulate([avg_row, std_row], headers=headers, tablefmt="grid"))
  
@show.command()
@click.argument("user_id")
@click.option(
    "-m",
    type=click.Choice(["t", "to", "s", "so"], case_sensitive=False),
    required=True,
    help="Mode, t: Tracker, to: TrackerOffset, s: Static, so: Static Offset"
)
@click.option(
    "-n",
    type=int,
    required=True,
    help="Challenge number"
)
def task(user_id,m,n):
    mode = m 
    challenge = n
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    users_collection = db["users"]
    results_collection = db["valid_results"]

    # Retrieve the user
    user = users_collection.find_one({"_id": user_id})
    if not user:
        print(f"User ID {user_id} does not exist in the database.")
        return
    
    # Fetch the results
    results = results_collection.find({"user_id": user["_id"], "mode": study.mode_map[mode], "challenge_num": challenge})
    
    x_offset = 0.0
    if mode == "to" or mode == "so":
        x_offset = 60.0
        
    # Prepare data for PyVista
    eye_points = []
    index_finger_points = []
    middle_finger_points = []
    for result in results:
        tracker_logs = result.get("trackerLogs", {})
        left_eye_logs = tracker_logs.get("leftEye", [])
        for log in left_eye_logs:
            eye_points.append([log["x"] + x_offset, log["y"], log["z"]])
            
        hand_logs = tracker_logs.get("hand", [])
        for log in hand_logs:
            index_finger_points.append([log["index"]["x"] + x_offset, log["index"]["y"], log["index"]["z"]])
            middle_finger_points.append([log["middle"]["x"]+ x_offset, log["middle"]["y"], log["middle"]["z"]])
    
    if not eye_points:
        print("No points found for the given parameters.")
        return
	    
    visualize.plot_trace(eye_points, index_finger_points, middle_finger_points, challenge)

@show.command()
@click.option(
    "-n",
    type=int,
    required=True,
    help="Challenge number"
)
def demo(n):
    visualize.plot_demo_trace(n)
    
############
### Eval ###
############   
@show.group()
def eval():
    """Generate graphs"""
    pass

@eval.command()
def framerate():
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    eval_collection = db["evaluation_snapshot_1_overall_latency"]
    
    results = eval_collection.find().sort("_id", -1).limit(5)

    challenge_times = []
    for result in results:
        times = []
        tracker_logs = result.get("trackerLogs", {})
        left_eye_logs = tracker_logs.get("leftEye", [])
        for log in left_eye_logs:
            times.append(log["time"])
        
        challenge_times.append(times)   
        
    graph.graph_framerate_overall(challenge_times)
    
    
@eval.command()
def pydown():
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    eval_collection = db["evaluation_snapshot_2_pydown"]
    
    results = eval_collection.find().sort("_id", -1).limit(5)
    
    challenge_times = {}
    for result in results:
        times = []
        tracker_logs = result.get("trackerLogs", {})
        left_eye_logs = tracker_logs.get("leftEye", [])
        for log in left_eye_logs:
            times.append(log["time"])
        
        challenge_times[int(result.get("pydown", {}))] = times 
    
    graph.graph_framerate_pydown(challenge_times)

@eval.command()
def times():
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    eval_collection = db["evaluation_snapshot_3_process_times"]
    
    results = eval_collection.find().sort("_id", -1).limit(5)
    
    capture_times = []
    tracking_times = []
    render_times = []
    for result in results:
        render_logs = result.get("renderLogs", {})
        for log in render_logs:
            render_times.append(log)
        tracker_logs = result.get("trackerLogs", {})
        capture_logs = tracker_logs.get("capture", [])
        for log in capture_logs:
            capture_times.append(log)
        tracking_logs = tracker_logs.get("tracking", [])
        for log in tracking_logs:
            tracking_times.append(log)
    
    graph.graph_process_times(capture_times,tracking_times,render_times)
    

@eval.command()
def angles():
    
    def calculate_angle(a, b, c):
        # Using the Law of Cosines to calculate the angle opposite to side c
        angle = math.acos((a**2 + b**2 - c**2) / (2 * a * b))
        return math.degrees(angle)
    # Dictionary containing the lengths of the sides of the triangles for each user
    triangle_lengths = {
        "A": {"left": [50.0, 57.0, 101.5], "right": [50.0, 58.0, 98.5]}, #Elizibeta
        "B": {"left": [50.0, 59.5, 100.0], "right": [50.0, 61.0, 101.5]}, #Stylianos
        "C": {"left": [50.0, 55.5, 99.0], "right": [50.0, 52.0, 89.0]}, #Hoshin
        "D": {"left": [50.0, 58.0, 104.0], "right": [50.0, 55.0, 96.0]} #Robby
        # Add more users as needed
    }

    angles = {}
    
    for user, lengths in triangle_lengths.items():
        angles[user] = {}
        # Calculate the left angle
        left_side_a, left_side_b, left_side_c = lengths["left"]
        left_angle = calculate_angle(left_side_a, left_side_b, left_side_c)
        angles[user]["left"] = left_angle

        # Calculate the right angle
        right_side_a, right_side_b, right_side_c = lengths["right"]
        right_angle = calculate_angle(right_side_a, right_side_b, right_side_c)
        angles[user]["right"] = right_angle
    
    graph.graph_angles(angles)
    
@eval.command()
def tracking():
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    eval_collection = db["evaluation_snapshot_4_track_distances"]
    results = eval_collection.find().sort("distance", -1).limit(100)
    
    # Worked out it's actually + 50
    distance_offset = 50.0
    distance_head = {}
    distance_hand = {}
    for result in results:
        head = []
        hands = []
        tracker_logs = result.get("trackerLogs", {})
        capture_logs = tracker_logs.get("headTrack", [])
        for log in capture_logs:
            head.append(log)
        distance_head[result.get("distance", {})+distance_offset] = head
        capture_logs = tracker_logs.get("handTrack", [])
        for log in capture_logs:
            hands.append(log)
        distance_hand[result.get("distance", {})+distance_offset] = hands
	
    graph.graph_tracking(distance_head, distance_hand)

@eval.command()
def failrate():
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    results = db["valid_results"].find()
    
    fail_rate = {}
    for result in results:
        id = result["user_id"]
        if id not in fail_rate:
            fail_rate[id] = {"head": [], "hand": []}

        tracker_logs = result.get("trackerLogs", {})
        head_track_logs = tracker_logs.get("headTrack", [])
        for log in head_track_logs:
            fail_rate[id]["head"].append(log)
        
        hand_track_logs = tracker_logs.get("handTrack", [])
        for log in hand_track_logs:
            fail_rate[id]["hand"].append(log)
    
    graph.graph_failrate(fail_rate)


@eval.group()
def task():
    """Generate graphs for task completion times."""
    pass

@task.command()
def times():
    task_results = utility.flatten_id(utility.get_filtered_segment_times())
    
    graph.graph_task_times(task_results)

@task.command()
def hand():
    hand_results = utility.get_filtered_hand_positions()
    print(hand_results)
    
############
### Save ###
############ 
@cli.group()
def save():
	"""Save various resources."""
	pass

@save.command()
@click.argument("user_id")
def user(user_id):
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    users_collection = db["users"]
    results_collection = db["results_2"]

    # Retrieve the user
    user = users_collection.find_one({"_id": user_id})
    if not user:
        print(f"User ID {user_id} does not exist in the database.")
        return
    
    user_results = results_collection.find({"user_id": user_id})
    
    if not user_results:
        print(f"No results found for User ID {user_id}.")
        return
    
    valid_results = db["valid_results"]
    valid_results.insert_many(user_results)
    
if __name__ == "__main__":
    cli()