import pyvista as pv
import numpy as np
import os

import utility

def plot_trace(eye_points, index_finger_points, middle_finger_points, challenge):
    # Create a PyVista plotter
    plotter = pv.Plotter()
    
    # Add points to the plotter
    eye_point_cloud = pv.PolyData(eye_points)
    index_finger_point_cloud = pv.PolyData(index_finger_points)
    middle_finger_point_cloud  = pv.PolyData(middle_finger_points)
    
    plotter.add_points(eye_point_cloud, color="red", point_size=5)
    plotter.add_points(index_finger_point_cloud, color="blue", point_size=5)
    plotter.add_points(middle_finger_point_cloud, color="green", point_size=5)

    plot_screen(plotter)
    plot_virtual_sim(plotter)
    plot_task(plotter, challenge)
    
    # Display the plot
    plotter.show()
    
def plot_demo_trace(challenge):
    plotter = pv.Plotter()
    plot_virtual_sim(plotter)
    plot_task(plotter, challenge, demo=True)
    plot_screen(plotter)
    plotter.show()
    

def plot_virtual_sim(plotter):
    width = 10.0
    height = 20.0
    
    centre = np.array([0, 15, 0])
    
    sim_points = np.array([
        centre + np.array([-width/2,  height/2, 0]),
        centre + np.array([width/2,  height/2, 0]),
        centre + np.array([-width/2, -height/2, 0]),
        centre + np.array([width/2, -height/2, 0]),
        centre + np.array([-width/2,  height/2, width]),
        centre + np.array([width/2,  height/2, width]),
        centre + np.array([-width/2, -height/2, width]),
        centre + np.array([width/2, -height/2, width])
    ])
    
    # Define the rectangle edges
    sim_edges = np.array([
        [sim_points[0], sim_points[1]],
        [sim_points[1], sim_points[3]],
        [sim_points[3], sim_points[2]],
        [sim_points[2], sim_points[0]],
        [sim_points[0], sim_points[4]],
        [sim_points[1], sim_points[5]],
        [sim_points[2], sim_points[6]],
        [sim_points[3], sim_points[7]],
        [sim_points[4], sim_points[5]],
        [sim_points[5], sim_points[7]],
        [sim_points[7], sim_points[6]],
        [sim_points[6], sim_points[4]]
    ])
    
    # Add the rectangle edges to the plotter
    for edge in sim_edges:
        plotter.add_lines(edge, color='black', width=5)

# Usage example (assuming plotter is defined elsewhere)
# plot_virtual_sim(plotter)

    
def plot_task(plotter, challenge, demo=False):
    # Convert task positions to numpy array for easy manipulation
    task_positions = np.array(get_task_positions(challenge,demo))

    # Add task positions to the plotter
    plotter.add_points(task_positions, color="purple", point_size=8)

    # Draw lines between consecutive task positions
    for i in range(len(task_positions) - 1):
        line = pv.Line(task_positions[i], task_positions[i + 1])
        plotter.add_mesh(line, color="purple", line_width=2)

def get_task_positions(challenge,demo=False):
    directions = utility.get_mapped_directions(challenge,demo)
    start = np.array([-3.0, 7.0, 2.0])
    
    # Compute the cumulative points
    cumulative_points = [start]
    current_position = start
    for direction in directions:
        current_position = current_position + direction
        cumulative_points.append(current_position)

    return cumulative_points


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
    
    plot_screen(plotter)

    plotter.view_xz()
    plotter.camera.position = (1, 1, 0)
    
    plotter.add_axes(interactive=False, line_width=5, labels_off=False)

    plotter.show()
    

def plot_screen(plotter):
    # Define the four points
    screen_width = 34.0
    screen_height = 52.0
    screen_points = np.array([
        [-screen_width/2, 0, 0],
        [screen_width/2, 0, 0],
        [-screen_width/2, screen_height, 0],
        [screen_width/2, screen_height, 0]
    ])
    
    # Add the points to the plotter
    plotter.add_points(screen_points, color='green', point_size=10)

    # Define the rectangle edges
    screen_edges = np.array([
        [screen_points[0], screen_points[1]],
        [screen_points[1], screen_points[3]],
        [screen_points[3], screen_points[2]],
        [screen_points[2], screen_points[0]]
    ])
    
    # Add the rectangle edges to the plotter
    for edge in screen_edges:
        plotter.add_lines(edge, color='yellow', width=5)
    