#ifndef MATERIAL_H
#define MATERIAL_H

#include "texture.h"
#include "arena/math/math.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Material Handle System
// ============================================================================

typedef uint32_t MaterialHandle;
#define MATERIAL_HANDLE_INVALID 0xFFFFFFFF
#define MAX_MATERIALS 128

// PBR Material properties
typedef struct Material {
    // Base properties
    Vec4 base_color;
    float metallic;
    float roughness;
    float ambient_occlusion;
    float emissive_strength;
    
    // Texture references
    TextureHandle albedo_texture;
    TextureHandle normal_texture;
    TextureHandle metallic_roughness_texture;
    TextureHandle emissive_texture;
    
    // Flags
    bool is_valid;
    bool is_transparent;
    bool double_sided;
    uint8_t padding;
} Material;

typedef struct MaterialManager {
    // Material storage
    Material materials[MAX_MATERIALS];
    uint32_t material_count;
    
    // Texture manager reference
    TextureManager* texture_manager;
    
    // Default material
    MaterialHandle default_material;
} MaterialManager;

// ============================================================================
// API
// ============================================================================

// Initialize material manager
bool material_manager_init(MaterialManager* mm, TextureManager* texture_manager);

// Cleanup
void material_manager_cleanup(MaterialManager* mm);

// Create material with default values
MaterialHandle material_create(MaterialManager* mm);

// Create material with properties
MaterialHandle material_create_pbr(MaterialManager* mm,
                                   Vec4 base_color,
                                   float metallic,
                                   float roughness,
                                   TextureHandle albedo);

// Set material textures
void material_set_albedo(MaterialManager* mm, MaterialHandle handle, TextureHandle texture);
void material_set_normal(MaterialManager* mm, MaterialHandle handle, TextureHandle texture);
void material_set_metallic_roughness(MaterialManager* mm, MaterialHandle handle, TextureHandle texture);

// Set material properties
void material_set_base_color(MaterialManager* mm, MaterialHandle handle, Vec4 color);
void material_set_metallic(MaterialManager* mm, MaterialHandle handle, float metallic);
void material_set_roughness(MaterialManager* mm, MaterialHandle handle, float roughness);

// Get material
Material* material_get(MaterialManager* mm, MaterialHandle handle);

// Get default material
MaterialHandle material_get_default(MaterialManager* mm);

#endif // MATERIAL_H

