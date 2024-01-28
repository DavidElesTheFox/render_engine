#version 460
precision mediump int;

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler3D texIntensity;
layout(binding = 3) uniform sampler3D texDistanceField;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput texRayStart;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput texRayEnd;

layout(push_constant, std430) uniform constants
{
    layout(offset = 128) 
    int c_num_steps;
    float step_size;
};

void main() {
    const vec4 epsilon = vec4(0.1);
    const vec3 rayStart = subpassLoad(texRayStart).xyz;
    const vec3 rayEnd = subpassLoad(texRayEnd).xyz;
    vec3 p = rayStart;
    const vec3 dir = normalize(rayEnd - rayStart);
    int stepCount = 0;
    vec4 resultColor = vec4(0.0);
    while(stepCount < c_num_steps && dot(resultColor, resultColor) < epsilon.x)
    {
        float closest_point_distance = texture(texDistanceField, p).r;
        p += dir * closest_point_distance;

        if(closest_point_distance >= 0.01)
        {
            continue;
        }
        vec4 currentColor = texture(texIntensity, p);
        resultColor += step(epsilon, currentColor) * currentColor;
        stepCount++;
    }
    outColor = resultColor;
}
