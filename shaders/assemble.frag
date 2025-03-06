#version 460
#extension GL_ARB_separate_shader_objects : enable
#define gTAAU texSampler[0]
#define gSky texSampler[1]
#define gForward texSampler[2]
layout(location = 0) in vec2 texCoord;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(location = 0) out vec4 outColor;

float gamma = 2.2;

void main(){
	vec4 TAAU = texture(gTAAU, texCoord);
	vec4 reinhardTAAU = vec4(TAAU.rgb/(TAAU.rgb + vec3(1)), TAAU.a);
	vec4 sky = texture(gSky, texCoord);
	vec4 forward = texture(gForward, texCoord);
	vec4 reinhardForward = vec4(forward.rgb/(forward.rgb + vec3(1)), forward.a);
	outColor = vec4(pow(mix(reinhardForward.rgb, mix(reinhardTAAU.rgb, sky.rgb, 1-reinhardTAAU.a ), 1-reinhardForward.a),vec3(1/gamma)),1);
}