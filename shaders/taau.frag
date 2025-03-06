#version 460
#define gPosition texSampler[0]
#define gNormal texSampler[1]
#define gSampleHandle texSampler[2]
#define gHistoricalTaau texSampler[3]
#define gHistoricalTaauPosition texSampler[4]
#define gHistoricalTaauNormal texSampler[5]

layout(location = 0) in vec2 texCoord;
layout(binding = 1) uniform sampler2D texSampler[190];

layout(push_constant) uniform constant
{
	vec4 v4; //innerW, innerH, currentSubPixel, 0
	mat4 m4; //historicalVP
} pushConstant;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 FragPosition;
layout(location = 2) out vec4 FragNormal;

void main()
{
	ivec2 innerWH = ivec2(pushConstant.v4.xy);
	int currentSubPixel = int(pushConstant.v4.z);
	mat4 historicalVP = pushConstant.m4;
	vec4 position = texelFetch(gPosition, ivec2(texCoord*innerWH), 0);
	vec3 normal = texelFetch(gNormal, ivec2(texCoord*innerWH), 0).xyz;
	vec2 historicalUV = (((historicalVP * vec4(position.xyz,1)).xy / (historicalVP * vec4(position.xyz,1)).w)) * 0.5 + 0.5;
	ivec2 offset = ivec2(int(gl_FragCoord.x)%2, int(gl_FragCoord.y)%2);
	ivec2 historicalPIX = ivec2(historicalUV*innerWH)*2 + offset;
	int alpha = position.w==0 ? 0 : 1;
	if(alpha == 0){
		historicalPIX = ivec2(gl_FragCoord.xy);
	}
	vec4 outColor = texelFetch(gHistoricalTaau, historicalPIX, 0);
	vec4 outPosition = texelFetch(gHistoricalTaauPosition, historicalPIX, 0);
	vec4 outNormal = texelFetch(gHistoricalTaauNormal, historicalPIX, 0);

	int alpha_1 = texelFetch(gPosition, ivec2(texCoord*innerWH) + ivec2(1,0), 0).w==0 ? 0 : 1;
	int alpha_2 = texelFetch(gPosition, ivec2(texCoord*innerWH) + ivec2(-1,0), 0).w==0 ? 0 : 1;
	int alpha_3 = texelFetch(gPosition, ivec2(texCoord*innerWH) + ivec2(0,1), 0).w==0 ? 0 : 1;
	int alpha_4 = texelFetch(gPosition, ivec2(texCoord*innerWH) + ivec2(0,-1), 0).w==0 ? 0 : 1;
	if(alpha_1 + alpha_2 + alpha_3 + alpha_4 == 0){
		outColor = vec4(0);
	}

	if(historicalPIX.x>innerWH.x*2-1 || historicalPIX.y>innerWH.y*2-1 || historicalPIX.x<0 || historicalPIX.y<0){
		outColor = vec4(texelFetch(gSampleHandle, ivec2(texCoord*innerWH), 0).xyz, alpha);
		outPosition = vec4(texelFetch(gPosition, ivec2(texCoord*innerWH), 0).xyz, alpha);
		outNormal = vec4(texelFetch(gNormal, ivec2(texCoord*innerWH), 0).xyz, alpha);
	}
	if(currentSubPixel == 0){
		if(int(gl_FragCoord.x)%2 == 0 && int(gl_FragCoord.y)%2 == 0){
			outColor = vec4(texelFetch(gSampleHandle, ivec2(texCoord*innerWH), 0).xyz, alpha);
			outPosition = vec4(texelFetch(gPosition, ivec2(texCoord*innerWH), 0).xyz, alpha);
			outNormal = vec4(texelFetch(gNormal, ivec2(texCoord*innerWH), 0).xyz, alpha);
		}
	}
	if(currentSubPixel == 1){
		if(int(gl_FragCoord.x)%2 == 1 && int(gl_FragCoord.y)%2 == 1){
			outColor = vec4(texelFetch(gSampleHandle, ivec2(texCoord*innerWH), 0).xyz, alpha);
			outPosition = vec4(texelFetch(gPosition, ivec2(texCoord*innerWH), 0).xyz, alpha);
			outNormal = vec4(texelFetch(gNormal, ivec2(texCoord*innerWH), 0).xyz, alpha);
		}
	}
	if(currentSubPixel == 2){
		if(int(gl_FragCoord.x)%2 == 0 && int(gl_FragCoord.y)%2 == 1){
			outColor = vec4(texelFetch(gSampleHandle, ivec2(texCoord*innerWH), 0).xyz, alpha);
			outPosition = vec4(texelFetch(gPosition, ivec2(texCoord*innerWH), 0).xyz, alpha);
			outNormal = vec4(texelFetch(gNormal, ivec2(texCoord*innerWH), 0).xyz, alpha);
		}
	}
	if(currentSubPixel == 3){
		if(int(gl_FragCoord.x)%2 == 1 && int(gl_FragCoord.y)%2 == 0){
			outColor = vec4(texelFetch(gSampleHandle, ivec2(texCoord*innerWH), 0).xyz, alpha);
			outPosition = vec4(texelFetch(gPosition, ivec2(texCoord*innerWH), 0).xyz, alpha);
			outNormal = vec4(texelFetch(gNormal, ivec2(texCoord*innerWH), 0).xyz, alpha);
		}
	}
	FragColor = outColor;
	FragPosition = outPosition;
	FragNormal = outNormal;
}