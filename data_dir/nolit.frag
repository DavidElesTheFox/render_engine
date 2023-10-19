#version 450

layout(binding = 0) uniform ColorOffset {
    float r;
} colorOffset;

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor.x + colorOffset.r, fragColor.y, fragColor.z, 1.0);
}
