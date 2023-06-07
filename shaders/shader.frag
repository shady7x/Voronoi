#version 450

layout(set = 1, binding = 0) uniform LightInfo {
    vec4 position;
    vec4 color;
} light;

layout(location = 0) in vec3 fColor;
layout(location = 1) in vec3 fPosition;
layout(location = 2) in vec3 fNormal;
layout(location = 3) noperspective in vec3 outline; 

layout(location = 0) out vec4 outColor;

void main() {
    float distX = outline.x / length(vec2(dFdx(outline.x), dFdy(outline.x)));
    float distY = outline.y / length(vec2(dFdx(outline.y), dFdy(outline.y)));
    float distZ = outline.z / length(vec2(dFdx(outline.z), dFdy(outline.z)));

    vec3 ambient = light.color.rgb * light.color.a;

    float diff = max(dot(fNormal, normalize(fPosition - light.position.xyz)), 0.0) * light.position.w; 

    if (distX < 1.0 || distY < 1.0 || distZ < 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        outColor = vec4(((diff * light.color.rgb) + ambient) * fColor, 1.0);
    } 
}