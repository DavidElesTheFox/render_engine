#version 460
layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler3D texIntensity;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput texRayStart;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput texRayEnd;

// TODO implement proper blending
vec4 blendColors(vec4 src, vec4 dst)
{
    vec4 result = vec4(dst.rgb * dst.a + (1.0 - dst.a) * src.rgb, dst.a);
    return result;
}
// TODO replace it with push constant
#define STEP_SIZE 0.01
#define MAX_STEP 100
void main() {
    vec3 rayStart = subpassLoad(texRayStart).xyz;
    vec3 rayEnd = subpassLoad(texRayEnd).xyz;
    vec3 p = rayStart;
    vec3 dir = normalize(rayEnd - rayStart);
    float intensity = 0.0f;
    int stepCount = 0;
    outColor = vec4(0.0);
    while(stepCount < MAX_STEP)
    {
        p += dir * STEP_SIZE;
        vec4 currentColor = texture(texIntensity, p);
        outColor = blendColors(outColor, currentColor);
        stepCount++;
    }
    // TODO remove this for supporting proper visibility
    outColor.a = 1.0;
}
