import pandas as pd
import matplotlib.pyplot as plt
from collections import Counter
import numpy as np

def graph_framerate_overall(challenge_times):
    
    plt.rcParams.update({
        "text.usetex": True,
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman"],
        "axes.labelsize": 20,
        "font.size": 15,
        "legend.fontsize": 15,
        "xtick.labelsize": 15,
        "ytick.labelsize": 15,
        "figure.figsize": (13, 6)
    })
    
    # Calculate time differences (latencies)
    latencies = []
    for times in challenge_times:
        for t1, t2 in zip(times[:-1], times[1:]):
            latency = t2 - t1
            latencies.append(latency)
    
    # Create a DataFrame for latencies
    df_latencies = pd.DataFrame(latencies, columns=['Latency'])

    # Count the frequency of each latency
    latency_counter = Counter(df_latencies['Latency'])
    latency_freq = latency_counter.most_common()

    for latency, freq in latency_freq:
        print(f'{latency} ms: {freq} times')


    # Filter latencies where the frequency is less than 5
    filtered_latencies = [latency for latency, freq in latency_freq if freq >= 5]
    
    # Create a filtered DataFrame
    df_filtered_latencies = df_latencies[df_latencies['Latency'].isin(filtered_latencies)]

    # Plotting the histogram
    plt.figure(figsize=(13, 6))
    plt.hist(df_filtered_latencies['Latency'], bins=68, edgecolor='black', color='#0073c0ff')
    plt.title('Tracker Framerate Distribution (5 mins)')
    plt.xlabel('Time (ms)')
    plt.ylabel('Frequency ')
    plt.grid(True)
    # Set x-axis ticks for every integer value
    plt.xticks(range(int(df_filtered_latencies['Latency'].min()), int(df_filtered_latencies['Latency'].max()) + 1))

    # Save to PNG with transparent background and tight bounding box
    plt.savefig('misc/graphs/framerate-overall.pdf', transparent=True, bbox_inches='tight')

    # Show the plot
    plt.show()
    
def graph_framerate_pydown(challenge_times_dict):
    
    # Map these keys for the legend
    key_mapping = {
        0: '2048 x 1536',
        1: '1024 x 793',
        2: '512 x 396',
        3: '256 x 198'
    }
    
    plt.rcParams.update({
        "text.usetex": True,
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman"],
        "axes.labelsize": 20,
        "font.size": 15,
        "legend.fontsize": 15,
        "xtick.labelsize": 15,
        "ytick.labelsize": 15,
        "figure.figsize": (10, 6)
    })
    
    # Define a list of colors for plotting
    colors = ['#8f45a3ff', '#ff7c37ff', '#0073c0ff', '#2aaa00ff']
    
    plt.figure(figsize=(10, 6))
    
    all_latencies = []

    # First pass to collect all latencies and find the global min and max latency
    for key, challenge_times in challenge_times_dict.items():
        for t1, t2 in zip(challenge_times[:-1], challenge_times[1:]):
            latency = t2 - t1
            if latency <= 80:  # Filter out values greater than 100 ms
                all_latencies.append(latency)

    # Determine the global min and max latency
    if all_latencies:
        min_latency = int(min(all_latencies))
        max_latency = int(max(all_latencies))
    else:
        min_latency, max_latency = 0, 80
    
    # Define the bins based on the global min and max latency
    bins = range(min_latency, max_latency + 2)  # +2 to include the max_latency in the range

    # Prepare data for all groups
    all_group_data = []
    for idx, (key, challenge_times) in enumerate(challenge_times_dict.items()):
        latencies = []
        for t1, t2 in zip(challenge_times[:-1], challenge_times[1:]):
            latency = t2 - t1
            if latency <= 80:
                latencies.append(latency)
        
        # Create a DataFrame for latencies
        df_latencies = pd.DataFrame(latencies, columns=['Latency'])

        # Count the frequency of each latency
        latency_counter = Counter(df_latencies['Latency'])
        latency_freq = latency_counter.most_common()

        for latency, freq in latency_freq:
            print(f'Set {key_mapping[key]} - {latency} ms: {freq} times')

        # Filter latencies where the frequency is less than 5
        filtered_latencies = [latency for latency, freq in latency_freq if freq >= 5]

        # Create a filtered DataFrame
        df_filtered_latencies = df_latencies[df_latencies['Latency'].isin(filtered_latencies)]
        
        # Collect data for each group
        group_data = pd.Series(dict(Counter(df_filtered_latencies['Latency'])))
        all_group_data.append((key_mapping[key], group_data, colors[idx]))

    # Plot the histogram bin by bin
    for bin in bins:
        bin_heights = []
        for key, group_data, color in all_group_data:
            if bin in group_data:
                bin_heights.append((group_data[bin], color, key))
        
        # Sort bin heights in descending order and plot
        bin_heights.sort(reverse=True, key=lambda x: x[0])
        for height, color, label in bin_heights:
            plt.bar(bin, height, edgecolor='black', color=color, label=label)

    # Handle the legend
    handles, labels = plt.gca().get_legend_handles_labels()
    by_label = dict(zip(labels, handles))
    plt.legend(reversed(by_label.values()), reversed(by_label.keys()), title="Resolution")

    plt.title('Tracker Framerate Distribution (1 min)')
    plt.xlabel('Time (ms)')
    plt.ylabel('Frequency')
    plt.grid(True)
    
    # Save to PNG with transparent background and tight bounding box
    plt.savefig('misc/graphs/pydown.pdf', transparent=True, bbox_inches='tight')

    # Show the plot
    plt.show()
    
