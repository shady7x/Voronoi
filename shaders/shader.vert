#version 450

layout(set = 0, binding = 0) uniform Matrices {
    mat4 mvp;
    mat4 mv;
    mat4 normal;
} ubo;

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec3 vOutline; 

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 position;
layout(location = 2) out vec3 normal;
layout(location = 3) noperspective out vec3 outline; 


void main() {
    color = vColor;
    position = vec3(ubo.mv * vec4(vPosition, 1.0));
    normal = normalize(mat3(ubo.normal) * vNormal);
    outline = vOutline;
    gl_Position = ubo.mvp * vec4(vPosition, 1.0);
}