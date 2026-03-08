/**
 * gltf_loader.c - glTF 2.0 Model Loading Implementation
 */

#define CGLTF_IMPLEMENTATION
#include "../../libs/cgltf/cgltf.h"

#include "gltf_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

// ============================================================================
// Internal Helpers
// ============================================================================

static void set_error(GltfLoadResult* result, const char* fmt, ...) {
    result->success = false;
    va_list args;
    va_start(args, fmt);
    vsnprintf(result->error_message, sizeof(result->error_message), fmt, args);
    va_end(args);
}

static void read_vec3(const cgltf_accessor* accessor, cgltf_size index, float* out) {
    cgltf_accessor_read_float(accessor, index, out, 3);
}

static void read_vec2(const cgltf_accessor* accessor, cgltf_size index, float* out) {
    cgltf_accessor_read_float(accessor, index, out, 2);
}

static void read_vec4(const cgltf_accessor* accessor, cgltf_size index, float* out) {
    cgltf_accessor_read_float(accessor, index, out, 4);
}

// Calculate tangent vectors using Mikktspace algorithm (simplified)
static void calculate_tangents(Vertex3D* vertices, uint32_t vertex_count,
                               const uint32_t* indices, uint32_t index_count) {
    // Zero out tangents
    for (uint32_t i = 0; i < vertex_count; i++) {
        vertices[i].tangent[0] = 0;
        vertices[i].tangent[1] = 0;
        vertices[i].tangent[2] = 0;
        vertices[i].tangent[3] = 1.0f;  // Handedness
    }

    // Calculate per-triangle tangents and accumulate
    for (uint32_t i = 0; i < index_count; i += 3) {
        uint32_t i0 = indices[i + 0];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        Vertex3D* v0 = &vertices[i0];
        Vertex3D* v1 = &vertices[i1];
        Vertex3D* v2 = &vertices[i2];

        // Edge vectors
        float edge1[3] = {
            v1->position[0] - v0->position[0],
            v1->position[1] - v0->position[1],
            v1->position[2] - v0->position[2]
        };
        float edge2[3] = {
            v2->position[0] - v0->position[0],
            v2->position[1] - v0->position[1],
            v2->position[2] - v0->position[2]
        };

        // UV deltas
        float duv1[2] = { v1->uv[0] - v0->uv[0], v1->uv[1] - v0->uv[1] };
        float duv2[2] = { v2->uv[0] - v0->uv[0], v2->uv[1] - v0->uv[1] };

        float det = duv1[0] * duv2[1] - duv2[0] * duv1[1];
        if (fabsf(det) < 1e-8f) det = 1.0f;
        float r = 1.0f / det;

        float tangent[3] = {
            (edge1[0] * duv2[1] - edge2[0] * duv1[1]) * r,
            (edge1[1] * duv2[1] - edge2[1] * duv1[1]) * r,
            (edge1[2] * duv2[1] - edge2[2] * duv1[1]) * r
        };

        // Accumulate for all three vertices
        for (int j = 0; j < 3; j++) {
            vertices[indices[i + j]].tangent[0] += tangent[0];
            vertices[indices[i + j]].tangent[1] += tangent[1];
            vertices[indices[i + j]].tangent[2] += tangent[2];
        }
    }

    // Normalize tangents and orthogonalize to normals
    for (uint32_t i = 0; i < vertex_count; i++) {
        float* t = vertices[i].tangent;
        float* n = vertices[i].normal;

        // Gram-Schmidt orthogonalize: t = normalize(t - n * dot(n, t))
        float dot = n[0] * t[0] + n[1] * t[1] + n[2] * t[2];
        t[0] -= n[0] * dot;
        t[1] -= n[1] * dot;
        t[2] -= n[2] * dot;

        // Normalize
        float len = sqrtf(t[0]*t[0] + t[1]*t[1] + t[2]*t[2]);
        if (len > 1e-8f) {
            t[0] /= len;
            t[1] /= len;
            t[2] /= len;
        } else {
            // Fallback tangent
            t[0] = 1.0f; t[1] = 0.0f; t[2] = 0.0f;
        }
        t[3] = 1.0f;  // Handedness (could compute properly with bitangent)
    }
}

