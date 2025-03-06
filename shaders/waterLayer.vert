#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aTextureIndex;
layout(location = 3) in vec3 aNormal;

layout(push_constant) uniform constant
{
	vec4 v4; //innerW, innerH, ssp, ssp_2
	mat4 m4;//M
    vec4 v4_2;
    mat4 m4_2;//VP
} pushConstant;

layout(location = 0) out BLOCK_OUT{
    out vec4 fragWPos;
    out vec2 texCoord;
    out vec3 textureIndex;
}vs_out;

void main() {
    gl_Position = pushConstant.m4_2 * pushConstant.m4 * vec4(aPosition, 1.0);
    vs_out.fragWPos = pushConstant.m4 * vec4(aPosition, 1.0);
    vs_out.texCoord = aTexCoord;
    vs_out.textureIndex = aTextureIndex;
}