import pyvista as pv
import numpy as np

def plot_trace(eye_points,index_finger_points,middle_finger_points):
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
    
    # Display the plot
    plotter.show()


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
    