#version 460
#extension GL_ARB_separate_shader_objects : enable
#define gPosition texSampler[0]
#define gNormal texSampler[1]
#define gAlbedo texSampler[2]
#define gHdrDirect texSampler[3]
#define gHdrIndirect texSampler[4]
layout(location = 0) in vec2 texCoord;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(push_constant) uniform constant
{
	vec4 v4; //innerW, innerH, rad, sig
	mat4 m4;
} pushConstant;

layout(location = 0) out vec4 gFilteredHdrDirect;
layout(location = 1) out vec4 gFilteredHdrIndirect;

float gamma = 2.2;

void main() {
    ivec2 innerWH = ivec2(pushConstant.v4.xy);
    int rad = int(pushConstant.v4.z);
    float roughness = texelFetch(gAlbedo, ivec2(gl_FragCoord.xy), 0).w;
	rad = int(round(rad * roughness));
    float sig = pushConstant.v4.w;
    vec4 position = texture(gPosition, texCoord);
    vec4 normal = texture(gNormal, texCoord);
    vec4 hdrDirect = texture(gHdrDirect, texCoord);
    vec4 hdrIndirect = texture(gHdrIndirect, texCoord);
    vec3 N = texelFetch(gNormal, ivec2(gl_FragCoord.xy), 0).xyz;
    vec4 filteredIndirect = vec4(0);
    vec4 filteredDirect = vec4(0);
    float gC1 = 0.01;
    float gC2 = 0.01;

    for(int i=-rad; i<=rad; i++){
		if(int(gl_FragCoord.x) + i < 0 || int(gl_FragCoord.x) + i > innerWH.x - 1){
			continue;
		}
		vec3 n = texelFetch(gNormal, ivec2(gl_FragCoord.xy) + ivec2(i,0), 0).rgb;
        vec3 p = texelFetch(gPosition, ivec2(gl_FragCoord.xy) + ivec2(i,0), 0).rgb;
		if(dot(N, n)<0.9 || distance(position.xyz, p)>0.5){
			continue;
		}
		float gauss = (1 / (sig * sqrt(2 * 3.14))) * exp(-i * i / (2 * sig * sig));
		filteredIndirect += gauss * texelFetch(gHdrIndirect, ivec2(gl_FragCoord.xy) + ivec2(i,0), 0);
		gC1 += gauss;
	}
	filteredIndirect /= gC1;
    gFilteredHdrIndirect = filteredIndirect;

   for(int x=-0; x<=0; x++){
        for(int y=-0; y<=0; y++){
            vec3 n = texelFetch(gNormal, ivec2(gl_FragCoord.xy) + ivec2(x,y), 0).xyz;
            vec3 p = texelFetch(gPosition, ivec2(gl_FragCoord.xy) + ivec2(x,y), 0).rgb;
            if(dot(N, n)<0.9 || distance(position.xyz, p)>0.5){
			    continue;
		    }
            float gaussX = (1 / (sig * sqrt(2 * 3.14))) * exp(-x * x / (2 * sig * sig));
            float gaussY = (1 / (sig * sqrt(2 * 3.14))) * exp(-y * y / (2 * sig * sig));
            gC2 += gaussX * gaussY;
            filteredDirect += gaussX * gaussY *texelFetch(gHdrDirect, ivec2(gl_FragCoord.xy) + ivec2(x,y), 0);
        }
    }
    filteredDirect /= gC2;
    gFilteredHdrDirect = filteredDirect;

}