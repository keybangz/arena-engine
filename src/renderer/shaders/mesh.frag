#version 450

// ============================================================================
// 3D Mesh Fragment Shader (Blinn-Phong Lighting)
// ============================================================================

// Inputs from vertex shader
layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

// Combined uniform buffer (all data in single binding, matches vertex shader)
layout(set = 0, binding = 0) uniform CombinedUBO {
    // Mesh transforms (unused in fragment shader but needed for layout match)
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normalMatrix;

    // Material
    vec4 baseColor;
    float metallic;
    float roughness;
    float ambientOcclusion;
    float _pad1;

    // Lighting
    vec4 lightDirection;  // xyz = direction, w = intensity
    vec4 lightColor;      // rgb = color, a = ambient intensity
    vec4 cameraPosition;  // xyz = camera world position
} ubo;

// Texture samplers
layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;

// Output
layout(location = 0) out vec4 outColor;

// Constants
const float PI = 3.14159265359;

// Get normal from normal map using TBN matrix
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, fragUV).rgb * 2.0 - 1.0;

    vec3 T = normalize(fragTangent);
    vec3 B = normalize(fragBitangent);
    vec3 N = normalize(fragNormal);
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    // Sample albedo texture or use material base color
    vec4 albedo = ubo.baseColor * texture(albedoMap, fragUV);

    // Discard fully transparent pixels
    if (albedo.a < 0.01) {
        discard;
    }

    // Get normal (from normal map or vertex normal)
    vec3 N = getNormalFromMap();

    // Light direction (pointing towards light)
    vec3 L = normalize(-ubo.lightDirection.xyz);

    // View direction
    vec3 V = normalize(ubo.cameraPosition.xyz - fragPosition);

    // Half vector (for Blinn-Phong)
    vec3 H = normalize(L + V);

    // Ambient lighting
    float ambientStrength = ubo.lightColor.a;
    vec3 ambient = ambientStrength * ubo.lightColor.rgb * albedo.rgb;

    // Diffuse lighting (Lambert)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * ubo.lightColor.rgb * ubo.lightDirection.w * albedo.rgb;

    // Specular lighting (Blinn-Phong)
    float shininess = (1.0 - ubo.roughness) * 128.0 + 8.0;
    float spec = pow(max(dot(N, H), 0.0), shininess);
    // Reduce specular for rough/non-metallic surfaces
    spec *= (1.0 - ubo.roughness) * (ubo.metallic * 0.5 + 0.5);
    vec3 specular = spec * ubo.lightColor.rgb * ubo.lightDirection.w;

    // Combine lighting
    vec3 result = ambient + diffuse + specular;

    // Apply ambient occlusion
    result *= ubo.ambientOcclusion;

    // Simple tone mapping (Reinhard)
    result = result / (result + vec3(1.0));

    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));

    outColor = vec4(result, albedo.a);
}

