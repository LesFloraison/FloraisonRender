#version 460
#extension GL_ARB_separate_shader_objects : enable
#define gPosition texSampler[0]
#define gNormal texSampler[1]
#define gIndirect texSampler[2]
#define gamma 2.2f
layout(location = 0) in vec2 texCoord;

layout(binding = 1) uniform sampler2D texSampler[190];

layout (binding = 7, rgba16) uniform image3D indirectCache_2;

layout(push_constant) uniform constant
{
	vec4 v4; //invCameraPos, radianceCacheRad
	mat4 m4;
	vec4 v4_2; //chunkSize
} pushConstant;

layout(location = 0) out vec4 gNothing;

void main() {
	vec3 cameraPos = -pushConstant.v4.xyz; 
	int radianceCacheRad = int(pushConstant.v4.w);
	float chunkSize = pushConstant.v4_2.x;
	vec4 position = texelFetch(gPosition, ivec2(gl_FragCoord.xy), 0);
	vec3 normal = texelFetch(gNormal, ivec2(gl_FragCoord.xy), 0).xyz;
	vec3 radiance = texelFetch(gIndirect, ivec2(gl_FragCoord.xy), 0).xyz;
	float accu = texelFetch(gIndirect, ivec2(gl_FragCoord.xy), 0).a;
	int alpha = position.w==0 ? 0 : 1;
	if(alpha == 0){
		return;
	}
	if(accu < 29){
		return;
	}
	ivec3 writeCoord = ivec3(floor(position.xyz*(radianceCacheRad/chunkSize))) % radianceCacheRad;
	float chunkHash = int(dot(ivec3(5,3,1), ivec3(ceil(position.xyz / chunkSize)))) + 0.5;
	imageStore(indirectCache_2, writeCoord, vec4(radiance, chunkHash));
}