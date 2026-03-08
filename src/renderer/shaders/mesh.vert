#version 450

// ============================================================================
// 3D Mesh Vertex Shader
// ============================================================================

// Vertex inputs (matches Vertex3D struct)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;

// Combined uniform buffer (all data in single binding)
layout(set = 0, binding = 0) uniform CombinedUBO {
    // Mesh transforms
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;

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
    // Transform position to world space
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    fragPosition = worldPos.xyz;
    
    // Transform to clip space
    gl_Position = ubo.projection * ubo.view * worldPos;
    
    // Transform normal to world space (use normal matrix)
    fragNormal = normalize(mat3(ubo.normalMatrix) * inNormal);
    
    // Pass through UV
    fragUV = inUV;
    
    // Transform tangent to world space
    fragTangent = normalize(mat3(ubo.model) * inTangent.xyz);
    
    // Calculate bitangent (tangent.w contains handedness)
    fragBitangent = cross(fragNormal, fragTangent) * inTangent.w;
}