def graph_process_times(capture_times, tracking_times, render_times):
	plt.rcParams.update({
		"text.usetex": True,
		"font.family": "serif",
		"font.serif": ["Computer Modern Roman"],
		"axes.labelsize": 20,
		"font.size": 15,
		"legend.fontsize": 15,
		"xtick.labelsize": 15,
		"ytick.labelsize": 15,
		"figure.figsize": (13, 8)
	})

	def calculate_process_times(times, key):
		return [entry[key] for entry in times if entry[key] <= 50]

	# Calculate latencies for each component and filter out values over 100 ms
	capture_latencies = calculate_process_times(capture_times, 'captureTime')
	track_latencies = calculate_process_times(tracking_times, 'trackTime')
	render_latencies = calculate_process_times(render_times, 'renderTime')
	
	all_latencies = capture_latencies + track_latencies + render_latencies

	# Determine the global min and max latency
	if all_latencies:
		min_latency = int(min(all_latencies))
		max_latency = int(max(all_latencies))
	else:
		min_latency, max_latency = 0, 80
	
	# Define the bins based on the global min and max latency
	bins = range(min_latency, max_latency + 2)  # +2 to include the max_latency in the range

	# Create dataframes for each component
	df_capture = pd.DataFrame(capture_latencies, columns=['Latency'])
	df_track = pd.DataFrame(track_latencies, columns=['Latency'])
	df_render = pd.DataFrame(render_latencies, columns=['Latency'])

	# Prepare data for all groups
	all_group_data = []
	for df, label, color in zip(
		[df_capture, df_track, df_render], 
		['Capture Time', 'Track Time', 'Render Time'],
		['#8f45a3ff', '#0073c0ff', '#2aaa00ff']
	):
		# Count the frequency of each latency
		latency_counter = Counter(df['Latency'])
		latency_freq = latency_counter.most_common()

		for latency, freq in latency_freq:
			print(f'{label} - {latency} ms: {freq} times')

		# Filter latencies where the frequency is less than 5
		filtered_latencies = [latency for latency, freq in latency_freq if freq >= 5]

		# Create a filtered DataFrame
		df_filtered_latencies = df[df['Latency'].isin(filtered_latencies)]
		
		# Collect data for each group
		group_data = pd.Series(dict(Counter(df_filtered_latencies['Latency'])))
		all_group_data.append((label, group_data, color))

	plt.figure(figsize=(13, 8))
	
	# Plot the histogram bin by bin
	for bin in bins:
		bin_heights = []
		for label, group_data, color in all_group_data:
			if bin in group_data:
				bin_heights.append((group_data[bin], color, label))
		
		# Sort bin heights in descending order and plot
		bin_heights.sort(reverse=True, key=lambda x: x[0])
		for height, color, label in bin_heights:
			plt.bar(bin, height, edgecolor='black', color=color, label=label)

	# Handle the legend
	handles, labels = plt.gca().get_legend_handles_labels()
	by_label = dict(zip(labels, handles))
	plt.legend(by_label.values(), by_label.keys(), title="Latency Type")

	plt.title('Process Times Distributions (3 mins)')
	plt.xlabel('Time (ms)')
	plt.ylabel('Frequency')
	plt.grid(True)

	plt.savefig('misc/graphs/process-times-distributions.pdf', transparent=True, bbox_inches='tight')
	plt.show()

