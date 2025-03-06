#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in BLOCK_IN{
    in vec2 texCoord;
    in vec3 textureIndex;
} fs_in;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(location = 0) out vec4 gUI;

void main() {
    gUI = vec4(texture(texSampler[int(fs_in.textureIndex.x+0.1)], fs_in.texCoord).rgb,1);
}