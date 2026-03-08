#version 450

layout(push_constant) uniform PushConstants {
    vec4 rect;       // x, y, width, height
    vec4 color;      // r, g, b, a
    vec2 screenSize; // screen width, height
} pc;

// Quad vertices (two triangles)
vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),  // top-left
    vec2(0.0, 1.0),  // bottom-left
    vec2(1.0, 1.0),  // bottom-right
    vec2(0.0, 0.0),  // top-left
    vec2(1.0, 1.0),  // bottom-right
    vec2(1.0, 0.0)   // top-right
);

layout(location = 0) out vec4 fragColor;

void main() {
    vec2 pos = positions[gl_VertexIndex];
    
    // Transform to rect position
    vec2 worldPos = pc.rect.xy + pos * pc.rect.zw;
    
    // Convert to NDC
    vec2 ndc = (worldPos / pc.screenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // Flip Y
    
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragColor = pc.color;
}

