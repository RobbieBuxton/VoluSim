import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from collections import Counter
import numpy as np
import seaborn as sns
import statsmodels.api as sm
from statsmodels.formula.api import ols
from scipy.stats import ttest_ind

import utility

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


# fail_rate = {user_id: { head: [{'success': False, 'time': 1717085224584}...], hand: [{'success': True, 'time': 1717085224610}]}}
def graph_failrate(fail_rate):
    plt.rcParams.update({
        "text.usetex": True,
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman"],
        "axes.labelsize": 20,
        "font.size": 15,
        "legend.fontsize": 15,
        "xtick.labelsize": 15,
        "ytick.labelsize": 15,
        "figure.figsize": (6, 8)
    })
    
    def calculate_fail_rate(attempts):
        total_attempts = len(attempts)
        failed_attempts = sum(1 for attempt in attempts if not attempt['success'])
        print(failed_attempts, total_attempts)
        fail_rate = (failed_attempts / total_attempts) * 100 if total_attempts > 0 else 0
        return fail_rate

    # Calculate fail rates
    head_fail_rates = {user: calculate_fail_rate(attempts['head']) for user, attempts in fail_rate.items()}
    hand_fail_rates = {user: calculate_fail_rate(attempts['hand']) for user, attempts in fail_rate.items()}

    # Anonymize users
    users = list(fail_rate.keys())
    anonymized_users = [f'{i+1}' for i in range(len(users))]
    
    head_rates = [head_fail_rates[user] for user in users]
    hand_rates = [hand_fail_rates[user] for user in users]
    
    # Create a DataFrame for plotting
    df = pd.DataFrame({
        'User': anonymized_users,
        'Head': head_rates,
        'Hand': hand_rates
    })

    # Set the bar width
    bar_width = 0.35

    # Set position of bar on X axis
    r1 = np.arange(len(df))
    r2 = [x + bar_width for x in r1]

    # Create bars
    plt.figure(figsize=(13, 8))
    plt.bar(r1, df['Head'], color='#0073c0ff', width=bar_width, edgecolor='grey', label='Head')
    plt.bar(r2, df['Hand'], color='#ff7c37ff', width=bar_width, edgecolor='grey', label='Hand')

    # Add xticks on the middle of the group bars
    plt.xlabel('User', fontweight='bold')
    plt.xticks([r + bar_width/2 for r in range(len(df))], df['User'])

    # Add labels
    plt.ylabel('Fail Rate (\%)')
    plt.title('Head vs Hand Fail Rate per User')
    plt.legend()
    plt.grid(True)

    # Save to PDF with transparent background and tight bounding box
    plt.savefig('misc/graphs/failrate.pdf', transparent=True, bbox_inches='tight')

    # Show the plot
    plt.show()
                      
def graph_task_times(task_times):
    base_colors = ['#8f45a3ff', '#ff7c37ff', '#0073c0ff', '#2aaa00ff']
    hatches = [None, '///']  # Alternating patterns for each section (where a section is the time difference between index)

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

    for task_num, conditions in task_times.items():
        _, ax = plt.subplots(figsize=(13, 8))

        condition_labels = []

        for idx, (condition, times) in enumerate(conditions.items()):
            time_differences = utility.calculate_time_differences(times)
            bottom = 0
            for section_idx, diff in enumerate(time_differences):
                ax.bar(condition, diff, bottom=bottom, color=base_colors[idx % len(base_colors)], hatch=hatches[section_idx % len(hatches)])
                bottom += diff
            condition_labels.append(condition)
        
        # Set labels and title
        ax.set_xlabel('Task Condition')
        ax.set_ylabel('Time (ms)')
        ax.set_title(f'Task {task_num} Completion Times')
        ax.legend([plt.Rectangle((0, 0), 1, 1, color=base_colors[i % len(base_colors)]) for i in range(len(conditions))],
                  condition_labels, title='Conditions')

        # Save to PD
        plt.savefig(f'misc/graphs/task_{task_num}_times.pdf', transparent=True, bbox_inches='tight')
        
        # Show the plot
        plt.show()
        
