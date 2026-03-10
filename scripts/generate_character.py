#!/usr/bin/env python3
"""
Generate a simple rigged humanoid character model in glTF format.

Creates:
- Simple humanoid mesh (head, torso, arms, legs)
- Skeleton hierarchy (root, spine, arms, legs)
- Basic animations (idle, walk, attack)

Output: character.glb (binary glTF)
"""

import json
import struct
import math
import os

# ============================================================================
# glTF Helper Classes
# ============================================================================

class Vec3:
    def __init__(self, x=0, y=0, z=0):
        self.x = float(x)
        self.y = float(y)
        self.z = float(z)
    
    def __add__(self, other):
        return Vec3(self.x + other.x, self.y + other.y, self.z + other.z)
    
    def __mul__(self, scalar):
        return Vec3(self.x * scalar, self.y * scalar, self.z * scalar)
    
    def to_list(self):
        return [self.x, self.y, self.z]

class Quat:
    def __init__(self, x=0, y=0, z=0, w=1):
        self.x = float(x)
        self.y = float(y)
        self.z = float(z)
        self.w = float(w)
    
    @staticmethod
    def from_euler(roll, pitch, yaw):
        """Create quaternion from Euler angles (radians)"""
        cy = math.cos(yaw * 0.5)
        sy = math.sin(yaw * 0.5)
        cp = math.cos(pitch * 0.5)
        sp = math.sin(pitch * 0.5)
        cr = math.cos(roll * 0.5)
        sr = math.sin(roll * 0.5)
        
        w = cr * cp * cy + sr * sp * sy
        x = sr * cp * cy - cr * sp * sy
        y = cr * sp * cy + sr * cp * sy
        z = cr * cp * sy - sr * sp * cy
        
        return Quat(x, y, z, w)
    
    def to_list(self):
        return [self.x, self.y, self.z, self.w]

class Mat4:
    @staticmethod
    def identity():
        return [
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        ]
    
    @staticmethod
    def translate(x, y, z):
        return [
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            x, y, z, 1
        ]
    
    @staticmethod
    def from_trs(translation, rotation, scale):
        """Create matrix from translation, rotation (quaternion), scale"""
        # Simplified: just do TRS without full quaternion rotation matrix
        # This is good enough for skeleton bind poses
        x, y, z = translation
        sx, sy, sz = scale
        
        return [
            sx, 0, 0, 0,
            0, sy, 0, 0,
            0, 0, sz, 0,
            x, y, z, 1
        ]

# ============================================================================
# Create Mesh Data
# ============================================================================

