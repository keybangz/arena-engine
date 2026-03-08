#include "material.h"
#include <stdio.h>
#include <string.h>

bool material_manager_init(MaterialManager* mm, TextureManager* texture_manager) {
    memset(mm, 0, sizeof(MaterialManager));
    mm->texture_manager = texture_manager;
    
    // Create default material (white, non-metallic, medium roughness)
    mm->default_material = material_create_pbr(mm,
        vec4(1.0f, 1.0f, 1.0f, 1.0f),  // white base color
        0.0f,   // non-metallic
        0.5f,   // medium roughness
        texture_get_white(texture_manager));
    
    printf("MaterialManager: Initialized (default material: %u)\n", mm->default_material);
    return true;
}

void material_manager_cleanup(MaterialManager* mm) {
    // Materials don't own textures, nothing to free
    mm->material_count = 0;
}

MaterialHandle material_create(MaterialManager* mm) {
    return material_create_pbr(mm, vec4(1,1,1,1), 0.0f, 0.5f, texture_get_white(mm->texture_manager));
}

MaterialHandle material_create_pbr(MaterialManager* mm, Vec4 base_color, float metallic, float roughness, TextureHandle albedo) {
    if (mm->material_count >= MAX_MATERIALS) return MATERIAL_HANDLE_INVALID;
    
    MaterialHandle handle = mm->material_count++;
    Material* mat = &mm->materials[handle];
    
    mat->base_color = base_color;
    mat->metallic = metallic;
    mat->roughness = roughness;
    mat->ambient_occlusion = 1.0f;
    mat->emissive_strength = 0.0f;
    
    mat->albedo_texture = albedo;
    mat->normal_texture = texture_get_normal(mm->texture_manager);
    mat->metallic_roughness_texture = TEXTURE_HANDLE_INVALID;
    mat->emissive_texture = TEXTURE_HANDLE_INVALID;
    
    mat->is_valid = true;
    mat->is_transparent = base_color.w < 1.0f;
    mat->double_sided = false;
    
    return handle;
}

void material_set_albedo(MaterialManager* mm, MaterialHandle h, TextureHandle tex) {
    if (h < mm->material_count) mm->materials[h].albedo_texture = tex;
}

void material_set_normal(MaterialManager* mm, MaterialHandle h, TextureHandle tex) {
    if (h < mm->material_count) mm->materials[h].normal_texture = tex;
}

void material_set_metallic_roughness(MaterialManager* mm, MaterialHandle h, TextureHandle tex) {
    if (h < mm->material_count) mm->materials[h].metallic_roughness_texture = tex;
}

void material_set_base_color(MaterialManager* mm, MaterialHandle h, Vec4 color) {
    if (h < mm->material_count) {
        mm->materials[h].base_color = color;
        mm->materials[h].is_transparent = color.w < 1.0f;
    }
}

void material_set_metallic(MaterialManager* mm, MaterialHandle h, float m) {
    if (h < mm->material_count) mm->materials[h].metallic = m;
}

void material_set_roughness(MaterialManager* mm, MaterialHandle h, float r) {
    if (h < mm->material_count) mm->materials[h].roughness = r;
}

Material* material_get(MaterialManager* mm, MaterialHandle h) {
    return h < mm->material_count ? &mm->materials[h] : NULL;
}

MaterialHandle material_get_default(MaterialManager* mm) {
    return mm->default_material;
}