// Update bounding box
static void update_bounds(Vec3* bounds_min, Vec3* bounds_max, const float* pos) {
    if (pos[0] < bounds_min->x) bounds_min->x = pos[0];
    if (pos[1] < bounds_min->y) bounds_min->y = pos[1];
    if (pos[2] < bounds_min->z) bounds_min->z = pos[2];
    if (pos[0] > bounds_max->x) bounds_max->x = pos[0];
    if (pos[1] > bounds_max->y) bounds_max->y = pos[1];
    if (pos[2] > bounds_max->z) bounds_max->z = pos[2];
}

// ============================================================================
// Load a single primitive from glTF
// ============================================================================

static bool load_primitive(MeshManager* mm, const cgltf_primitive* prim,
                           const GltfLoadOptions* options, GltfMeshInfo* out_info,
                           Vec3* bounds_min, Vec3* bounds_max) {
    // Only support triangles
    if (prim->type != cgltf_primitive_type_triangles) {
        return false;
    }

    // Find required attributes
    const cgltf_accessor* pos_accessor = NULL;
    const cgltf_accessor* norm_accessor = NULL;
    const cgltf_accessor* uv_accessor = NULL;
    const cgltf_accessor* tan_accessor = NULL;
    const cgltf_accessor* joints_accessor = NULL;
    const cgltf_accessor* weights_accessor = NULL;

    for (cgltf_size i = 0; i < prim->attributes_count; i++) {
        cgltf_attribute* attr = &prim->attributes[i];
        switch (attr->type) {
            case cgltf_attribute_type_position:  pos_accessor = attr->data; break;
            case cgltf_attribute_type_normal:    norm_accessor = attr->data; break;
            case cgltf_attribute_type_texcoord:
                if (!uv_accessor) uv_accessor = attr->data;
                break;
            case cgltf_attribute_type_tangent:   tan_accessor = attr->data; break;
            case cgltf_attribute_type_joints:    joints_accessor = attr->data; break;
            case cgltf_attribute_type_weights:   weights_accessor = attr->data; break;
            default: break;
        }
    }

    if (!pos_accessor) {
        return false;  // Position is required
    }

    uint32_t vertex_count = (uint32_t)pos_accessor->count;
    bool is_skinned = (joints_accessor != NULL && weights_accessor != NULL);

    // Allocate temporary vertex buffer
    Vertex3D* vertices = NULL;
    VertexSkinned* skinned_vertices = NULL;

    if (is_skinned) {
        skinned_vertices = (VertexSkinned*)calloc(vertex_count, sizeof(VertexSkinned));
        if (!skinned_vertices) return false;
    } else {
        vertices = (Vertex3D*)calloc(vertex_count, sizeof(Vertex3D));
        if (!vertices) return false;
    }

    // Read vertex data
    for (uint32_t i = 0; i < vertex_count; i++) {
        float pos[3] = {0, 0, 0};
        float norm[3] = {0, 1, 0};
        float uv[2] = {0, 0};
        float tan[4] = {1, 0, 0, 1};

        read_vec3(pos_accessor, i, pos);
        if (norm_accessor) read_vec3(norm_accessor, i, norm);
        if (uv_accessor) read_vec2(uv_accessor, i, uv);
        if (tan_accessor) read_vec4(tan_accessor, i, tan);

        // Apply scale
        pos[0] *= options->scale;
        pos[1] *= options->scale;
        pos[2] *= options->scale;

        // Flip UVs if requested
        if (options->flip_uvs) {
            uv[1] = 1.0f - uv[1];
        }

        // Update bounds
        update_bounds(bounds_min, bounds_max, pos);

        if (is_skinned) {
            memcpy(skinned_vertices[i].position, pos, sizeof(float) * 3);
            memcpy(skinned_vertices[i].normal, norm, sizeof(float) * 3);
            memcpy(skinned_vertices[i].uv, uv, sizeof(float) * 2);
            memcpy(skinned_vertices[i].tangent, tan, sizeof(float) * 4);

            // Read bone data
            if (joints_accessor) {
                cgltf_uint joints[4] = {0, 0, 0, 0};
                cgltf_accessor_read_uint(joints_accessor, i, joints, 4);
                skinned_vertices[i].bone_ids[0] = (uint8_t)joints[0];
                skinned_vertices[i].bone_ids[1] = (uint8_t)joints[1];
                skinned_vertices[i].bone_ids[2] = (uint8_t)joints[2];
                skinned_vertices[i].bone_ids[3] = (uint8_t)joints[3];
            }
            if (weights_accessor) {
                cgltf_accessor_read_float(weights_accessor, i,
                    skinned_vertices[i].bone_weights, 4);
            }
        } else {
            memcpy(vertices[i].position, pos, sizeof(float) * 3);
            memcpy(vertices[i].normal, norm, sizeof(float) * 3);
            memcpy(vertices[i].uv, uv, sizeof(float) * 2);
            memcpy(vertices[i].tangent, tan, sizeof(float) * 4);
        }
    }

    // Read indices
    uint32_t index_count = 0;
    uint32_t* indices = NULL;

    if (prim->indices) {
        index_count = (uint32_t)prim->indices->count;
        indices = (uint32_t*)malloc(index_count * sizeof(uint32_t));
        if (!indices) {
            free(vertices);
            free(skinned_vertices);
            return false;
        }

        for (uint32_t i = 0; i < index_count; i++) {
            indices[i] = (uint32_t)cgltf_accessor_read_index(prim->indices, i);
        }
    } else {
        // Generate indices for non-indexed geometry
        index_count = vertex_count;
        indices = (uint32_t*)malloc(index_count * sizeof(uint32_t));
        if (!indices) {
            free(vertices);
            free(skinned_vertices);
            return false;
        }
        for (uint32_t i = 0; i < index_count; i++) {
            indices[i] = i;
        }
    }

    // Calculate tangents if not present and requested
    if (!tan_accessor && options->calculate_tangents && !is_skinned && vertices) {
        calculate_tangents(vertices, vertex_count, indices, index_count);
    }

    // Create mesh
    MeshHandle handle;
    if (is_skinned) {
        handle = mesh_create_skinned(mm, skinned_vertices, vertex_count, indices, index_count);
    } else {
        handle = mesh_create(mm, vertices, vertex_count, indices, index_count);
    }

    // Cleanup temporary buffers
    free(vertices);
    free(skinned_vertices);
    free(indices);

    if (handle == MESH_HANDLE_INVALID) {
        return false;
    }

    // Fill output info
    out_info->handle = handle;
    out_info->is_skinned = is_skinned;
    out_info->vertex_count = vertex_count;
    out_info->index_count = index_count;

    return true;
}