def create_humanoid_mesh():
    """Create a simple humanoid mesh with vertices and indices"""
    
    vertices = []
    indices = []
    
    # Head (top of body)
    head_bottom = len(vertices)
    vertices.extend([
        [-0.2, 0.8, -0.2], [0.2, 0.8, -0.2],
        [0.2, 0.8, 0.2],   [-0.2, 0.8, 0.2],
        [-0.2, 1.2, -0.2], [0.2, 1.2, -0.2],
        [0.2, 1.2, 0.2],   [-0.2, 1.2, 0.2],
    ])
    # Head faces
    for i in range(0, 8, 4):
        indices.extend([i+0, i+1, i+4, i+1, i+5, i+4])
        indices.extend([i+1, i+2, i+5, i+2, i+6, i+5])
        indices.extend([i+2, i+3, i+6, i+3, i+7, i+6])
        indices.extend([i+3, i+0, i+7, i+0, i+4, i+7])
    
    # Torso
    torso_bottom = len(vertices)
    vertices.extend([
        [-0.25, 0.2, -0.15], [0.25, 0.2, -0.15],
        [0.25, 0.2, 0.15],   [-0.25, 0.2, 0.15],
        [-0.25, 0.8, -0.15], [0.25, 0.8, -0.15],
        [0.25, 0.8, 0.15],   [-0.25, 0.8, 0.15],
    ])
    # Torso faces
    for i in range(torso_bottom, torso_bottom + 8, 4):
        idx = i - torso_bottom
        indices.extend([i+0, i+1, i+4, i+1, i+5, i+4])
        indices.extend([i+1, i+2, i+5, i+2, i+6, i+5])
        indices.extend([i+2, i+3, i+6, i+3, i+7, i+6])
        indices.extend([i+3, i+0, i+7, i+0, i+4, i+7])
    
    # Left arm
    l_arm_start = len(vertices)
    vertices.extend([
        [-0.25, 0.6, -0.1], [-0.6, 0.6, -0.1],
        [-0.6, 0.6, 0.1],   [-0.25, 0.6, 0.1],
        [-0.25, 0.2, -0.1], [-0.6, 0.2, -0.1],
        [-0.6, 0.2, 0.1],   [-0.25, 0.2, 0.1],
    ])
    for i in range(l_arm_start, l_arm_start + 8, 4):
        indices.extend([i+0, i+1, i+4, i+1, i+5, i+4])
        indices.extend([i+1, i+2, i+5, i+2, i+6, i+5])
        indices.extend([i+2, i+3, i+6, i+3, i+7, i+6])
        indices.extend([i+3, i+0, i+7, i+0, i+4, i+7])
    
    # Right arm (mirror of left)
    r_arm_start = len(vertices)
    vertices.extend([
        [0.25, 0.6, -0.1], [0.6, 0.6, -0.1],
        [0.6, 0.6, 0.1],   [0.25, 0.6, 0.1],
        [0.25, 0.2, -0.1], [0.6, 0.2, -0.1],
        [0.6, 0.2, 0.1],   [0.25, 0.2, 0.1],
    ])
    for i in range(r_arm_start, r_arm_start + 8, 4):
        indices.extend([i+0, i+1, i+4, i+1, i+5, i+4])
        indices.extend([i+1, i+2, i+5, i+2, i+6, i+5])
        indices.extend([i+2, i+3, i+6, i+3, i+7, i+6])
        indices.extend([i+3, i+0, i+7, i+0, i+4, i+7])
    
    # Left leg
    l_leg_start = len(vertices)
    vertices.extend([
        [-0.15, 0.2, -0.1], [-0.15, 0.2, 0.1],
        [-0.0, 0.2, 0.1],   [-0.0, 0.2, -0.1],
        [-0.15, -0.2, -0.1], [-0.15, -0.2, 0.1],
        [-0.0, -0.2, 0.1],   [-0.0, -0.2, -0.1],
    ])
    for i in range(l_leg_start, l_leg_start + 8, 4):
        indices.extend([i+0, i+1, i+4, i+1, i+5, i+4])
        indices.extend([i+1, i+2, i+5, i+2, i+6, i+5])
        indices.extend([i+2, i+3, i+6, i+3, i+7, i+6])
        indices.extend([i+3, i+0, i+7, i+0, i+4, i+7])
    
    # Right leg (mirror of left)
    r_leg_start = len(vertices)
    vertices.extend([
        [0.15, 0.2, -0.1], [0.15, 0.2, 0.1],
        [0.0, 0.2, 0.1],   [0.0, 0.2, -0.1],
        [0.15, -0.2, -0.1], [0.15, -0.2, 0.1],
        [0.0, -0.2, 0.1],   [0.0, -0.2, -0.1],
    ])
    for i in range(r_leg_start, r_leg_start + 8, 4):
        indices.extend([i+0, i+1, i+4, i+1, i+5, i+4])
        indices.extend([i+1, i+2, i+5, i+2, i+6, i+5])
        indices.extend([i+2, i+3, i+6, i+3, i+7, i+6])
        indices.extend([i+3, i+0, i+7, i+0, i+4, i+7])
    
    # Flatten and convert to bytes
    flat_vertices = []
    for v in vertices:
        flat_vertices.extend(v)
    
    return flat_vertices, indices

# ============================================================================
# Create Skeleton
# ============================================================================

