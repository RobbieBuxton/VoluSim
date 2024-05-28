import pandas as pd
import matplotlib.pyplot as plt
from collections import Counter

def graph_latency_overall(challenge_times):
    
    plt.rcParams.update({
    "text.usetex": True,
    "font.family": "serif",
    "font.serif": ["Computer Modern Roman"],
    "axes.labelsize": 20,
    "font.size": 25,
    "legend.fontsize": 10,
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


    # # Calculate the total number of latencies
    # total_latencies = len(df_latencies)    
    # # Calculate the number of latencies between 30-35ms
    # latencies_30_35 = df_latencies[(df_latencies['Latency'] >= 30) & (df_latencies['Latency'] <= 35)]
    # percentage_30_35 = len(latencies_30_35) / total_latencies * 100    
    # # Calculate the number of latencies between 16-18ms
    # latencies_16_18 = df_latencies[(df_latencies['Latency'] >= 16) & (df_latencies['Latency'] <= 18)]
    # percentage_16_18 = len(latencies_16_18) / total_latencies * 100    
    # # Calculate the number of latencies between 49-51ms
    # latencies_49_51 = df_latencies[(df_latencies['Latency'] >= 49) & (df_latencies['Latency'] <= 51)]
    # percentage_49_51 = len(latencies_49_51) / total_latencies * 100    
    # print(f"Percentage of latencies between 30-35ms: {percentage_30_35:.2f}%")
    # print(f"Percentage of latencies between 16-18ms: {percentage_16_18:.2f}%")
    # print(f"Percentage of latencies between 49-51ms: {percentage_49_51:.2f}%")

    # Filter latencies where the frequency is less than 5
    filtered_latencies = [latency for latency, freq in latency_freq if freq >= 5]
    
    # Create a filtered DataFrame
    df_filtered_latencies = df_latencies[df_latencies['Latency'].isin(filtered_latencies)]

    # Plotting the histogram
    plt.figure(figsize=(13, 6))
    plt.hist(df_filtered_latencies['Latency'], bins=68, edgecolor='black', color='#0073c0ff')
    plt.title('Tracker Latency Distribution (5 mins)')
    plt.xlabel('Latency (ms)')
    plt.ylabel('Frequency ')

    # Set x-axis ticks for every integer value
    plt.xticks(range(int(df_filtered_latencies['Latency'].min()), int(df_filtered_latencies['Latency'].max()) + 1))

    # Save to PNG with transparent background and tight bounding box
    plt.savefig('misc/overall-latency.pdf', transparent=True, bbox_inches='tight')

    # Show the plot
    plt.show()
    
def graph_latency_pydown(challenge_times_dict):
    
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
    plt.legend(by_label.values(), by_label.keys(), title="Resolution")

    plt.title('Tracker Latency Distribution (1 min)')
    plt.xlabel('Latency (ms)')
    plt.ylabel('Frequency')
    
    # Save to PNG with transparent background and tight bounding box
    plt.savefig('misc/pydown.pdf', transparent=True, bbox_inches='tight')

    # Show the plot
    plt.show()