// ============================================================================
// Public API Implementation
// ============================================================================

bool gltf_load_file(MeshManager* mm, const char* filepath, GltfLoadResult* result) {
    GltfLoadOptions options = gltf_load_options_default();
    return gltf_load_file_ex(mm, filepath, &options, result);
}

bool gltf_load_file_ex(MeshManager* mm, const char* filepath,
                       const GltfLoadOptions* options, GltfLoadResult* result) {
    // Initialize result
    memset(result, 0, sizeof(GltfLoadResult));
    result->bounds_min = vec3(INFINITY, INFINITY, INFINITY);
    result->bounds_max = vec3(-INFINITY, -INFINITY, -INFINITY);

    // Parse glTF file
    cgltf_options cgltf_opts = {0};
    cgltf_data* data = NULL;

    cgltf_result parse_result = cgltf_parse_file(&cgltf_opts, filepath, &data);
    if (parse_result != cgltf_result_success) {
        set_error(result, "Failed to parse glTF file: %s (error %d)", filepath, parse_result);
        return false;
    }

    // Load buffers (needed for accessor data)
    cgltf_result load_result = cgltf_load_buffers(&cgltf_opts, data, filepath);
    if (load_result != cgltf_result_success) {
        set_error(result, "Failed to load glTF buffers: %s (error %d)", filepath, load_result);
        cgltf_free(data);
        return false;
    }

    // Validate data
    cgltf_result validate_result = cgltf_validate(data);
    if (validate_result != cgltf_result_success) {
        set_error(result, "glTF validation failed: %s (error %d)", filepath, validate_result);
        cgltf_free(data);
        return false;
    }

    printf("glTF: Loading '%s' - %zu meshes, %zu nodes\n",
           filepath, data->meshes_count, data->nodes_count);

    // Load each mesh
    for (cgltf_size m = 0; m < data->meshes_count && result->mesh_count < GLTF_MAX_MESHES_PER_FILE; m++) {
        cgltf_mesh* mesh = &data->meshes[m];

        // Load each primitive in the mesh
        for (cgltf_size p = 0; p < mesh->primitives_count && result->mesh_count < GLTF_MAX_MESHES_PER_FILE; p++) {
            GltfMeshInfo* info = &result->meshes[result->mesh_count];

            if (load_primitive(mm, &mesh->primitives[p], options, info,
                              &result->bounds_min, &result->bounds_max)) {

                // Set name
                if (mesh->name) {
                    snprintf(info->name, GLTF_MAX_NAME_LENGTH, "%s", mesh->name);
                    if (mesh->primitives_count > 1) {
                        char suffix[16];
                        snprintf(suffix, sizeof(suffix), "_%zu", p);
                        strncat(info->name, suffix, GLTF_MAX_NAME_LENGTH - strlen(info->name) - 1);
                    }
                } else {
                    snprintf(info->name, GLTF_MAX_NAME_LENGTH, "mesh_%zu_%zu", m, p);
                }

                result->mesh_count++;
                printf("  Loaded: %s (%u verts, %u indices%s)\n",
                       info->name, info->vertex_count, info->index_count,
                       info->is_skinned ? ", skinned" : "");
            }
        }
    }

    cgltf_free(data);

    if (result->mesh_count == 0) {
        set_error(result, "No valid meshes found in: %s", filepath);
        return false;
    }

    result->success = true;
    printf("glTF: Loaded %u meshes from '%s'\n", result->mesh_count, filepath);
    return true;
}

