import pandas as pd
import matplotlib.pyplot as plt


def graph_latency(times):
    # times looks like [[time, x, y, z], [time, x, y, z], ...]

    # Calculate time differences (latencies) and differences in x, y, z
    latencies = []
    for t1, t2 in zip(times[:-1], times[1:]):
        latency = t2[0] - t1[0]
        latencies.append(latency)
    
    # Create a DataFrame for latencies
    df_latencies = pd.DataFrame(latencies, columns=['Latency'])

    # Plotting the histogram
    plt.figure(figsize=(10, 6))
    plt.hist(df_latencies['Latency'], bins=100, edgecolor='black')
    plt.title('Latency Distribution')
    plt.xlabel('Latency (ms)')
    plt.ylabel('Frequency')

    # Save to PDF (uncomment if needed)
    # plt.savefig('/mnt/data/latency_distribution.pdf')

    # Show the plot
    plt.show()