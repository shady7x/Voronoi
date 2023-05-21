#version 450

layout(location = 0) in vec3 color;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(color, 1.0);

    // float distX = color.x / length(vec2(dFdx(color.x), dFdy(color.x)));
    // float distY = color.y / length(vec2(dFdx(color.y), dFdy(color.y)));
    // float distZ = color.z / length(vec2(dFdx(color.z), dFdy(color.z)));

    // if (distX < 2.0 || distY < 2.0 || distZ < 2.0) {
    //     outColor = vec4(0.0, 0.0, 0.0, 1.0);
    // } else {
    //     outColor = vec4(0.0, 1.0, 0.0, 1.0);
    // } 
}