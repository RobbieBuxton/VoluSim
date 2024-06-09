from bson import ObjectId
import simpleaudio as sa
import os
import numpy as np
from scipy import stats
import pandas as pd
import math
import mongodb
import statistics

# search_condition = {"user_id": "alenea"}
search_condition = {}

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
    
    results = db["valid_results"].find(search_condition)
    
    
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

def get_eye_positions():
    seg_ranges = segment_ranges()
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    valid_results = db["valid_results"].find(search_condition)
    eye_results = {}  # Initialize the eye_results dictionary
    for result in valid_results:
        id = result["user_id"]
        num = result["challenge_num"]
        mode = result["mode"]
        
        if (mode == "STATIC" or mode == "STATIC_OFFSET"):
            continue
        
        if id not in eye_results:
            eye_results[id] = {}
        if num not in eye_results[id]:
            eye_results[id][num] = {}
        if mode not in eye_results[id][num]:
            eye_results[id][num][mode] = [[] for _ in range(len(seg_ranges[id][num][mode])+1)]
            
        trackerLogs = result["trackerLogs"]
        eyePoss = trackerLogs["leftEye"]
        
        for pos in eyePoss:
            if pos["time"] < seg_ranges[id][num][mode][0][0]:
                eye_results[id][num][mode][0].append((pos["time"],(pos["x"],pos["y"],pos["z"])))
            for i in range(len(seg_ranges[id][num][mode])):
                if seg_ranges[id][num][mode][i][0] <= pos["time"] <= seg_ranges[id][num][mode][i][1]:
                    eye_results[id][num][mode][i].append((pos["time"],(pos["x"],pos["y"],pos["z"])))
                    break
            else:
                eye_results[id][num][mode][-1].append((pos["time"],(pos["x"],pos["y"],pos["z"])))
    return eye_results
    

def get_hand_failed_segments():
    # Get the hand fail times
    hand_fail_times = get_hand_fails_times(threshold=5)
    
    # Get the segment ranges
    segment_ranges_data = segment_ranges()
    
    # Dictionary to store failed segments
    hand_failed_segments = {}

    for user_id, challenges in segment_ranges_data.items():
        hand_failed_segments[user_id] = {}

        for challenge_num, modes in challenges.items():
            hand_failed_segments[user_id][challenge_num] = {}

            for mode, segments in modes.items():
                hand_failed_segments[user_id][challenge_num][mode] = []

                for start_time, end_time in segments:
                    # Check if any failure times fall within this segment
                    has_failures = any(start_time <= failure_time <= end_time for failure_time in hand_fail_times[user_id][challenge_num][mode])
                    # if has_failures:
                        # print("Found one: ", user_id, challenge_num, mode)
                    hand_failed_segments[user_id][challenge_num][mode].append(has_failures)
    
    return hand_failed_segments
 
def get_filtered_hand_positions():
    seg_ranges = segment_ranges()
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return
    
    valid_results = db["valid_results"].find(search_condition)
    
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
            
            for i in range(len(seg_ranges[id][num][mode])):
                if seg_ranges[id][num][mode][i][0] <= pos["time"] <= seg_ranges[id][num][mode][i][1]:
                    hand_results[id][num][mode][i+1].append((pos["time"],middle))
                    break
            else:
                hand_results[id][num][mode][0].append((pos["time"],middle))
    
    return hand_results

def calculate_distance(p1, p2):
    """Calculates the Euclidean distance between two points p1 and p2 in 3D space."""
    return math.sqrt((p1[0] - p2[0]) ** 2 + (p1[1] - p2[1]) ** 2 + (p1[2] - p2[2]) ** 2)

def get_filtered_distance():
    hand_positions = get_filtered_hand_positions()
    task_positions = dict([(idx, get_task_positions(idx)[1:]) for idx in range(1, 6)])

    filtered_distance = {}

    for hand_id, challenges in hand_positions.items():
        filtered_distance[hand_id] = {}
        
        for challenge_num, modes in challenges.items():
            filtered_distance[hand_id][challenge_num] = {}
            
            task_positions_for_challenge = task_positions[challenge_num]
            
            for mode, hand_mode_positions in modes.items():
                # print("User: ", hand_id, "Challenge: ", challenge_num, "Mode: ", mode)
                filtered_distance[hand_id][challenge_num][mode] = []
                for hand_positions_list in hand_mode_positions:
                    distances = []
                    for time, hand_position in hand_positions_list:
                        # Assuming task_positions_for_challenge is a list of target positions for this mode
                        corresponding_task_position = task_positions_for_challenge[modes[mode].index(hand_positions_list)]
                        
                        distance = calculate_distance(hand_position, corresponding_task_position)
                        distances.append((time, distance))
                    # print([distance for _, distance in distances[-5:]])
                    
                    filtered_distance[hand_id][challenge_num][mode].append(distances)
    
    return filtered_distance

def segment_ranges():
    db = mongodb.connect_to_mongo()
    if db is None:
        print("Failed to connect to MongoDB.")
        return None

    results = db["valid_results"].find(search_condition)
    
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
                        # print(id, len(filtered_hand_fail))
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

def calculate_time_differences(times):
    # Calculate the average of the differences for each task, ignoring None values
    averaged_differences = []
    for diff in zip(*times):
        valid_diffs = [d for d in diff if d is not None]

        if (len(valid_diffs) != len(diff)):
            print(f"ERROR: None values in {diff}")  
        if valid_diffs:
            average_diff = sum(valid_diffs) / len(valid_diffs)
        else:
            print("This shouldn't happen")
            average_diff = None
        averaged_differences.append(average_diff)
    return averaged_differences

