
import json
import click
import tabulate
import random

#User created
import utility
import study

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
    study.run_simulation("t", 1)
    utility.visualize_point_cloud_pyvista("misc/pointCloud.csv")   
    
@run.command()
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
def demo(m,n):
    study.run_simulation(m, -n)

@run.command()
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
def task(m, n, u, test):
    mode = m
    num = n
    user = u
    """ Run the simulation with specific mode, challenge number, and user ID. """
    print("Starting")
    
    if not test:
        db = utility.connect_to_mongo()
        users_collection = db["users"]
        results_collection = db["results"]
        
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
        if (not data["finished"]):
            print("Simulation did not finish successfully: exiting")
            return
        
        data['user_id'] = user  # Add the user ID to the data dictionary
        data['mode'] = study.mode_map[mode]  # Store the mode as well
        data["challenge_num"] = num  # Store the challenge number as well

        result = results_collection.insert_one(data)
        print("Inserted record id:", result.inserted_id)
    

@run.command()
@click.argument("user_id")
def next(user_id):
    # Connect to the MongoDB database
    db = utility.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    users_collection = db["users"]
    results_collection = db["results"]
    
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
    for mode in task_order:
        for challenge_num in range(1, 6):
            result_exists = results_collection.count_documents({
                "user_id": user_id,
                "mode": mode,
                "challenge_num": challenge_num
            }) > 0
            
            if not result_exists:
                print(f"Running simulation for mode {mode} and challenge number {challenge_num} for user {user_id}.")
                output = study.run_simulation(study.mode_map_inverse[mode], challenge_num)
                
                # Convert the JSON string to a Python dictionary
                data = json.loads(output)
                if not data["finished"]:
                    print("Simulation did not finish successfully: exiting")
                    return
                
                data['user_id'] = user_id
                data['mode'] = mode
                data['challenge_num'] = challenge_num
                
                result = results_collection.insert_one(data)
                print("Inserted record id:", result.inserted_id)
                return

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
    mydatabase = utility.connect_to_mongo()
    user_collection = mydatabase["users"]
    
    random.seed()
    order = random.sample(["TRACKER", "OFFSET", "STATIC"], k=3)
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
    mydatabase = utility.connect_to_mongo()
    user_collection = mydatabase["users"]

    for user in user_collection.find():
        print(json.dumps(user, indent=4, default=utility.json_encoder))

@list.command()
def results():
    """ Displays a table with the completion status of each task for every user. """
    db = utility.connect_to_mongo()
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

if __name__ == "__main__":
    cli()