#version 450


layout(location = 0) out vec4 outColor;

layout(push_constant, std430) uniform constants
{
    layout(offset = 128) vec3 instance_color;
};

void main() {
    outColor = vec4(instance_color, 1.0);
}
