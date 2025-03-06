#version 460
#extension GL_ARB_separate_shader_objects : enable
#define gPosition texSampler[0]
#define gNormal texSampler[1]
#define gAlbedo texSampler[2]
#define gPreFilteredHdrDirect texSampler[3]
#define gPreFilteredHdrIndirect texSampler[4]
layout(location = 0) in vec2 texCoord;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(push_constant) uniform constant
{
	vec4 v4; //innerW, innerH, rad, sig
	mat4 m4;
} pushConstant;

layout(location = 0) out vec4 gFilteredHdrDirectIndirect;

float gamma = 2.2;

void main() {
    ivec2 innerWH = ivec2(pushConstant.v4.xy);
    int rad = int(pushConstant.v4.z);
	float roughness = texelFetch(gAlbedo, ivec2(gl_FragCoord.xy), 0).w;
	rad = int(round(rad * roughness));
    float sig = pushConstant.v4.w;
    vec4 position = texelFetch(gPosition, ivec2(gl_FragCoord.xy), 0);
    vec4 normal = texture(gNormal, texCoord);
    vec4 gamma_albedo = vec4(pow(texture(gAlbedo, texCoord).xyz, vec3(gamma)), 1);
    vec3 N = texelFetch(gNormal, ivec2(gl_FragCoord.xy), 0).xyz;
    vec4 filteredRadiance = vec4(0);


    float gC = 0.01;
	for(int i=-rad; i<=rad; i++){
		if(int(gl_FragCoord.y) + i < 0 || int(gl_FragCoord.y) + i > innerWH.y - 1){
			continue;
		}
		vec3 n = texelFetch(gNormal, ivec2(gl_FragCoord.xy) + ivec2(0,i), 0).rgb;
		vec3 p = texelFetch(gPosition, ivec2(gl_FragCoord.xy) + ivec2(0,i), 0).rgb;
		if(dot(N, n)<0.9 || distance(position.xyz, p)>0.5){
			continue;
		}
		float gauss = (1 / (sig * sqrt(2 * 3.14))) * exp(-i * i / (2 * sig * sig));
		filteredRadiance += gauss * texelFetch(gPreFilteredHdrIndirect, ivec2(gl_FragCoord.xy) + ivec2(0,i), 0);
		gC += gauss;
	}
	filteredRadiance /= gC;

    filteredRadiance += vec4(texelFetch(gPreFilteredHdrDirect, ivec2(gl_FragCoord.xy), 0).rgb, 0); //addDirect
    filteredRadiance = filteredRadiance * gamma_albedo;
    gFilteredHdrDirectIndirect = filteredRadiance;
}