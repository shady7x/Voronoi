#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    mat3 normal;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 2) in vec3 inOutline; 

layout(location = 0) out vec3 outColor;
layout(location = 1) noperspective out vec3 outline; 


void main() {
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
    outColor = inColor;
    outline = inOutline;
}