#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant, std430) uniform constants
{
    mat4 projection;
    mat4 model_view;
};

void main() {
    gl_Position = projection * model_view * vec4(inPosition, 0.0, 1.0); 
    fragColor = inColor;
}
