import re
from collections import defaultdict

# Function to parse the MTL file
def parse_mtl_file(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()

    materials = []
    current_material = []
    for line in lines:
        line = line.strip()
        if line.startswith('newmtl'):
            if current_material:
                materials.append('\n'.join(current_material))
            current_material = [line]
        else:
            if current_material:
                current_material.append(line)
    
    if current_material:
        materials.append('\n'.join(current_material))
    
    return materials

# Function to remove the material name and normalize the material data
def normalize_material(material):
    lines = material.split('\n')
    material_name = lines[0]
    properties = '\n'.join(sorted(lines[1:]))  # Sort to ensure order doesn't affect uniqueness
    return material_name, properties

# Function to count unique materials and keep track of their original names
def count_unique_materials(materials):
    material_dict = defaultdict(list)
    for material in materials:
        material_name, normalized_material = normalize_material(material)
        material_dict[normalized_material].append(material_name)
    return material_dict

# Function to write the new MTL file with unique materials
def write_new_mtl_file(material_dict, new_mtl_path):
    with open(new_mtl_path, 'w') as file:
        for i, (properties, names) in enumerate(material_dict.items(), start=1):
            new_material_name = f"Material_{i}"
            file.write(f"newmtl {new_material_name}\n{properties}\n\n")
    return {name: f"Material_{i+1}" for i, names in enumerate(material_dict.values()) for name in names}

# Function to update the OBJ file with new material names
def update_obj_file(obj_path, material_mapping, new_obj_path):
    with open(obj_path, 'r') as file:
        lines = file.readlines()
    
    with open(new_obj_path, 'w') as file:
        for line in lines:
            if line.startswith('usemtl'):
                original_material_name = line.strip().split()[1]
                new_material_name = material_mapping.get(original_material_name, original_material_name)
                file.write(f"usemtl {new_material_name}\n")
            else:
                file.write(line)

mtl_path = '/home/robbieb/Projects/VolumetricSim/volsim/data/resources/models/8qbk_old.obj'  # Replace with your .mtl file path
obj_path = '/home/robbieb/Projects/VolumetricSim/volsim/data/resources/materials/8qbk_old.mtl'  # Replace with your .obj file path
new_mtl_path = '/home/robbieb/Projects/VolumetricSim/volsim/data/resources/models/8qbk.obj'
new_obj_path = '/home/robbieb/Projects/VolumetricSim/volsim/data/resources/materials/8qbk.mtl'

materials = parse_mtl_file(mtl_path)
material_dict = count_unique_materials(materials)
material_mapping = write_new_mtl_file(material_dict, new_mtl_path)
update_obj_file(obj_path, material_mapping, new_obj_path)

print("Material compression completed.")
print(f"New MTL file: {new_mtl_path}")
print(f"New OBJ file: {new_obj_path}")

# obj_file_path = '/home/robbieb/Projects/VolumetricSim/volsim/data/resources/models/8qbk.obj'
# mtl_file_path = '/home/robbieb/Projects/VolumetricSim/volsim/data/resources/materials/8qbk.mtl'

