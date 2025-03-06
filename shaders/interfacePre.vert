#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aTextureIndex;
layout(location = 3) in vec3 aCoef;

layout(location = 0) out BLOCK_OUT{
    out vec2 texCoord;
    out vec3 textureIndex;
    out float rad;
} vs_out;

layout(push_constant) uniform constant
{
	vec4 v4; //innerW, innerH, sig, page
	mat4 m4;
} pushConstant;

void main() {
    int page = int(pushConstant.v4.w);
    gl_Position = vec4(aPosition - 2 * vec3(page,0,0), 1.0);
    vs_out.texCoord = aTexCoord;
    vs_out.textureIndex = aTextureIndex;
    vs_out.rad = aCoef.x;
}