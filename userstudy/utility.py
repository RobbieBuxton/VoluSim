from bson import ObjectId
import simpleaudio as sa
import os
import numpy as np

import mongodb

# Helper function to handle MongoDB ObjectId when serializing to JSON
def json_encoder(o):
    if isinstance(o, ObjectId):
        return str(o)
    return o

def play_beep():
    # Generate a beep sound
    play_sound("countdown.wav")

def play_finished():
	# Generate a finish sound
	play_sound("finished.wav")

def play_sound(sound_path):
	# Generate a beep sound
    dir_path = os.path.dirname(os.path.realpath(__file__))
    wav_path = os.path.join(dir_path, f"data/{sound_path}")
    wave_obj = sa.WaveObject.from_wave_file(wav_path)
    play_obj = wave_obj.play()
    play_obj.wait_done()
    
def get_hand_fails_times(threshold=5):
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    results = db["valid_results"].find()
    
    
    hand_fail = {}  # Initialize the hand_fail dictionary
    for result in results:
        id = result["user_id"]
        num = result["challenge_num"]
        mode = result["mode"]
        
        if id not in hand_fail:
            hand_fail[id] = {}
        if num not in hand_fail[id]:
            hand_fail[id][num] = {}
        if mode not in hand_fail[id][num]:
            hand_fail[id][num][mode] = []

        tracker_logs = result.get("trackerLogs", {})
        hand_track_logs = tracker_logs.get("handTrack", [])
        for log in hand_track_logs:
            success = log.get("success", False)
            time = log.get("time", 0)
            if not success:
                hand_fail[id][num][mode].append(time)
        if len(hand_fail[id][num][mode]) < threshold:
            hand_fail[id][num][mode] = []
        # print(id, len(hand_fail[id][num][mode]))
    return hand_fail

def get_filtered_hand_positions():
    seg_ranges = segment_ranges()
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    valid_results = db["valid_results"].find()
    
    hand_results = {}  # Initialize the segment_results dictionary
    for result in valid_results:
        id = result["user_id"]
        num = result["challenge_num"]
        mode = result["mode"]
        
        if id not in hand_results:
            hand_results[id] = {}
        if num not in hand_results[id]:
            hand_results[id][num] = {}
        if mode not in hand_results[id][num]:
            hand_results[id][num][mode] = [[] for _ in range(len(seg_ranges[id][num][mode])+1)]
        
        trackerLogs = result["trackerLogs"]
        hand = trackerLogs["hand"]
        for pos in hand:
            x_offset = 0
            if mode == "TRACKER_OFFSET" or mode == "STATIC_OFFSET":
                x_offset = 60
            middle = np.array([(pos["middle"]["x"] + pos["index"]["x"])/2.0 + x_offset, (pos["middle"]["y"] + pos["index"]["y"])/2.0, (pos["middle"]["z"] + pos["index"]["z"])/2.0])
            
            if pos["time"] < seg_ranges[id][num][mode][0][0]:
                hand_results[id][num][mode][0].append((pos["time"],middle))
            for i in range(len(seg_ranges[id][num][mode])):
                if seg_ranges[id][num][mode][i][0] <= pos["time"] <= seg_ranges[id][num][mode][i][1]:
                    hand_results[id][num][mode][i].append((pos["time"],middle))
                    break
            else:
                hand_results[id][num][mode][-1].append((pos["time"],middle))
    
    return hand_results
    
def segment_ranges():
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return None

    results = db["valid_results"].find()
    
    segment_results = {}  # Initialize the segment_results dictionary
    for result in results:
        id = result["user_id"]
        num = result["challenge_num"]
        mode = result["mode"]
        
        if id not in segment_results:
            segment_results[id] = {}
        if num not in segment_results[id]:
            segment_results[id][num] = {}
        if mode not in segment_results[id][num]:
            segment_results[id][num][mode] = []
        
        challenge_result = result["results"]
        for i in range(1, len(challenge_result)):
            start_time = challenge_result[i - 1]['completedTime']
            end_time = challenge_result[i]['completedTime']
            segment_results[id][num][mode].append((start_time, end_time))
    
    return segment_results

def get_filtered_segment_times():
    hand_fail = get_hand_fails_times(threshold=5)
    segment_results = segment_ranges()
    
    if segment_results is None:
        return
    
    filtered_segment_results = {}
    debug_diffs = []

    for id, challenges in segment_results.items():
        if id not in filtered_segment_results:
            filtered_segment_results[id] = {}
        for num, modes in challenges.items():
            if num not in filtered_segment_results[id]:
                filtered_segment_results[id][num] = {}
            for mode, times in modes.items():
                if mode not in filtered_segment_results[id][num]:
                    filtered_segment_results[id][num][mode] = []
                
                for start_time, end_time in times:
                    filtered_hand_fail = [time for time in hand_fail[id][num][mode] if start_time <= time <= end_time]
                    if filtered_hand_fail:
                        diff = None
                    else:
                        diff = end_time - start_time
                        debug_diffs.append(diff)
                    
                    filtered_segment_results[id][num][mode].append(diff)
    
    return filtered_segment_results

def flatten_id(segment_results):
    flattened_results = {}
    for _, challenges in segment_results.items():
        for challenge, modes in challenges.items():
            if challenge not in flattened_results:
                flattened_results[challenge] = {}
            for mode, times in modes.items():
                if mode not in flattened_results[challenge]:
                    flattened_results[challenge][mode] = []
                flattened_results[challenge][mode].append(times)
    return flattened_results

#ignores the first direction
def get_task_directions_tail():
    task_directions = []
    for i in range(1,5):
        task_directions.append([dir for (dir,_) in get_directions(i)][1:])
    
    return task_directions
    

def get_directions(challenge, demo=False):
    dir_path = os.path.dirname(os.path.realpath(__file__))
    parent_dir = os.path.dirname(dir_path)
    if demo:
        lib_path = os.path.join(parent_dir, "result/data/challenges/demo" + str(int(challenge)) + ".txt")
    else:
        lib_path = os.path.join(parent_dir, "result/data/challenges/task" + str(int(challenge)) + ".txt")
    
    directions = []
    
    try:
        with open(lib_path, 'r') as file:
            for line in file:
                parts = line.strip().split()
                if len(parts) == 2:
                    keyword, length_str = parts
                    try:
                        length = float(length_str)
                        directions.append((keyword, length))
                    except ValueError:
                        pass  # Invalid length format, ignoring
                else:
                    pass  # Invalid line format, ignoring
    except IOError as e:
        pass  # Handle file I/O error if necessary

    return directions

def get_mapped_directions(challenge, demo=False):
    direction_map = {
        "up": np.array([0.0, 1.0, 0.0]),
        "down": np.array([0.0, -1.0, 0.0]),
        "forward": np.array([0.0, 0.0, 1.0]),
        "back": np.array([0.0, 0.0, -1.0]),
        "right": np.array([1.0, 0.0, 0.0]),
        "left": np.array([-1.0, 0.0, 0.0])
    }

    directions = get_directions(challenge, demo)
    mapped_directions = []
    
    for keyword, length in directions:
        if keyword in direction_map:
            mapped_directions.append(direction_map[keyword] * length)
    
    return mapped_directions

def get_task_positions(challenge,demo=False):
    directions = get_mapped_directions(challenge,demo)
    start = np.array([-3.0, 7.0, 2.0])
    
    # Compute the cumulative points
    cumulative_points = [start]
    current_position = start
    for direction in directions:
        current_position = current_position + direction
        cumulative_points.append(current_position)

    return cumulative_points