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

// Material uniforms (binding 1)
layout(set = 0, binding = 1) uniform MaterialUBO {
    vec4 baseColor;
    float metallic;
    float roughness;
    float ambientOcclusion;
    float padding;
} material;

// Light uniforms (binding 2)
layout(set = 0, binding = 2) uniform LightUBO {
    vec4 lightDirection;  // xyz = direction, w = intensity
    vec4 lightColor;      // rgb = color, a = ambient intensity
    vec4 cameraPosition;  // xyz = camera world position
} light;

// Texture samplers (optional - binding 3, 4, 5)
layout(set = 1, binding = 0) uniform sampler2D albedoMap;
// layout(set = 1, binding = 1) uniform sampler2D normalMap;  // TODO: Add later
// layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;  // TODO

// Output
layout(location = 0) out vec4 outColor;

// Constants
const float PI = 3.14159265359;

void main() {
    // Sample albedo texture or use material base color
    vec4 albedo = material.baseColor * texture(albedoMap, fragUV);
    
    // Discard fully transparent pixels
    if (albedo.a < 0.01) {
        discard;
    }
    
    // Normalize interpolated normal
    vec3 N = normalize(fragNormal);
    
    // Light direction (pointing towards light)
    vec3 L = normalize(-light.lightDirection.xyz);
    
    // View direction
    vec3 V = normalize(light.cameraPosition.xyz - fragPosition);
    
    // Half vector (for Blinn-Phong)
    vec3 H = normalize(L + V);
    
    // Ambient lighting
    float ambientStrength = light.lightColor.a;
    vec3 ambient = ambientStrength * light.lightColor.rgb * albedo.rgb;
    
    // Diffuse lighting (Lambert)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * light.lightColor.rgb * light.lightDirection.w * albedo.rgb;
    
    // Specular lighting (Blinn-Phong)
    float shininess = (1.0 - material.roughness) * 128.0 + 8.0;
    float spec = pow(max(dot(N, H), 0.0), shininess);
    // Reduce specular for rough/non-metallic surfaces
    spec *= (1.0 - material.roughness) * (material.metallic * 0.5 + 0.5);
    vec3 specular = spec * light.lightColor.rgb * light.lightDirection.w;
    
    // Combine lighting
    vec3 result = ambient + diffuse + specular;
    
    // Apply ambient occlusion
    result *= material.ambientOcclusion;
    
    // Simple tone mapping (Reinhard)
    result = result / (result + vec3(1.0));
    
    // Gamma correction
    result = pow(result, vec3(1.0 / 2.2));
    
    outColor = vec4(result, albedo.a);
}

