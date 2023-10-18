#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant, std430) uniform constants
{
    mat4 projection;
    mat4 model_view;
};

void main() {
    gl_Position = projection * model_view * vec4(inPosition, 1.0); 
}
