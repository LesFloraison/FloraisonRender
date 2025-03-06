#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in BLOCK_IN{
    in vec4 position;
} fs_in;

layout(binding = 1) uniform sampler2D texSampler[190];
layout(binding = 2) uniform samplerCube hdrSkybox[190];

layout(location = 0) out vec4 gSky;

void main() {
    vec3 hdrColor = texture(hdrSkybox[0],normalize(vec3(fs_in.position))).rgb;
    vec3 reinhard = hdrColor / (hdrColor + vec3(1.0));
    gSky = vec4(reinhard, 1);
}