def graph_ttest(task_times):
    base_colors = ['#8f45a3ff', '#ff7c37ff']  # Colors for STATIC and TRACKER
    hatches = [None, '///']  # Patterns for each condition

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

    for user, conditions in task_times.items():
        _, ax = plt.subplots(figsize=(13, 8))

        condition_labels = list(conditions.keys())

        for idx, condition in enumerate(condition_labels):
            times = conditions.get(condition, [])
            time_differences = times  # Assuming times are already the differences you want to plot
            bottom = 0
            for section_idx, diff in enumerate(time_differences):
                ax.bar(condition, diff, bottom=bottom, color=base_colors[idx], hatch=hatches[section_idx % len(hatches)])
                bottom += diff
        
        # Set labels and title
        ax.set_xlabel('Condition')
        ax.set_ylabel('Time (ms)')
        ax.set_title(f'User {user} Completion Times')
        ax.legend([plt.Rectangle((0, 0), 1, 1, color=base_colors[i]) for i in range(len(condition_labels))],
                  condition_labels, title='Conditions')

        # Save to PDF
        plt.savefig(f'misc/graphs/user_{user}_times.pdf', transparent=True, bbox_inches='tight')
        
        # Show the plot
        plt.show()
        
        
def graph_anova(results):
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

    rows = []
    for user, conditions in results.items():
        for task_id, times in conditions.items():
            for condition, time_list in times.items():
                for time in time_list:
                    rows.append({
                        'User': user,
                        'Task': task_id,
                        'Condition': condition,
                        'Time': time
                    })

    # Creating DataFrame
    df = pd.DataFrame(rows)

    # Performing ANOVA
    # Since you have two factors (e.g., TRACKER/STATIC and OFFSET/NOT_OFFSET), you need to separate them
    df['Type'] = df['Condition'].apply(lambda x: 'TRACKER' if 'TRACKER' in x else 'STATIC')
    df['Offset'] = df['Condition'].apply(lambda x: 'OFFSET' if 'OFFSET' in x else 'NOT_OFFSET')

    # Perform the ANOVA
    model = ols('Time ~ C(Type) * C(Offset)', data=df).fit()
    anova_table = sm.stats.anova_lm(model, typ=2)

    print(anova_table)

    # Plotting interaction plot
    plt.figure(figsize=(13, 8))
    sns.pointplot(data=df, x='Offset', y='Time', hue='Type', dodge=True, markers=['o', 's'], capsize=.1, errwidth=1, palette=['#ff7c37ff', '#0073c0ff', '#2aaa00ff'])
    # plt.title('Interaction between Type and Offset')
    plt.xlabel('Offset')
    plt.ylabel('Time (ms)')
    plt.legend(title='Type')

    # Save to PDF with transparent background and tight bounding box
    plt.savefig('misc/graphs/anova-interaction.pdf', transparent=True, bbox_inches='tight')

    # Show the plot
    plt.show()

