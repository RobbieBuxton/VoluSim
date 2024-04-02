import pyvista as pv
import numpy as np

def load_eye_position(file_path):
    # Assuming the eye position CSV contains a single row with x,y,z coordinates
    return np.loadtxt(file_path, delimiter=',', skiprows=0) 

def visualize_point_cloud_pyvista(file_path, left_eye_path, right_eye_path, z_min=0, z_max=500, distance_threshold=100):
    # Load the point cloud data
    data = np.loadtxt(file_path, delimiter=',', skiprows=1)
    
    # Load eye positions
    left_eye_pos = load_eye_position(left_eye_path)
    right_eye_pos = load_eye_position(right_eye_path)
    
    # Apply z-range filtering

    # Calculate distances to each eye and filter
    distances_to_left_eye = np.linalg.norm(data[:, :3] - left_eye_pos, axis=1)
    distances_to_right_eye = np.linalg.norm(data[:, :3] - right_eye_pos, axis=1)
    
    distance_mask = (distances_to_left_eye <= distance_threshold) | (distances_to_right_eye <= distance_threshold)
    filtered_data = data[distance_mask]
    
    # Create a PyVista point cloud object
    point_cloud = pv.PolyData(filtered_data[:, :3])
    
    # Create a plotter and add the main point cloud
    plotter = pv.Plotter()
    plotter.add_points(point_cloud, cmap="viridis", point_size=1.5)
    
    # Add the eye positions in red
    eye_positions = np.vstack([left_eye_pos, right_eye_pos])
    eye_cloud = pv.PolyData(eye_positions)
    plotter.add_points(eye_cloud, color="red", point_size=5)
    
    # Adjust camera settings here to face the front of the point cloud
    # This example assumes the front is along the negative Z-axis and above the point cloud
    plotter.view_xy()
    plotter.camera.position = (0, 0, -1)  # Position the camera in front of the point cloud
    plotter.camera.roll = 180  # Adjust if necessary to orient the view correctly
    
    plotter.add_axes(interactive=False, line_width=2, labels_off=False)

    plotter.show()

# Usage example
visualize_point_cloud_pyvista("misc/pointCloud.csv", "misc/leftEyePos.csv", "misc/rightEyePos.csv")