bool gltf_load_memory(MeshManager* mm, const void* data, size_t size,
                      const GltfLoadOptions* options, GltfLoadResult* result) {
    // Initialize result
    memset(result, 0, sizeof(GltfLoadResult));
    result->bounds_min = vec3(INFINITY, INFINITY, INFINITY);
    result->bounds_max = vec3(-INFINITY, -INFINITY, -INFINITY);

    // Parse glTF from memory
    cgltf_options cgltf_opts = {0};
    cgltf_data* gltf_data = NULL;

    cgltf_result parse_result = cgltf_parse(&cgltf_opts, data, size, &gltf_data);
    if (parse_result != cgltf_result_success) {
        set_error(result, "Failed to parse glTF from memory (error %d)", parse_result);
        return false;
    }

    // For GLB files, buffers are embedded - load them
    // For .gltf with external buffers from memory, this won't work
    cgltf_result load_result = cgltf_load_buffers(&cgltf_opts, gltf_data, NULL);
    if (load_result != cgltf_result_success) {
        set_error(result, "Failed to load glTF buffers from memory (error %d)", load_result);
        cgltf_free(gltf_data);
        return false;
    }

    // Load meshes (same as file loading)
    for (cgltf_size m = 0; m < gltf_data->meshes_count && result->mesh_count < GLTF_MAX_MESHES_PER_FILE; m++) {
        cgltf_mesh* mesh = &gltf_data->meshes[m];

        for (cgltf_size p = 0; p < mesh->primitives_count && result->mesh_count < GLTF_MAX_MESHES_PER_FILE; p++) {
            GltfMeshInfo* info = &result->meshes[result->mesh_count];

            if (load_primitive(mm, &mesh->primitives[p], options, info,
                              &result->bounds_min, &result->bounds_max)) {
                snprintf(info->name, GLTF_MAX_NAME_LENGTH, "mesh_%zu_%zu", m, p);
                result->mesh_count++;
            }
        }
    }

    cgltf_free(gltf_data);

    if (result->mesh_count == 0) {
        set_error(result, "No valid meshes found in memory buffer");
        return false;
    }

    result->success = true;
    return true;
}

void gltf_load_result_free(GltfLoadResult* result) {
    // Currently nothing to free - all data is stack/inline
    // This is here for future extensibility
    (void)result;
}

void gltf_unload(MeshManager* mm, GltfLoadResult* result) {
    for (uint32_t i = 0; i < result->mesh_count; i++) {
        if (result->meshes[i].handle != MESH_HANDLE_INVALID) {
            mesh_destroy(mm, result->meshes[i].handle);
            result->meshes[i].handle = MESH_HANDLE_INVALID;
        }
    }
    result->mesh_count = 0;
}

bool gltf_is_valid_file(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return false;

    // Check for GLB magic number
    uint32_t magic;
    size_t read = fread(&magic, 4, 1, f);
    fclose(f);

    if (read != 1) return false;

    // GLB magic: 0x46546C67 ("glTF" in little-endian)
    if (magic == 0x46546C67) return true;

    // Could be .gltf JSON file - check extension
    const char* ext = gltf_get_extension(filepath);
    return ext && (strcmp(ext, ".gltf") == 0 || strcmp(ext, ".glb") == 0);
}

const char* gltf_get_extension(const char* filepath) {
    const char* dot = strrchr(filepath, '.');
    return dot ? dot : "";
}