def combine_task_results(task_results, combine_by_offset=False):
    combined_results = {}

    for id_value, challenges in task_results.items():
        combined_results[id_value] = {}
        for challenge_num, modes in challenges.items():
            combined_results[id_value][challenge_num] = {}

            if combine_by_offset:
                # Combine TRACKER and STATIC with their respective offsets
                not_offset = modes['TRACKER'] + modes['STATIC']
                offset = modes['TRACKER_OFFSET'] + modes['STATIC_OFFSET']
                
                combined_results[id_value][challenge_num]['NOT_OFFSET'] = not_offset
                combined_results[id_value][challenge_num]['OFFSET'] = offset
            else:
                tracker_combined = modes['TRACKER'] + modes['TRACKER_OFFSET']
                static_combined = modes['STATIC'] + modes['STATIC_OFFSET']
            
                combined_results[id_value][challenge_num]['TRACKER'] = tracker_combined
                combined_results[id_value][challenge_num]['STATIC'] = static_combined

    return combined_results

def combine_challenges_results(challenge_results):
    combined_data = {}

    for key, challenges in challenge_results.items():
        combined_data[key] = {}
        for challenge in sorted(challenges.keys()):
            for sub_key, values in challenges[challenge].items():
                if sub_key not in combined_data[key]:
                    combined_data[key][sub_key] = []
                combined_data[key][sub_key] += values

    return combined_data

def results_averages(results):
    from collections import defaultdict
    import numpy as np

    averages = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))

    # Collect all values for each index, ignoring None
    for user, tasks in results.items():
        for task, metrics in tasks.items():
            for metric, values in metrics.items():
                for i, value in enumerate(values):
                    if value is not None:
                        averages[task][metric][i].append(value)
    
    # Calculate the averages
    average_results = {}
    for task, metrics in averages.items():
        average_results[task] = {}
        for metric, indices in metrics.items():
            average_results[task][metric] = []
            for i in range(len(indices)):
                if indices[i]:
                    average_results[task][metric].append(np.mean(indices[i]))
                else:
                    average_results[task][metric].append(None)  # Handle cases where all values were None
    
    return average_results

def fill_nones(results):
    res_averages = results_averages(results)
    
    filled_results = {}

    for user, tasks in results.items():
        filled_results[user] = {}
        for task, metrics in tasks.items():
            filled_results[user][task] = {}
            for metric, values in metrics.items():
                filled_results[user][task][metric] = []
                for i, value in enumerate(values):
                    if value is None:
                        # Replace None with the average from res_averages
                        filled_results[user][task][metric].append(res_averages[task][metric][i])
                    else:
                        filled_results[user][task][metric].append(value)
    
    return filled_results

def get_condition_sums(data):
    condition_sums = {}

    for user in data:
        for condition in data[user]:
            if condition not in condition_sums:
                condition_sums[condition] = []
            condition_sums[condition].append(sum(data[user][condition]))

    return condition_sums

def perform_ttest(data):
    
    results = {}
    conditions = list(data.keys())

    # Perform pairwise t-tests between all conditions
    for i in range(len(conditions)):
        for j in range(i+1, len(conditions)):
            cond1 = conditions[i]
            cond2 = conditions[j]
            t_stat, p_value = stats.ttest_ind(data[cond1], data[cond2])
            results[(cond1, cond2)] = (t_stat, p_value)

    return results

def perform_anova(data):
    """
    Perform ANOVA test on the provided dataset.

    Parameters:
    data (dict): A dictionary containing lists of values for each group.

    Returns:
    tuple: F-value and P-value from the ANOVA test.
    """
    # Extract lists from the dictionary
    tracker = data['TRACKER']
    tracker_offset = data['TRACKER_OFFSET']
    static = data['STATIC']
    static_offset = data['STATIC_OFFSET']

    # Perform ANOVA
    f_val, p_val = stats.f_oneway(tracker, tracker_offset, static, static_offset)

    # Print the results
    print(f"F-value: {f_val}")
    print(f"P-value: {p_val}")

    # Interpretation
    if p_val < 0.05:
        print("The test result is significant; at least one of the group means is different.")
    else:
        print("The test result is not significant; no significant difference between the group means.")
    
    return f_val, p_val

def load_excel():
    dir_path = os.path.dirname(os.path.realpath(__file__))
    excel_path = os.path.join(dir_path, f"survey/main.xlsx")
    df = pd.read_excel(excel_path)
    
    data_dict = df.to_dict(orient='records')
    
    return data_dict
    
def calculate_movement(points):
    movement_array = []
    for i in range(1, len(points)):
        x1, y1, z1 = points[i-1][1]
        x2, y2, z2 = points[i][1]
        distance = math.sqrt((x2 - x1)**2 + (y2 - y1)**2 + (z2 - z1)**2)
        time_diff = points[i][0] - points[i-1][0]
        movement_array.append(distance/time_diff)
    return movement_array

def compute_movement_metrics(data):
    results = {}
    for user, tasks in data.items():
        results[user] = {}
        for task_number, conditions in tasks.items():
            results[user][task_number] = {}
            for condition, point_segments in conditions.items():
                segments = []
                for segment in point_segments:
                    segment_movements = calculate_movement(segment)
                    segments.append(
                        statistics.mean(segment_movements)
                        # "median": statistics.median(segment_movements),
                        # "std": np.std(segment_movements)
                    )
                results[user][task_number][condition] = segments
    return results