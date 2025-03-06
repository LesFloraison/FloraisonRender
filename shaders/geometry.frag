#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in BLOCK_IN{
    in vec4 fragWPos;
    in vec2 texCoord;
    in vec3 gVertexNormal;
    in vec3 textureIndex;
    in float roughness;
} fs_in;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;

void main() {
    gPosition = fs_in.fragWPos;
    gNormal = vec4(fs_in.gVertexNormal, 1);
    gAlbedo = vec4(texture(texSampler[int(fs_in.textureIndex.x+0.1)], fs_in.texCoord).rgb, fs_in.roughness);
}