def graph_angles(angles):
    
    # angles = {"A" :{"left": [50.0, 57.0, 101.5], "right": [50.0, 58.0, 98.5]}, "B": ...}
    print(angles)
    plt.rcParams.update({
        "text.usetex": True,
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman"],
        "axes.labelsize": 20,
        "font.size": 15,
        "legend.fontsize": 15,
        "xtick.labelsize": 15,
        "ytick.labelsize": 15,
        "figure.figsize": (10, 10)
    })
    
    fig, ax = plt.subplots(figsize=(8, 8), subplot_kw={'projection': 'polar'})
    
    colors = ['#8f45a3ff', '#ff7c37ff', '#0073c0ff', '#2aaa00ff']
    
    for i, (user, angle_dict) in enumerate(angles.items()):
        color = colors[i % len(colors)]
        left_angle = angle_dict['left']
        right_angle = angle_dict['right']
        
        # Convert angles to radians with 0 degrees at the top
        left_angle_rad = np.radians(90 - left_angle)
        right_angle_rad = np.radians(90 - (360 - right_angle))  # Adjust for clockwise direction
        
        # Calculate points on the circumference
        left_r = 1
        right_r = 1
        
        # Plot points
        ax.plot(left_angle_rad, left_r, 'o', color=color, markersize=15, alpha=0.5, label=user)
        ax.plot(right_angle_rad, right_r, 'o', color=color, markersize=15, alpha=0.5)
        
    ax.set_ylim([0, 1.5])
    ax.set_yticks([])
    ax.set_xticks(np.radians(np.arange(0, 360, 30)))
    ax.set_xticklabels([f"{(90 - i) % 360}°" for i in range(0, 360, 30)])
    ax.grid(True)
    
    plt.title('Limit of head tracking angles (degrees)')
    
    # Handle the legend
    handles, labels = ax.get_legend_handles_labels()
    by_label = dict(zip(labels, handles))

    ax.legend(by_label.values(), by_label.keys(), loc='upper right', title="User")
    
    plt.savefig('misc/graphs/user-angles.pdf', transparent=True, bbox_inches='tight')
    plt.show()
    
def graph_tracking(distance_head, distance_hand):
    # distance_head = {2.0: [{'success': False, 'time': 1717085224584}, {'success': True, 'time': 1717085224610}]}
    # distance_hand = {2.0: [{'success': False, 'time': 1717085224584}, {'success': True, 'time': 1717085224610}]}
    
    plt.rcParams.update({
        "text.usetex": True,
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman"],
        "axes.labelsize": 20,
        "font.size": 15,
        "legend.fontsize": 15,
        "xtick.labelsize": 15,
        "ytick.labelsize": 15,
        "figure.figsize": (8, 4)
    })
    
    def calculate_success_rate(data):
        success_rates = {}
        for distance, attempts in data.items():
            total_attempts = len(attempts)
            successful_attempts = sum(1 for attempt in attempts if attempt['success'])
            success_rate = (successful_attempts / total_attempts) * 100 if total_attempts > 0 else 0
            success_rates[distance] = success_rate
        return success_rates

    # Calculate success rates
    head_success_rates = calculate_success_rate(distance_head)
    hand_success_rates = calculate_success_rate(distance_hand)

    # Prepare data for plotting
    distances = sorted(set(head_success_rates.keys()).union(hand_success_rates.keys()))
    head_rates = [head_success_rates.get(distance, 0) for distance in distances]
    hand_rates = [hand_success_rates.get(distance, 0) for distance in distances]

    # Plotting the success rates
    plt.figure(figsize=(13, 8))
    plt.plot(distances, head_rates, marker='o', linestyle='-', color='#0073c0ff', label='Head Tracking (Dlib)')
    plt.plot(distances, hand_rates, marker='o', linestyle='-', color='#ff7c37ff', label='Hand Tracking (MediaPipe)')
    
    # plt.title('Tracking Success Rate at Different Distances (30 secs)')
    plt.xlabel('Distance (cm)')
    plt.ylabel('Success Rate (\%)')
    plt.legend(title='Tracking Type')
    plt.grid(True)

    # Save to PDF with transparent background and tight bounding box
    plt.savefig('misc/graphs/tracking-success-rate.pdf', transparent=True, bbox_inches='tight')

    # Show the plot
    plt.show()