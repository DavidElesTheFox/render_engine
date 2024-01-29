#version 460
precision mediump int;

layout(location = 0) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput texRayStart;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput texRayEnd;
layout(binding = 2) uniform sampler3D texIntensity;
layout(binding = 3) uniform sampler3D texDistanceField;
layout(push_constant, std430) uniform constants
{
    layout(offset = 128) 
    int c_num_steps;
    float step_size;
};

float calculateAO(vec3 p, vec3 dir)
{
    const float neighbor_distance = 0.01;
    const float threshold = 0.001;
    const vec3 up = vec3(0.0, 1.0, 0.0);
    const vec3 axis_x = cross(dir, up);
    const vec3 axis_y = cross(axis_x, dir);
    
    

    const vec3 sample_center = p - dir  * neighbor_distance;

    const vec3 sample_points[8] = vec3[8](sample_center - axis_x * neighbor_distance - axis_y * neighbor_distance, 
                                          sample_center - axis_x * neighbor_distance,
                                          sample_center - axis_x * neighbor_distance + axis_y * neighbor_distance,
                                          
                                          sample_center - axis_y * neighbor_distance,
                                          sample_center + axis_y * neighbor_distance,
                                          
                                          sample_center + axis_x * neighbor_distance - axis_y * neighbor_distance,
                                          sample_center + axis_x * neighbor_distance,
                                          sample_center + axis_x * neighbor_distance + axis_y * neighbor_distance);
    
    float ao = 0.0;
    for(int i = 0; i < 8; ++i)
    {
        float closest_point_distance = texture(texDistanceField, sample_points[i]).r;
        if(closest_point_distance <= threshold)
        {
            ao += mix(0.4, 0.0, closest_point_distance / threshold);
        }
    }
    return ao;
}

void main() {
    const vec4 epsilon = vec4(0.1);
    const vec3 rayStart = subpassLoad(texRayStart).xyz;
    const vec3 rayEnd = subpassLoad(texRayEnd).xyz;
    vec3 p = rayStart;
    const vec3 dir = normalize(rayEnd - rayStart);
    int stepCount = 0;
    vec4 resultColor = vec4(0.0);
    while(stepCount < c_num_steps)
    {
        stepCount++;

        float closest_point_distance = texture(texDistanceField, p).r;
        p += 0.5 * dir * closest_point_distance;
        if(closest_point_distance <= 0.001)
        {
            resultColor = vec4(0.89, 0.85, 0.78, 1.) * (1.0 - calculateAO(p, dir));
        }
    }
    outColor = resultColor;
}