#Data is in this form (this is just one user):
# {'Id': 3, 'Start time': Timestamp('2024-06-01 14:19:17'), 'Completion time': Timestamp('2024-06-01 14:21:39'), 'Email': 'anonymous', 'Name': nan, 'First Name': 'Charlie', 'Last\xa0Name': 'Lidbury', 'Gender': 'Man', 'Do you wear glasses': 'No', 'Have you used VR/AR before?': 'Yes', 'Age': 22, 'Handedness (which is your dominant hand)': 'Right', 'StudyID (this will be generated by Robbie)': 'charlid', 'Email (if you would like to be updated about this study)': 'charlie.lidbury@icloud.com', 'I completed the demo': 'Twice', 'I found completing the tasks for this condition difficult': 2, 'I found this condition frustrating': 4, 'It was unclear in which direction I needed to move my hand next': 1, 'I found completing the tasks for this condition difficult1': 4, 'I found this condition frustrating1': 6, 'It was unclear in which direction I needed to move my hand next1': 2, 'I found completing the tasks for this condition difficult2': 3, 'I found this condition frustrating2': 5, 'It was unclear in which direction I needed to move my hand next2': 6, 'I found completing the tasks for this condition difficult3': 4, 'I found this condition frustrating3': 6, 'It was unclear in which direction I needed to move my hand next3': 3, 'The task was able to track my hand accurately': 8, 'The task was able to track my hand reliably\xa0': 2, 'The task was able to track my eye\xa0accurately\n\n': 8, 'The task was able to track my eye reliably\xa0\n\n': 8, 'Rank the conditions in order of prefrence': 'Tracker Condition\xa0(View changes)\xa0;Tracker Offset Condition\xa0(View changes, offset)\xa0;Static Condition\xa0(View does not change)\xa0;Static Offset Condition\xa0(View does not change, offset)\xa0;'}
def graph_survey(data_list):
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

    # Extract data
    genders = [entry['Gender'] for entry in data_list]
    ages = [entry['Age'] for entry in data_list]
    glasses = [entry['Do you wear glasses'] for entry in data_list]
    vr_ar = [entry['Have you used VR/AR before?'] for entry in data_list]

    # Plot 1: Gender Distribution (Pie Chart)
    gender_counts = Counter(genders)
    fig, ax = plt.subplots(figsize=(8, 8))
    ax.pie(gender_counts.values(), labels=gender_counts.keys(), autopct='%1.1f%%', startangle=140, colors=['#8f45a3ff', '#0073c0ff', '#ff7c37ff'])
    # ax.set_title('Gender Distribution')
    plt.savefig('misc/graphs/gender-distribution.pdf', transparent=True, bbox_inches='tight')
    plt.show()

    # Plot 2: Age Distribution (Bar Chart)
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.hist(ages, bins=range(min(ages), max(ages) + 1, 1), edgecolor='black', color='#0073c0ff')
    # ax.set_title('Age Distribution')
    ax.set_xlabel('Age')
    ax.set_ylabel('Frequency')
    plt.savefig('misc/graphs/age-distribution.pdf', transparent=True, bbox_inches='tight')
    plt.show()

    # Plot 3: Glasses Distribution (Pie Chart)
    glasses_counts = Counter(glasses)
    fig, ax = plt.subplots(figsize=(8, 8))
    ax.pie(glasses_counts.values(), labels=glasses_counts.keys(), autopct='%1.1f%%', startangle=140, colors=['#8f45a3ff', '#0073c0ff'])
    # ax.set_title('Glasses Distribution')
    plt.savefig('misc/graphs/glasses-distribution.pdf', transparent=True, bbox_inches='tight')
    plt.show()

    # Plot 4: VR/AR Experience Distribution (Pie Chart)
    vr_ar_counts = Counter(vr_ar)
    fig, ax = plt.subplots(figsize=(8, 8))
    ax.pie(vr_ar_counts.values(), labels=vr_ar_counts.keys(), autopct='%1.1f%%', startangle=140, colors=['#8f45a3ff', '#0073c0ff'])
    # ax.set_title('VR/AR Experience Distribution')
    plt.savefig('misc/graphs/vr-ar-distribution.pdf', transparent=True, bbox_inches='tight')
    plt.show()
    
