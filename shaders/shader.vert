#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) noperspective out vec3 vk;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    vk = inColor;
}