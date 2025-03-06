#version 460
#extension GL_ARB_separate_shader_objects : enable
#define gBackground texSampler[0]

layout(location = 0) in BLOCK_IN{
    in vec2 texCoord;
    in vec3 textureIndex;
    in float rad;
} fs_in;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(push_constant) uniform constant
{
	vec4 v4; //innerW, innerH, sig
	mat4 m4;
} pushConstant;

layout(location = 0) out vec4 gBackground_out;
layout(location = 1) out vec4 gInterfaceTexture;

void main() {
	ivec2 innerWH = ivec2(pushConstant.v4.xy);
	int rad = int(fs_in.rad + 0.1);
	float sig = pushConstant.v4.z;
	vec3 bgc = vec3(0);
	float gC = 0;
    for(int i=-rad; i<=rad; i++){
		if(int(gl_FragCoord.x) + i < 0 || int(gl_FragCoord.x) + i > innerWH.x - 1){
			continue;
		}
		float gauss = (1 / (sig * sqrt(2 * 3.14))) * exp(-i * i / (2 * sig * sig));
		bgc += gauss * texelFetch(gBackground, ivec2(gl_FragCoord.xy) + ivec2(i,0), 0).rgb;
		gC += gauss;
	}
	bgc /= gC;
	vec4 texColor = texture(texSampler[int(fs_in.textureIndex.x+1+0.1)], fs_in.texCoord);
	vec4 bgColor = texelFetch(gBackground, ivec2(gl_FragCoord.xy), 0);
    //gInterface = vec4(bgc, 1);
    //gInterface = vec4(mix(texColor.rgb, bgc, 1-texColor.a), 1);
	gBackground_out = vec4(bgc, rad);
	gInterfaceTexture = texture(texSampler[int(fs_in.textureIndex.x+1+0.1)], fs_in.texCoord);
}