def graph_preferences(data_list):
    
    print(len(data_list))
    
    plt.rcParams.update({
        "font.family": "serif",
        "axes.labelsize": 15,
        "font.size": 12,
        "legend.fontsize": 10,
        "xtick.labelsize": 12,
        "ytick.labelsize": 12,
        "figure.figsize": (13, 6)
    })

    # Extract and count preferences
    preference_list = []
    for entry in data_list:
        preference_str = entry.get('Rank the conditions in order of prefrence', None)
        if isinstance(preference_str, str):
            preference_list.append(preference_str.split(';')[:-1])

    all_conditions = [cond.strip() for prefs in preference_list for cond in prefs]
    unique_conditions = list(set(all_conditions))

    condition_order_counts = {cond: [0] * len(preference_list[0]) for cond in unique_conditions}

    for prefs in preference_list:
        for rank, condition in enumerate(prefs):
            condition_order_counts[condition.strip()][rank] += 1

    
    # Create DataFrame
    df = pd.DataFrame(condition_order_counts).transpose()
    df.columns = [f'{i+1}{suffix}' for i, suffix in enumerate(['st', 'nd', 'rd'] + ['th'] * (len(df.columns) - 3))]
    df['Total'] = df.sum(axis=1)
    
    # Normalize counts by total responses
    df_normalized = df.div(df['Total'], axis=0).drop(columns=['Total'])
    
    # Sort by percentage of first preference
    df_normalized = df_normalized.sort_values(by='1st', ascending=True)

    # Ensure enough colors are available
    n_ranks = len(df_normalized.columns)
    rank_colors = plt.cm.get_cmap('tab20c', n_ranks).colors  # Get 'n_ranks' colors from 'Set3' colormap

    # Plotting
    fig, ax = plt.subplots()
    bar_width = 0.85

    y_positions = np.arange(len(df_normalized.index))
    for i, rank in enumerate(df_normalized.columns):
        ax.barh(y_positions, df_normalized[rank], left=df_normalized[df_normalized.columns[:i]].sum(axis=1), 
                height=bar_width, color=rank_colors[i], label=rank, edgecolor='grey')

    ax.set_yticks(y_positions)
    ax.set_yticklabels([condition.split(' Condition')[0] for condition in df_normalized.index])
    ax.set_xlabel('Normalized Preference Count')
    ax.legend(title='Rank', loc='lower right')

    # Adding text labels for each segment
    for i, rank in enumerate(df_normalized.columns):
        for j, condition in enumerate(df_normalized.index):
            value = df_normalized.loc[condition, rank]
            if value > 0:
                ax.text(df_normalized[df_normalized.columns[:i]].sum(axis=1).iloc[j] + value / 2, j, 
                        f'{value * 100:.1f}%', va='center', ha='center', color='black', fontsize=10)

    plt.grid(axis='x')
    plt.tight_layout()
    
    # Save to PDF with transparent background and tight bounding box
    plt.savefig('misc/graphs/preferences.pdf', transparent=True, bbox_inches='tight')

    # Show the plot
    plt.show()
    



#overalll data sample example
# {'Id': 3, 'Start time': Timestamp('2024-06-01 14:19:17'), 'Completion time': Timestamp('2024-06-01 14:21:39'), 'Email': 'anonymous', 'Name': nan, 'First Name': 'Charlie', 'Last\xa0Name': 'Lidbury', 'Gender': 'Man', 'Do you wear glasses': 'No', 'Have you used VR/AR before?': 'Yes', 'Age': 22, 'Handedness (which is your dominant hand)': 'Right', 'StudyID (this will be generated by Robbie)': 'charlid', 'Email (if you would like to be updated about this study)': 'charlie.lidbury@icloud.com', 'I completed the demo': 'Twice', 'I found completing the tasks for this condition difficult': 2, 'I found this condition frustrating': 4, 'It was unclear in which direction I needed to move my hand next': 1, 'I found completing the tasks for this condition difficult1': 4, 'I found this condition frustrating1': 6, 'It was unclear in which direction I needed to move my hand next1': 2, 'I found completing the tasks for this condition difficult2': 3, 'I found this condition frustrating2': 5, 'It was unclear in which direction I needed to move my hand next2': 6, 'I found completing the tasks for this condition difficult3': 4, 'I found this condition frustrating3': 6, 'It was unclear in which direction I needed to move my hand next3': 3, 'The task was able to track my hand accurately': 8, 'The task was able to track my hand reliably\xa0': 2, 'The task was able to track my eye\xa0accurately\n\n': 8, 'The task was able to track my eye reliably\xa0\n\n': 8, 'Rank the conditions in order of prefrence': 'Tracker Condition\xa0(View changes)\xa0;Tracker Offset Condition\xa0(View changes, offset)\xa0;Static Condition\xa0(View does not change)\xa0;Static Offset Condition\xa0(View does not change, offset)\xa0;'}
def graph_reliable_accurate(data_list):
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

    # Extract relevant data
    hand_accuracy = [entry['The task was able to track my hand accurately'] for entry in data_list]
    hand_reliability = [entry['The task was able to track my hand reliably\xa0'] for entry in data_list]
    eye_accuracy = [entry['The task was able to track my eye\xa0accurately\n\n'] for entry in data_list]
    eye_reliability = [entry['The task was able to track my eye reliably\xa0\n\n'] for entry in data_list]

    # Calculate means
    hand_accuracy_mean = np.mean(hand_accuracy)
    hand_reliability_mean = np.mean(hand_reliability)
    eye_accuracy_mean = np.mean(eye_accuracy)
    eye_reliability_mean = np.mean(eye_reliability)

    # Set up bar chart
    labels = ['Hand Tracking', 'Eye Tracking']
    accuracy_means = [hand_accuracy_mean, eye_accuracy_mean]
    reliability_means = [hand_reliability_mean, eye_reliability_mean]

    x = np.arange(len(labels))  # the label locations
    width = 0.35  # the width of the bars

    fig, ax = plt.subplots(figsize=(13, 8))
    rects1 = ax.bar(x - width/2, accuracy_means, width, label='Accuracy', color='#0073c0ff')
    rects2 = ax.bar(x + width/2, reliability_means, width, label='Reliability', color='#ff7c37ff')

    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('Mean Score')
    ax.set_xlabel('Tracking Type')
    # ax.set_title('Mean Accuracy and Reliability of Hand and Eye Tracking')
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.legend()

    # Attach a text label above each bar in rects, displaying its height
    def autolabel(rects):
        for rect in rects:
            height = rect.get_height()
            ax.annotate(f'{height:.2f}',
                        xy=(rect.get_x() + rect.get_width() / 2, height),
                        xytext=(0, 3),  # 3 points vertical offset
                        textcoords="offset points",
                        ha='center', va='bottom')

    autolabel(rects1)
    autolabel(rects2)

    fig.tight_layout()

    # Save to PDF with transparent background and tight bounding box
    plt.savefig('misc/graphs/reliable-accurate.pdf', transparent=True, bbox_inches='tight')

    plt.show()

