import pyvista as pv
import numpy as np
import os
import matplotlib.pyplot as plt
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
    
    
def generate_color_gradient(start_color, end_color, num_colors):
    """
    Generate a list of colors forming a gradient from start_color to end_color.
    """
    gradient = np.linspace(0, 1, num_colors)
    color_list = []
    for g in gradient:
        color = (1 - g) * np.array(start_color) + g * np.array(end_color)
        color_list.append(color)
    return color_list

def plot_finger_trace(finger_points, challenge):
    # Create a PyVista plotter with off_screen=True
    plotter = pv.Plotter(off_screen=True)
    
    # Generate a gradient from green to red
    start_color = (0, 1, 0)  # Green in RGB
    end_color = (1, 0, 0)    # Red in RGB
    num_segments = sum(len(person) for person in finger_points)
    colors = generate_color_gradient(start_color, end_color, num_segments)

    # Calculate volsim bounds
    width = 15.0
    height = 30.0
    centre = np.array([0, 15, 0])
    min_x, max_x = centre[0] - width/2, centre[0] + width/2
    min_y, max_y = centre[1] - height/2, centre[1] + height/2
    min_z, max_z = centre[2], centre[2] + width

    def is_point_inside_screen(point):
        x, y, z = point
        return min_x <= x <= max_x and min_y <= y <= max_y and min_z <= z <= max_z

    color_index = 0
    for person in finger_points:
        for segment in person:
            filtered_points = [point for (_, point) in segment if is_point_inside_screen(point)]
            if filtered_points:
                finger_point_cloud = pv.PolyData(filtered_points)
                color = colors[color_index]
                plotter.add_points(finger_point_cloud, color=color, point_size=5)
                color_index += 1

    # Assuming plot_screen, plot_virtual_sim, and plot_task add necessary elements to the plotter
    plot_screen(plotter)
    plot_virtual_sim(plotter)
    plot_task(plotter, challenge)
    
    # Set the background to white (opaque, as transparency is handled during saving)
    plotter.set_background(color="white")

    # Save the screenshot with a transparent background
    plotter.screenshot("misc/finger-trace-plot.png", transparent_background=True)

    # Close the plotter to release resources
    plotter.close()

    plotter = pv.Plotter()
    plot_virtual_sim(plotter)
    plot_task(plotter, challenge, demo=True)
    plot_screen(plotter)

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
        plotter.add_lines(edge, color='black', width=2)


    
def plot_task(plotter, challenge, demo=False):
    # Convert task positions to numpy array for easy manipulation
    task_positions = np.array(utility.get_task_positions(challenge,demo))

    # Add task positions to the plotter
    plotter.add_points(task_positions, color="purple", point_size=8)

    # Draw lines between consecutive task positions
    for i in range(len(task_positions) - 1):
        line = pv.Line(task_positions[i], task_positions[i + 1])
        plotter.add_mesh(line, color="purple", line_width=2)


def visualize_point_cloud_pyvista_print(file_path, z_min=0, z_max=500, distance_threshold=1000):
    data = np.loadtxt(file_path, delimiter=',', skiprows=1)

    # Create a mask that includes points close to the eyes or hands
    filtered_data = data
    
    # Normalize z-values for color mapping
    z_normalized = (filtered_data[:, 2] - z_min) / (z_max - z_min)
    z_normalized = np.clip(z_normalized, 0, 1)  # Ensure values are between 0 and 1
    
    point_cloud = pv.PolyData(filtered_data[:, :3])
    plotter_pre = pv.Plotter(off_screen=True)
    
    # Use the 'coolwarm' colormap, with colors based on normalized z-values
    plotter_pre.add_points(point_cloud, scalars=z_normalized, cmap="coolwarm", point_size=1.5, render_points_as_spheres=True, scalar_bar_args={'title': None, 'n_labels': 0, 'label_font_size': 0, 'color': None, 'position_x': -1, 'position_y': -1})
    
    # Hide the scalar bar if it still shows up
    plotter_pre.scalar_bar.SetVisibility(False)
    
    plot_screen(plotter_pre, (10,10,0))

    plotter_pre.view_xz()
    plotter_pre.camera.position = (0, 300, 100)
    
    # plotter.add_axes(interactive=False, line_width=5, labels_off=False)
    
    # plotter.show()
    
    screenshot_eye_filename = "misc/calibration_pre.png"
    plotter_pre.screenshot(screenshot_eye_filename)
    
    plotter_post = pv.Plotter(off_screen=True)
    
    # Use the 'coolwarm' colormap, with colors based on normalized z-values
    plotter_post.add_points(point_cloud, scalars=z_normalized, cmap="coolwarm", point_size=1.5, render_points_as_spheres=True, scalar_bar_args={'title': None, 'n_labels': 0, 'label_font_size': 0, 'color': None, 'position_x': -1, 'position_y': -1})
    
    # Hide the scalar bar if it still shows up
    plotter_post.scalar_bar.SetVisibility(False)
    
    plot_screen(plotter_post)

    plotter_post.view_xz()
    plotter_post.camera.position = (0, 300, 100)
    
    # plotter.add_axes(interactive=False, line_width=5, labels_off=False)
    
    # plotter.show()
    
    screenshot_eye_filename = "misc/calibration_post.png"
    plotter_post.screenshot(screenshot_eye_filename)
    
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
    
def get_screen_points(offset=(0,0,0)):
    screen_width = 34.0
    screen_height = 52.0
    screen_points = np.array([
        [-screen_width/2, 0, 0],
        [screen_width/2, 0, 0],
        [-screen_width/2, screen_height, 0],
        [screen_width/2, screen_height, 0]
    ]) + offset
    return screen_points

def plot_screen(plotter,offset=(0,0,0)):
    # Define the four points
 

    screen_points = get_screen_points(offset)
    # Add the points to the plotter
    # plotter.add_points(screen_points, color='green', point_size=10, scalar_bar_args={'title': None, 'n_labels': 0, 'label_font_size': 0, 'color': None, 'position_x': -1, 'position_y': -1})

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