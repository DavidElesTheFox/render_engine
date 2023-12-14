#version 460
layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler3D texIntensity;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput texRayStart;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput texRayEnd;


void main() {
    vec3 rayStart = subpassLoad(texRayStart).xyz;
    vec3 rayEnd = subpassLoad(texRayEnd).xyz;
    outColor = vec4(rayEnd - rayStart, 0.0f);
}
