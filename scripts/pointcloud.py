import pyvista as pv
import numpy as np

def visualize_point_cloud_pyvista(file_path, left_eye_path, hand_path, z_min=0, z_max=500, distance_threshold=1000):
    data = np.loadtxt(file_path, delimiter=',', skiprows=1)
    left_eye_pos = np.loadtxt(left_eye_path, delimiter=',', skiprows=0)
    hand_positions = np.loadtxt(hand_path, delimiter=',', skiprows=1)
    
    distances_to_left_eye = np.linalg.norm(data[:, :3] - left_eye_pos, axis=1)
    
    # Create a mask that includes points close to the eyes or hands
    distance_mask = (distances_to_left_eye <= distance_threshold)
    filtered_data = data[distance_mask]
    
    # Normalize z-values for color mapping
    z_normalized = (filtered_data[:, 2] - z_min) / (z_max - z_min)
    z_normalized = np.clip(z_normalized, 0, 1)  # Ensure values are between 0 and 1
    
    point_cloud = pv.PolyData(filtered_data[:, :3])
    plotter = pv.Plotter()
    
    # Use the 'coolwarm' colormap, with colors based on normalized z-values
    plotter.add_points(point_cloud, scalars=z_normalized, cmap="coolwarm", point_size=1.5)
    
    # Add left eye position to the plot
    eye_cloud = pv.PolyData(left_eye_pos.reshape(1, -1))  # Reshape needed if left_eye_pos is a single point
    plotter.add_points(eye_cloud, color="orange", point_size=5)

    # Add hand positions to the plot
    hand_cloud = pv.PolyData(hand_positions)
    plotter.add_points(hand_cloud, color="purple", point_size=5)
    
    plotter.view_xy()
    plotter.camera.position = (0, 0, -1)
    
    plotter.add_axes(interactive=False, line_width=5, labels_off=False)

    plotter.show()

# Usage example
visualize_point_cloud_pyvista("misc/pointCloud.csv", "misc/leftEyePos.csv", "misc/hand.csv")
