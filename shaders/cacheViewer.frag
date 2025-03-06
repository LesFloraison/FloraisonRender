#version 460
#extension GL_ARB_separate_shader_objects : enable
#define gPosition texSampler[0]
#define gNormal texSampler[1]
layout(location = 0) in vec2 texCoord;

layout(binding = 1) uniform sampler2D texSampler[190];

layout (binding = 6, rgba16) uniform image3D indirectCache_1;
layout (binding = 7, rgba16) uniform image3D indirectCache_2;

layout(push_constant) uniform constant
{
	vec4 v4; //invCameraPos, radianceCacheRad
	mat4 m4;
	vec4 v4_2; //chunkSize
} pushConstant;

layout(location = 0) out vec4 gCacheView_1;
layout(location = 1) out vec4 gCacheView_2;

void main() {
	int radianceCacheRad = int(pushConstant.v4.w);
	float chunkSize = pushConstant.v4_2.x;
	vec4 position = texelFetch(gPosition, ivec2(gl_FragCoord.xy), 0);
	vec3 normal = normalize(texelFetch(gNormal, ivec2(gl_FragCoord.xy), 0).xyz);
	int alpha = position.w==0 ? 0 : 1;

	gCacheView_1 = vec4(imageLoad(indirectCache_1, ivec3(floor(position.xyz*(radianceCacheRad/chunkSize))) % radianceCacheRad).rgb, 1);
	if(gCacheView_1.x + gCacheView_1.y + gCacheView_1.z == 0){
		gCacheView_1 = vec4(1,0,1,1);
	}

	if(alpha == 0){
		gCacheView_2 = vec4(0);
		return;
	}
	ivec3 readCoord = ivec3(floor(position.xyz*(radianceCacheRad/chunkSize))) % radianceCacheRad;
	float chunkHash = int(dot(ivec3(5,3,1), ivec3(ceil(position.xyz / chunkSize)))) + 0.5;
	vec4 cacheData = imageLoad(indirectCache_2, readCoord);
	if(chunkHash != cacheData.a){
		gCacheView_2 = vec4(0);
		return;
	}

	vec3 centerPos = vec3(floor(position.xyz / (chunkSize/radianceCacheRad)) * (chunkSize/radianceCacheRad) ) + 0.5 * (chunkSize/radianceCacheRad);
	vec3 offset = position.xyz - centerPos;
	offset = offset / (chunkSize/radianceCacheRad);
	vec4 cacheDataX = imageLoad(indirectCache_2, (readCoord + ivec3(sign(offset.x),0,0))%radianceCacheRad);
	vec4 cacheDataY = imageLoad(indirectCache_2, (readCoord + ivec3(0,sign(offset.y),0))%radianceCacheRad);
	vec4 cacheDataZ = imageLoad(indirectCache_2, (readCoord + ivec3(0,0,sign(offset.z)))%radianceCacheRad);
	vec4 cacheDataXY = imageLoad(indirectCache_2, (readCoord + ivec3(sign(offset.x),sign(offset.y),0))%radianceCacheRad);
	vec4 cacheDataXZ = imageLoad(indirectCache_2, (readCoord + ivec3(sign(offset.x),0,sign(offset.z)))%radianceCacheRad);
	vec4 cacheDataYZ = imageLoad(indirectCache_2, (readCoord + ivec3(0,sign(offset.y),sign(offset.z)))%radianceCacheRad);
	if(cacheDataX.a == cacheData.a && cacheDataY.a == cacheData.a){
		vec3 cX1 = (1-abs(offset.x)) * cacheData.rgb + abs(offset.x) * cacheDataX.rgb;
		vec3 cX2 = (1-abs(offset.x)) * cacheDataY.rgb + abs(offset.x) * cacheDataXY.rgb;
		vec3 cY = (1-abs(offset.y)) * cX1.rgb + abs(offset.y) * cX2.rgb;
		gCacheView_2 = vec4(cY, 1);
		return;
	}
	if(cacheDataX.a == cacheData.a && cacheDataZ.a == cacheData.a){
		vec3 cX1 = (1-abs(offset.x)) * cacheData.rgb + abs(offset.x) * cacheDataX.rgb;
		vec3 cX2 = (1-abs(offset.x)) * cacheDataZ.rgb + abs(offset.x) * cacheDataXZ.rgb;
		vec3 cZ = (1-abs(offset.z)) * cX1.rgb + abs(offset.z) * cX2.rgb;
		gCacheView_2 = vec4(cZ, 1);
		return;
	}
	if(cacheDataY.a == cacheData.a && cacheDataZ.a == cacheData.a){
		vec3 cY1 = (1-abs(offset.y)) * cacheData.rgb + abs(offset.y) * cacheDataY.rgb;
		vec3 cY2 = (1-abs(offset.y)) * cacheDataZ.rgb + abs(offset.y) * cacheDataYZ.rgb;
		vec3 cZ = (1-abs(offset.z)) * cY1.rgb + abs(offset.z) * cY2.rgb;
		gCacheView_2 = vec4(cZ, 1);
		return;
	}
	gCacheView_2 = vec4(cacheData.rgb, 1);
}