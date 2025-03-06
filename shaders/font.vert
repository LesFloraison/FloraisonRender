#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aTextureIndex;
layout(location = 3) in vec3 aColor;

layout(location = 0) out BLOCK_OUT{
    out vec2 texCoord;
    out vec3 textureIndex;
    out vec3 color;
}vs_out;

layout(push_constant) uniform constant
{
    float textDisableTable[64];
} pushConstant;

void main() {
    int page = int(pushConstant.textDisableTable[0]);
    int flag = int(aTextureIndex.y);
    vec3 pos = aPosition;
    if(pushConstant.textDisableTable[flag + 1] == 1){
        pos = vec3(10);
    }
    gl_Position = vec4(pos - 2*vec3(page,0,0), 1.0);
    vs_out.texCoord = aTexCoord;
    vs_out.textureIndex = aTextureIndex;
    vs_out.color = aColor;
}