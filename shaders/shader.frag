#version 450

layout(set = 1, binding = 0) uniform LightInfo {
    vec4 position;
    vec3 intensity;
} light;

layout(location = 0) in vec3 inColor;
layout(location = 1) noperspective in vec3 outline;

layout(location = 0) out vec4 outColor;

void main() {
    float distX = outline.x / length(vec2(dFdx(outline.x), dFdy(outline.x)));
    float distY = outline.y / length(vec2(dFdx(outline.y), dFdy(outline.y)));
    float distZ = outline.z / length(vec2(dFdx(outline.z), dFdy(outline.z)));

    if (light.position.x > 0.5) {
        if (distX < 1.0 || distY < 1.0 || distZ < 1.0) {
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            outColor = vec4(inColor, 1.0);
        } 
    } else {
        outColor = vec4(inColor, 1.0);
    }
}