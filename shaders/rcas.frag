#version 460
#extension GL_ARB_separate_shader_objects : enable
#define inputTexture texSampler[0]
layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 FragColor;

layout(binding = 1) uniform sampler2D texSampler[190];
layout(push_constant) uniform constant
{
	vec4 v4; //sharpness
	mat4 m4;
} pushConstant;

float luma(vec3 v3){
    return (0.5*v3.r + v3.g + 0.5*v3.b);
}

void main(){       

    float sharpness = pushConstant.v4.x;

    float sharpnessStop = (1 - sharpness)*2.5;
    float sharpnessLinear = pow(2, -sharpnessStop);

    int a = int(floor(gl_FragCoord.x));
    int b = int(floor(gl_FragCoord.y));
    ivec2 pp = ivec2(a,b);

    vec3 cb = texelFetch(inputTexture, pp - ivec2(0, -1), 0).rgb;

    vec3 cd = texelFetch(inputTexture, pp - ivec2(-1, 0), 0).rgb;
    vec3 ce = texelFetch(inputTexture, pp - ivec2(0, 0), 0).rgb;
    vec3 cf = texelFetch(inputTexture, pp - ivec2(1, 0), 0).rgb;

    vec3 ch = texelFetch(inputTexture, pp - ivec2(0, 1), 0).rgb;

    float lb = luma(cb);
    float ld = luma(cd);
    float le = luma(ce);
    float lf = luma(cf);
    float lh = luma(ch);

    float max4R = max(max(max(cb.r, cd.r), cf.r), ch.r);
    float min4R = min(min(min(cb.r, cd.r), cf.r), ch.r);

    float max4G = max(max(max(cb.g, cd.g), cf.g), ch.g);
    float min4G = min(min(min(cb.g, cd.g), cf.g), ch.g);

    float max4B = max(max(max(cb.b, cd.b), cf.b), ch.b);
    float min4B = min(min(min(cb.b, cd.b), cf.b), ch.b);

    float wr = max(-min4R/(4*max4R), (1-max4R)/(4*min4R - 4));
    float wg = max(-min4G/(4*max4G), (1-max4G)/(4*min4G - 4));
    float wb = max(-min4B/(4*max4B), (1-max4B)/(4*min4B - 4));

    float w = max(-0.1875, min(max(max(wr, wg), wb), 0)) * sharpnessLinear;

    vec3 finalColor = (w*(cb + cd + cf + ch) + ce) / (4*w + 1);
    FragColor = vec4(finalColor, 1.0);
}