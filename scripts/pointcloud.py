import pyvista as pv
import numpy as np

def visualize_point_cloud_pyvista(file_path, left_eye_path, hand_path, z_min=0, z_max=500, distance_threshold=50):
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
    plotter = pv.Plotter(off_screen=True)  # Enable off-screen rendering
    
    # Use the 'coolwarm' colormap, with colors based on normalized z-values
    plotter.add_points(point_cloud, color=(0,115,192), point_size=1.)
    
    # Add left eye position to the plot using spheres
    eye_cloud = pv.PolyData(left_eye_pos.reshape(1, -1))  # Reshape needed if left_eye_pos is a single point
    eye_glyph = eye_cloud.glyph(scale=False, geom=pv.Sphere(radius=1.0))
    plotter.add_mesh(eye_glyph, color=(255, 124, 55))
    
    # Add hand positions to the plot using spheres
    hand_cloud = pv.PolyData(hand_positions)
    hand_glyph = hand_cloud.glyph(scale=False, geom=pv.Sphere(radius=1.0))
    plotter.add_mesh(hand_glyph, color=(143,69,163))
    
    plotter.view_isometric()
    plotter.view_xy()
    scale = 100
    plotter.camera.position = (-scale, scale, -scale)
    
    plotter.window_size = (1920, 1080)
    
    # Set transparent background
    plotter.background_color = None  # None sets it to transparent

    # Save screenshot with transparent background
    screenshot_filename = "misc/output.png"
    plotter.screenshot(screenshot_filename)
    plotter.close()  # Explicitly close the plotter

# Usage example
visualize_point_cloud_pyvista("misc/pointCloud.csv", "misc/leftEyePos.csv", "misc/hand.csv")
