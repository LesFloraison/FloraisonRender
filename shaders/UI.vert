#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aTextureIndex;
layout(location = 3) in vec3 aNormal;

layout(push_constant) uniform constant
{
    mat4 MVP;
} pushConstant;

layout(location = 0) out BLOCK_OUT{
    out vec2 texCoord;
    out vec3 textureIndex;
}vs_out;

void main() {
    gl_Position = pushConstant.MVP * vec4(aPosition, 1.0);
    vs_out.texCoord = aTexCoord;
    vs_out.textureIndex = aTextureIndex;
}