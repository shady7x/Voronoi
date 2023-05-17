#version 450

layout(location = 0) noperspective in vec3 vk;

layout(location = 0) out vec4 outColor;

void main() {
    float distX = vk.x / length(vec2(dFdx(vk.x), dFdy(vk.x)));
    float distY = vk.y / length(vec2(dFdx(vk.y), dFdy(vk.y)));
    float distZ = vk.z / length(vec2(dFdx(vk.z), dFdy(vk.z)));

    if (distX < 4 || distY < 4 || distZ < 4) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        outColor = vec4(0.0, 1.0, 0.0, 1.0);
    } 
}