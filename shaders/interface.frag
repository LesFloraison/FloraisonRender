#version 460
#extension GL_ARB_separate_shader_objects : enable
#define gFilterPre texSampler[0]
#define gInterfaceTexture texSampler[1]
layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 gInterface;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(push_constant) uniform constant
{
	vec4 v4; //innerW, innerH, sig, page
	mat4 m4;
} pushConstant;

void main(){
	ivec2 innerWH = ivec2(pushConstant.v4.xy);
	int rad = int(texelFetch(gFilterPre, ivec2(gl_FragCoord.xy), 0).a);
	float sig = pushConstant.v4.z;
	vec3 bgc = vec3(0);
	float gC = 0;
    for(int i=-rad; i<=rad; i++){
		if(int(gl_FragCoord.y) + i < 0 || int(gl_FragCoord.y) + i > innerWH.y - 1){
			continue;
		}
		if(rad != int(texelFetch(gFilterPre, ivec2(gl_FragCoord.xy) + ivec2(0,i), 0).a)){
			//gInterface = vec4(1,0,1,1);
			//return;
			continue;
		}
		float gauss = (1 / (sig * sqrt(2 * 3.14))) * exp(-i * i / (2 * sig * sig));
		bgc += gauss * texelFetch(gFilterPre, ivec2(gl_FragCoord.xy) + ivec2(0,i), 0).rgb;
		gC += gauss;
	}
	bgc /= gC;
	vec4 texColor = texelFetch(gInterfaceTexture, ivec2(gl_FragCoord.xy), 0);
	vec4 testbgColor = texelFetch(gFilterPre, ivec2(gl_FragCoord.xy), 0);
    //gInterface = vec4(bgc, 1);
    gInterface = vec4(mix(texColor.rgb, bgc, 1-texColor.a), texColor.a > 0 ? 1 : 0);
}