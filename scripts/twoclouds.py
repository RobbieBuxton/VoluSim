import pyvista as pv
import numpy as np

def load_eye_position(file_path):
    # Assuming the eye position CSV contains a single row with x,y,z coordinates
    return np.loadtxt(file_path, delimiter=',', skiprows=0)

def load_and_filter_point_cloud(file_path, left_eye_path, right_eye_path, distance_threshold=100):
    # Load the point cloud data
    data = np.loadtxt(file_path, delimiter=',', skiprows=1)
    
    # Load eye positions
    left_eye_pos = load_eye_position(left_eye_path)
    right_eye_pos = load_eye_position(right_eye_path)
    
    # Calculate distances to each eye and filter
    distances_to_left_eye = np.linalg.norm(data[:, :3] - left_eye_pos, axis=1)
    distances_to_right_eye = np.linalg.norm(data[:, :3] - right_eye_pos, axis=1)
    
    distance_mask = (distances_to_left_eye <= distance_threshold) | (distances_to_right_eye <= distance_threshold)
    filtered_data = data[distance_mask]
    
    return filtered_data

def visualize_point_cloud_pyvista(file_path, second_file_path, left_eye_path, right_eye_path, z_min=0, z_max=500, distance_threshold=100):
    # Load and filter the first point cloud
    filtered_data_1 = load_and_filter_point_cloud(file_path, left_eye_path, right_eye_path, distance_threshold)
    
    # Load and filter the second point cloud
    filtered_data_2 = load_and_filter_point_cloud(second_file_path, left_eye_path, right_eye_path, distance_threshold)
    
    # Create PyVista point cloud objects
    point_cloud_1 = pv.PolyData(filtered_data_1[:, :3])
    point_cloud_2 = pv.PolyData(filtered_data_2[:, :3])
    
    # Create a plotter
    plotter = pv.Plotter()
    
    # Add the main point cloud in the default color (viridis)
    plotter.add_points(point_cloud_1, cmap="viridis", point_size=1.5)
    
    # Add the second point cloud in green
    plotter.add_points(point_cloud_2, color="green", point_size=1.5)
    
    # Add the eye positions in red
    left_eye_pos = load_eye_position(left_eye_path)
    right_eye_pos = load_eye_position(right_eye_path)
    eye_positions = np.vstack([left_eye_pos, right_eye_pos])
    eye_cloud = pv.PolyData(eye_positions)
    plotter.add_points(eye_cloud, color="red", point_size=5)
    
    # Adjust camera settings here
    plotter.view_xy()
    plotter.camera.position = (0, 0, -1)  # Position the camera in front of the point clouds
    plotter.camera.roll = 180  # Adjust if necessary
    
    plotter.show()

# Usage example
visualize_point_cloud_pyvista("misc/pointCloud.csv", "misc/oldPointCloud.csv", "misc/leftEyePos.csv", "misc/rightEyePos.csv")