def graph_eye_movement(data):
    
    # Prepare figure with LaTeX style
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

    tracker_data_combined = []
    tracker_offset_data_combined = []

    for user in data.keys():
        for _, conditions in data[user].items():
            tracker_data_combined.extend(conditions['TRACKER'])
            tracker_offset_data_combined.extend(conditions['TRACKER_OFFSET'])

    # Perform t-test
    t_stat, p_value = ttest_ind(tracker_data_combined, tracker_offset_data_combined)
    print(f"T-statistic: {t_stat:.4f}, P-value: {p_value:.4f}")

    # Calculate combined statistics
    if tracker_data_combined:
        tracker_mean = np.mean(tracker_data_combined)
        tracker_std = np.std(tracker_data_combined)
    else:
        tracker_mean = 0
        tracker_std = 0

    if tracker_offset_data_combined:
        tracker_offset_mean = np.mean(tracker_offset_data_combined)
        tracker_offset_std = np.std(tracker_offset_data_combined)
    else:
        tracker_offset_mean = 0
        tracker_offset_std = 0

    means = [tracker_mean, tracker_offset_mean]
    stds = [tracker_std, tracker_offset_std]
    labels = ['TRACKER', 'TRACKER_OFFSET']
    colors = ['#0073c0ff', '#ff7c37ff']

    fig, ax = plt.subplots()

    rects = ax.bar(labels, means, yerr=stds, capsize=5, color=colors)

    ax.set_ylabel('Mean Values')
    ax.set_title('Combined Mean Eye Movement Values and Standard Deviations')

    def autolabel(rects):
        for rect in rects:
            height = rect.get_height()
            ax.annotate(f'{height:.4f}',
				xy=(rect.get_x() + rect.get_width() / 2, height),
				xytext=(0, 3),
				textcoords="offset points",
				ha='center', va='bottom')

    autolabel(rects)

    # # Display the T-Test results on the plot
    # ax.text(0.5, 0.05, f'T-test: t={t_stat:.4f}, p={p_value:.4f}',
    #         transform=ax.transAxes, ha='center', va='center', fontsize=12, color='red')

    fig.tight_layout()

    plt.savefig('misc/graphs/combined-eye-movement.pdf', transparent=True, bbox_inches='tight')
    plt.show()