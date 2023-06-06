#version 450

layout(set = 0, binding = 0) uniform Matrices {
    mat4 mvp;
    mat4 mv;
    mat3 normal;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 2) in vec3 inOutline; 

layout(location = 0) out vec3 outColor;
layout(location = 1) noperspective out vec3 outline; 


void main() {
    outColor = inColor;
    outline = inOutline;
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
}