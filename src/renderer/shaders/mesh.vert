#version 450

// ============================================================================
// 3D Mesh Vertex Shader
// ============================================================================

// Vertex inputs (matches Vertex3D struct)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;

// Push constants for per-object model matrix
layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

// Combined uniform buffer (shared across all draws)
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

// Outputs to fragment shader
layout(location = 0) out vec3 fragPosition;   // World position
layout(location = 1) out vec3 fragNormal;     // World normal
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;

void main() {
    // Transform position to world space using push constant model matrix
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragPosition = worldPos.xyz;

    // Transform to clip space
    gl_Position = ubo.projection * ubo.view * worldPos;

    // Transform normal to world space
    fragNormal = normalize(mat3(push.model) * inNormal);

    // Pass through UV
    fragUV = inUV;

    // Transform tangent to world space
    fragTangent = normalize(mat3(push.model) * inTangent.xyz);

    // Calculate bitangent
    fragBitangent = cross(fragNormal, fragTangent) * inTangent.w;
}