def create_skeleton():
    """Create skeleton nodes with hierarchy"""
    nodes = []
    
    # Root node (base of character)
    nodes.append({
        "name": "Root",
        "translation": [0, 0, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": [1]  # Points to Spine
    })
    
    # Spine/Torso
    nodes.append({
        "name": "Spine",
        "translation": [0, 0.2, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": [2, 3, 4]  # Points to LeftArm, RightArm, LeftLeg, RightLeg
    })
    
    # Head
    nodes.append({
        "name": "Head",
        "translation": [0, 0.8, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": []
    })
    
    # Left arm
    nodes.append({
        "name": "LeftArm",
        "translation": [-0.4, 0.6, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": [5]
    })
    
    # Left forearm
    nodes.append({
        "name": "LeftForearm",
        "translation": [-0.35, 0, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": []
    })
    
    # Right arm
    nodes.append({
        "name": "RightArm",
        "translation": [0.4, 0.6, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": [6]
    })
    
    # Right forearm
    nodes.append({
        "name": "RightForearm",
        "translation": [0.35, 0, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": []
    })
    
    # Left leg
    nodes.append({
        "name": "LeftLeg",
        "translation": [-0.1, 0, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": [8]
    })
    
    # Left foot
    nodes.append({
        "name": "LeftFoot",
        "translation": [0, -0.4, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": []
    })
    
    # Right leg
    nodes.append({
        "name": "RightLeg",
        "translation": [0.1, 0, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": [9]
    })
    
    # Right foot
    nodes.append({
        "name": "RightFoot",
        "translation": [0, -0.4, 0],
        "rotation": [0, 0, 0, 1],
        "scale": [1, 1, 1],
        "children": []
    })
    
    # Return node list and skin data
    joints = list(range(len(nodes)))  # All nodes are joints
    
    return nodes, joints

# ============================================================================
# Create Animations
# ============================================================================

def create_animations():
    """Create animation tracks (idle, walk, attack)"""
    animations = []
    
    # IDLE animation (just subtle breathing)
    idle_anim = {
        "name": "Idle",
        "channels": [
            # Spine breathing
            {
                "sampler": 0,
                "target": {"node": 1, "path": "translation"}
            },
        ],
        "samplers": [
            {
                "input": 0,  # Times for spine translation
                "output": 1,  # Values for spine translation
                "interpolation": "LINEAR"
            }
        ]
    }
    
    # WALK animation (legs move, arms swing)
    walk_anim = {
        "name": "Walk",
        "channels": [
            # Left leg rotation
            {"sampler": 0, "target": {"node": 7, "path": "rotation"}},
            # Right leg rotation (opposite)
            {"sampler": 1, "target": {"node": 9, "path": "rotation"}},
            # Left arm swing
            {"sampler": 2, "target": {"node": 3, "path": "rotation"}},
            # Right arm swing (opposite)
            {"sampler": 3, "target": {"node": 5, "path": "rotation"}},
        ],
        "samplers": [
            {"input": 2, "output": 3, "interpolation": "LINEAR"},  # Left leg times/values
            {"input": 2, "output": 4, "interpolation": "LINEAR"},  # Right leg times/values
            {"input": 2, "output": 5, "interpolation": "LINEAR"},  # Left arm times/values
            {"input": 2, "output": 6, "interpolation": "LINEAR"},  # Right arm times/values
        ]
    }
    
    # ATTACK animation (raise arm and punch)
    attack_anim = {
        "name": "Attack",
        "channels": [
            # Right arm rotation for punch
            {"sampler": 0, "target": {"node": 5, "path": "rotation"}},
            # Right forearm rotation for punch extension
            {"sampler": 1, "target": {"node": 6, "path": "rotation"}},
        ],
        "samplers": [
            {"input": 4, "output": 7, "interpolation": "LINEAR"},
            {"input": 4, "output": 8, "interpolation": "LINEAR"},
        ]
    }
    
    return [idle_anim, walk_anim, attack_anim]

# ============================================================================
# Create Accessors and Buffer Data
# ============================================================================

def pack_float_array(values):
    """Pack float array to bytes"""
    return b''.join(struct.pack('<f', v) for v in values)

def create_buffer_views_and_accessors():
    """Create animation keyframe data"""
    buffer_data = bytearray()
    accessors = []
    buffer_views = []
    
    # Idle animation - spine translation keyframes
    idle_times = [0.0, 1.0, 2.0]
    idle_values = [
        0, 0.2, 0,      # Frame 0: no translation
        0, 0.22, 0,     # Frame 1: slight breathing
        0, 0.2, 0       # Frame 2: back to normal
    ]
    
    # Walk animation - keyframe times (same for all walk channels)
    walk_times = [0.0, 0.25, 0.5, 0.75, 1.0]
    
    # Left leg walk rotation (back and forth)
    left_leg_walk = [
        0, 0, 0, 1,           # Frame 0: forward
        -0.2, 0, 0, 0.98,     # Frame 1: 
        -0.5, 0, 0, 0.866,    # Frame 2: max back
        -0.2, 0, 0, 0.98,     # Frame 3:
        0, 0, 0, 1            # Frame 4: back to forward
    ]
    
    # Right leg walk rotation (opposite of left)
    right_leg_walk = [
        -0.5, 0, 0, 0.866,   # Frame 0: back
        -0.2, 0, 0, 0.98,    # Frame 1:
        0, 0, 0, 1,          # Frame 2: forward
        -0.2, 0, 0, 0.98,    # Frame 3:
        -0.5, 0, 0, 0.866    # Frame 4: back
    ]
    
    # Left arm walk swing
    left_arm_walk = [
        0.2, 0, 0, 0.98,     # Frame 0: forward swing
        0, 0, 0, 1,          # Frame 1: neutral
        -0.2, 0, 0, 0.98,    # Frame 2: back swing
        0, 0, 0, 1,          # Frame 3: neutral
        0.2, 0, 0, 0.98      # Frame 4: forward again
    ]
    
    # Right arm walk swing (opposite)
    right_arm_walk = [
        -0.2, 0, 0, 0.98,    # Frame 0: back
        0, 0, 0, 1,          # Frame 1: neutral
        0.2, 0, 0, 0.98,     # Frame 2: forward
        0, 0, 0, 1,          # Frame 3: neutral
        -0.2, 0, 0, 0.98     # Frame 4: back
    ]
    
    # Attack animation - arm punch keyframes
    attack_times = [0.0, 0.2, 0.4, 0.6]
    
    # Right arm punch rotation
    right_arm_punch = [
        0, 0, 0, 1,              # Frame 0: neutral
        0.707, 0, 0, 0.707,      # Frame 1: raise up 90 degrees
        0.707, 0, 0, 0.707,      # Frame 2: extended
        0, 0, 0, 1               # Frame 3: back to neutral
    ]
    
    # Right forearm punch extension
    right_forearm_punch = [
        0, 0, 0, 1,              # Frame 0: neutral
        0, 0, 0, 1,              # Frame 1: neutral
        0.707, 0, 0, 0.707,      # Frame 2: extend forward
        0, 0, 0, 1               # Frame 3: back to neutral
    ]
    
    # Pack all data into buffer
    data_list = [
        (idle_times, "time"),
        (idle_values, "vec3"),
        (walk_times, "time"),
        (left_leg_walk, "quat"),
        (right_leg_walk, "quat"),
        (left_arm_walk, "quat"),
        (right_arm_walk, "quat"),
        (attack_times, "time"),
        (right_arm_punch, "quat"),
        (right_forearm_punch, "quat"),
    ]
    
    buffer_offset = 0
    for idx, (data, dtype) in enumerate(data_list):
        packed = pack_float_array(data)
        buffer_data.extend(packed)
        
        count = len(data) if dtype == "time" else len(data) // (3 if dtype == "vec3" else 4)
        
        buffer_view = {
            "buffer": 0,
            "byteOffset": buffer_offset,
            "byteLength": len(packed),
            "target": 34962  # ARRAY_BUFFER
        }
        
        if dtype == "time":
            comp_type = 5126  # FLOAT
            accessor_type = "SCALAR"
        elif dtype == "vec3":
            comp_type = 5126  # FLOAT
            accessor_type = "VEC3"
        else:  # quat
            comp_type = 5126  # FLOAT
            accessor_type = "VEC4"
        
        accessor = {
            "bufferView": idx,
            "byteOffset": 0,
            "componentType": comp_type,
            "count": count,
            "type": accessor_type,
            "min": [min(data)] if dtype == "time" else None,
            "max": [max(data)] if dtype == "time" else None,
        }
        accessor = {k: v for k, v in accessor.items() if v is not None}
        
        accessors.append(accessor)
        buffer_views.append(buffer_view)
        buffer_offset += len(packed)
    
    return bytes(buffer_data), buffer_views, accessors

# ============================================================================
# Build glTF Structure
# ============================================================================

def build_gltf():
    """Build complete glTF JSON structure"""
    
    # Create geometry
    vertices, indices = create_humanoid_mesh()
    
    # Create skeleton
    nodes, joints = create_skeleton()
    
    # Create animations
    animations = create_animations()
    
    # Create buffer data
    buffer_data, buffer_views, accessors = create_buffer_views_and_accessors()
    
    # Build glTF structure
    gltf = {
        "asset": {
            "generator": "Arena Engine Character Generator",
            "version": "2.0"
        },
        "scene": 0,
        "scenes": [
            {"nodes": [0]}
        ],
        "nodes": nodes,
        "meshes": [
            {
                "name": "CharacterMesh",
                "primitives": [
                    {
                        "attributes": {"POSITION": 0},
                        "indices": 1,
                        "material": 0
                    }
                ]
            }
        ],
        "materials": [
            {
                "name": "CharacterMaterial",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [0.8, 0.8, 0.8, 1.0],
                    "metallicFactor": 0.0,
                    "roughnessFactor": 0.5
                }
            }
        ],
        "accessors": accessors,
        "bufferViews": buffer_views,
        "buffers": [
            {
                "byteLength": len(buffer_data),
                "uri": "data:application/octet-stream;base64,..."  # Will be replaced
            }
        ],
        "animations": animations,
        "skins": [
            {
                "joints": joints,
                "skeleton": 0,
                "inverseBindMatrices": 2  # Accessor index for IBM
            }
        ]
    }
    
    return gltf, buffer_data

# ============================================================================
# Export glTF
# ============================================================================

def write_glb(output_path, gltf_json, buffer_data):
    """Write glTF in binary GLB format"""
    
    # Encode JSON
    json_str = json.dumps(gltf_json)
    json_bytes = json_str.encode('utf-8')
    
    # Pad JSON to 4-byte boundary
    json_padding = (4 - (len(json_bytes) % 4)) % 4
    json_bytes += b' ' * json_padding
    
    # Combine JSON and buffer data
    combined = json_bytes + buffer_data
    padding = (4 - (len(combined) % 4)) % 4
    combined += b'\0' * padding
    
    # GLB header
    glb = bytearray()
    glb += b'glTF'  # Magic
    glb += struct.pack('<I', 2)  # Version 2
    glb += struct.pack('<I', 12 + 8 + len(json_bytes) + 8 + len(buffer_data) + padding)  # File size
    
    # JSON chunk
    glb += struct.pack('<I', len(json_bytes))  # Chunk size
    glb += b'JSON'  # Chunk type
    glb += json_bytes
    
    # Binary chunk
    glb += struct.pack('<I', len(buffer_data) + padding)  # Chunk size
    glb += b'BIN\0'  # Chunk type
    glb += buffer_data
    glb += b'\0' * padding
    
    with open(output_path, 'wb') as f:
        f.write(glb)

# ============================================================================
# Main
# ============================================================================

def main():
    print("Generating rigged humanoid character model...")
    
    gltf_json, buffer_data = build_gltf()
    
    output_path = "/home/wyattw/Documents/Github/arena-engine/assets/models/character.glb"
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    write_glb(output_path, gltf_json, buffer_data)
    
    print(f"✓ Generated {output_path}")
    print(f"  File size: {os.path.getsize(output_path)} bytes")
    print(f"  Skeleton: 10 bones (Root, Spine, Head, LeftArm, LeftForearm, RightArm, RightForearm, LeftLeg, LeftFoot, RightLeg, RightFoot)")
    print(f"  Animations: Idle (2s), Walk (1s), Attack (0.6s)")

if __name__ == "__main__":
    main()
