#version 450

// ============================================================================
// Skinned Mesh Vertex Shader (GPU Bone Skinning)
// ============================================================================
// Performs skeletal animation on GPU by applying bone transforms to vertex
// positions and normals. Supports up to 4 bones per vertex with weights.

// Vertex inputs (matches VertexSkinned struct)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in uvec4 inBoneIds;      // Up to 4 bone indices per vertex
layout(location = 5) in vec4 inBoneWeights;   // Corresponding weights (should sum to 1.0)

// Push constants for per-object model matrix
layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

// Combined uniform buffer (camera/lighting - same as non-skinned mesh)
layout(set = 0, binding = 0) uniform CombinedUBO {
    // Camera transforms (shared)
    mat4 view;
    mat4 projection;

    // Material (unused in vertex shader but needed for layout match)
    vec4 baseColor;
    float metallic;
    float roughness;
    float ambientOcclusion;
    float _pad1;

    // Lighting (unused in vertex shader but needed for layout match)
    vec4 lightDirection;
    vec4 lightColor;
    vec4 cameraPosition;
} ubo;

// Storage buffer for bone matrices
// Each frame, AnimationSystem updates this with current bone transforms
#define MAX_BONES 64
layout(set = 0, binding = 1) readonly buffer BoneMatrices {
    mat4 bones[MAX_BONES];
} boneData;

// Outputs to fragment shader (same as non-skinned)
layout(location = 0) out vec3 fragPosition;   // World position
layout(location = 1) out vec3 fragNormal;     // World normal
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;

void main() {
    // Apply bone skinning to position
    // Weighted blend of up to 4 bone transforms
    vec4 skinnedPosition = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);
    vec3 skinnedTangent = vec3(0.0);

    for (int i = 0; i < 4; i++) {
        float weight = inBoneWeights[i];
        
        // Skip if weight is negligible
        if (weight > 0.001) {
            // Get bone index and ensure it's in valid range
            uint boneIdx = min(inBoneIds[i], MAX_BONES - 1u);
            
            // Apply bone transform to position
            vec4 bonePos = boneData.bones[boneIdx] * vec4(inPosition, 1.0);
            skinnedPosition += weight * bonePos;
            
            // Apply bone rotation to normal (use 3x3 portion of bone matrix)
            skinnedNormal += weight * (mat3(boneData.bones[boneIdx]) * inNormal);
            
            // Apply bone rotation to tangent
            skinnedTangent += weight * (mat3(boneData.bones[boneIdx]) * inTangent.xyz);
        }
    }

    // Transform skinned position to world space using model matrix
    vec4 worldPos = push.model * skinnedPosition;
    fragPosition = worldPos.xyz;

    // Transform to clip space
    gl_Position = ubo.projection * ubo.view * worldPos;

    // Transform skinned normal to world space and normalize
    fragNormal = normalize(mat3(push.model) * skinnedNormal);

    // Pass through UV
    fragUV = inUV;

    // Transform skinned tangent to world space and normalize
    fragTangent = normalize(mat3(push.model) * skinnedTangent);

    // Calculate bitangent from normal and tangent
    // inTangent.w stores handedness (-1 or 1)
    fragBitangent = cross(fragNormal, fragTangent) * inTangent.w;
}
