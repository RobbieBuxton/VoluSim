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
    
    # Plot with eye position
    plotter_eye = pv.Plotter(off_screen=True)
    plotter_eye.add_points(point_cloud, color=(0, 115, 192), point_size=1.5)
    eye_cloud = pv.PolyData(left_eye_pos.reshape(1, -1))  # Reshape needed if left_eye_pos is a single point
    eye_glyph = eye_cloud.glyph(scale=False, geom=pv.Sphere(radius=1.0))
    plotter_eye.add_mesh(eye_glyph, color=(255, 124, 55))
    
    plotter_eye.view_isometric()
    plotter_eye.view_xy()
    scale = 75
    plotter_eye.camera.position = (-scale, scale, -scale)
    plotter_eye.window_size = (1920, 1080)
    plotter_eye.background_color = None  # None sets it to transparent
    
    screenshot_eye_filename = "misc/output_eye.png"
    plotter_eye.screenshot(screenshot_eye_filename)
    plotter_eye.close()
    
    # Plot with hand positions
    plotter_hand = pv.Plotter(off_screen=True)
    plotter_hand.add_points(point_cloud, color=(0, 115, 192), point_size=1.5)
    hand_cloud = pv.PolyData(hand_positions)
    hand_glyph = hand_cloud.glyph(scale=False, geom=pv.Sphere(radius=1.0))
    plotter_hand.add_mesh(hand_glyph, color=(143, 69, 163))
    
    plotter_hand.view_isometric()
    plotter_hand.view_xy()
    plotter_hand.camera.position = (-scale, scale, -scale)
    plotter_hand.window_size = (1920, 1080)
    plotter_hand.background_color = None  # None sets it to transparent
    
    screenshot_hand_filename = "misc/output_hand.png"
    plotter_hand.screenshot(screenshot_hand_filename)
    plotter_hand.close()

# Usage example
visualize_point_cloud_pyvista("misc/pointCloud.csv", "misc/leftEyePos.csv", "misc/hand.csv")

