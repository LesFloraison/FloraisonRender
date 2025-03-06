#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in BLOCK_IN{
    in vec2 texCoord;
    in vec3 textureIndex;
    in vec3 color;
} fs_in;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(location = 0) out vec4 gFont;

void main() {
    //gFont = vec4(1,0,1,texture(texSampler[int(fs_in.textureIndex.x+0.1)], fs_in.texCoord).r);
    gFont = vec4(fs_in.color, texture(texSampler[int(fs_in.textureIndex.x+0.1)], fs_in.texCoord).r);
    //gFont = vec4(1,0,1,1);
}