#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_ray_query : require
#define gPosition texSampler[0]
#define gScreen texSampler[1]
layout(location = 0) in BLOCK_IN{
    in vec4 fragWPos;
    in vec2 texCoord;
    in vec3 textureIndex;
} fs_in;

layout(push_constant) uniform constant
{
	vec4 v4; //vec2 warpScale_1, vec2 warpScale_2
	mat4 m4;//M
    vec4 v4_2; //vec2 flow_1, vec2 flow_2
    mat4 m4_2;//VP
} pushConstant;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(binding = 2) uniform samplerCube hdrSkybox[190];

layout(binding = 3, set = 0) uniform accelerationStructureEXT tlas;

layout(binding = 4, set = 0) buffer Vertices
{
  double vertices[];
};

layout(binding = 0) uniform UBO {
    vec3 cameraPos;
    mat4 historicalVP;
    float runingTime;
    float randSeed;
    int lightSize;
    int debugVal;
    vec3 lightInfos[50];
} ubo;

layout(location = 0) out vec4 gForward;

vec4 rayTest(vec3 origin, vec3 direction);

void main() {
    float time = ubo.runingTime;
    vec2 warpScale_1 = vec2(pushConstant.v4.x, pushConstant.v4.y);
    vec2 warpScale_2 = vec2(pushConstant.v4.z, pushConstant.v4.w);
    vec2 flow_1 = vec2(pushConstant.v4_2.x, pushConstant.v4_2.y);
    vec2 flow_2 = vec2(pushConstant.v4_2.z, pushConstant.v4_2.w);
    vec4 gN_1 = vec4(texture(texSampler[int(fs_in.textureIndex.x+0.1) + 2], fs_in.texCoord*warpScale_1 + time*flow_1).rgb,1);
    vec3 N_1 = normalize(((gN_1.rbg)*2-1));
    vec4 gN_2 = vec4(texture(texSampler[int(fs_in.textureIndex.x+0.1) + 2], fs_in.texCoord*warpScale_2 + time*flow_2).rgb,1);
    vec3 N_2 = normalize(((gN_2.rbg)*2-1));
    vec3 N = normalize(N_1+N_2);
    //N= vec3 (0,1,0);
    vec3 incident = normalize(fs_in.fragWPos.xyz - ubo.cameraPos);
    float mixComp = 1 - max(dot(-incident, N), 0);

    vec3 Ra = normalize(refract(incident, N, 0.9));
    vec4 raCrossPos = rayTest(fs_in.fragWPos.xyz, Ra);
    vec2 raUV = (((pushConstant.m4_2 * vec4(raCrossPos.xyz,1)).xy / (pushConstant.m4_2 * vec4(raCrossPos.xyz,1)).w)) * 0.5 + 0.5;
    vec3 scRaPos = texture(gPosition, raUV).xyz;
    vec3 raColor = texture(gScreen, raUV).rgb;
    float alpha = texture(gPosition, (((pushConstant.m4_2 * vec4(fs_in.fragWPos.xyz,1)).xy / (pushConstant.m4_2 * vec4(fs_in.fragWPos.xyz,1)).w)) * 0.5 + 0.5).r;
    vec3 scr = texture(gScreen, (((pushConstant.m4_2 * vec4(fs_in.fragWPos.xyz,1)).xy / (pushConstant.m4_2 * vec4(fs_in.fragWPos.xyz,1)).w)) * 0.5 + 0.5).rgb;
    vec3 sky = texture(hdrSkybox[0], Ra).rgb / (texture(hdrSkybox[0], Ra).rgb + 1);
    if(distance(raCrossPos.xyz, scRaPos) > 0.1 && raCrossPos.a == 1){
        raColor = alpha == 0 ? sky : scr.rgb;
    }
    if(raUV.x<0 || raUV.y<0 || raUV.x>1 || raUV.y>1){
        raColor = alpha == 0 ? sky : scr.rgb;
    }
    if(raCrossPos.a == 0){
        raColor = sky;
    }

    vec3 Re = normalize(reflect(incident, N));
    vec4 reCrossPos = rayTest(fs_in.fragWPos.xyz, Re);
    vec2 reUV = (((pushConstant.m4_2 * vec4(reCrossPos.xyz,1)).xy / (pushConstant.m4_2 * vec4(reCrossPos.xyz,1)).w)) * 0.5 + 0.5;
    vec3 scRePos = texture(gPosition, reUV).xyz;
    vec3 reColor = texture(gScreen, reUV).rgb;
    if(distance(reCrossPos.xyz, scRePos) > 0.1 && reCrossPos.a == 1){ //wrong Z
        mixComp = 0;
    }
    if((reUV.x<0 || reUV.y<0 || reUV.x>1 || reUV.y>1) && reCrossPos.a == 1){ //out of screen
        mixComp = 0;
    }
    if(reCrossPos.a == 0){
        vec3 sky = texture(hdrSkybox[0], Re).rgb;
        reColor = sky / (sky + vec3(1.0));
    }
    
    vec3 rMix = mix(raColor, reColor, mixComp);
    gForward = vec4(rMix,1);
}

int getVertex(int goemetryID, int primitiveID){
    return int(vertices[goemetryID]+0.1) + 3*11*primitiveID;
}

vec4 rayTest(vec3 origin, vec3 direction){
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, tlas, gl_RayFlagsOpaqueEXT | gl_RayFlagsCullFrontFacingTrianglesEXT, 0xFF, origin, 0.0, normalize(direction), 10000);
    rayQueryProceedEXT(rayQuery);
    vec2 barycenter = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
    int geometryID = rayQueryGetIntersectionInstanceIdEXT(rayQuery, true);
    int primitiveID = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
    mat4x3 M = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
    if(rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT){
        return vec4(0);
    }

    vec3 v0WPos = M * vec4(vertices[getVertex(geometryID, primitiveID) + 0], vertices[getVertex(geometryID, primitiveID) + 1], vertices[getVertex(geometryID, primitiveID) + 2], 1);
    vec3 v1WPos = M * vec4(vertices[getVertex(geometryID, primitiveID) + 11 + 0], vertices[getVertex(geometryID, primitiveID) + 11 + 1], vertices[getVertex(geometryID, primitiveID) + 11 + 2], 1);
    vec3 v2WPos = M * vec4(vertices[getVertex(geometryID, primitiveID) + 22 + 0], vertices[getVertex(geometryID, primitiveID) + 22 + 1], vertices[getVertex(geometryID, primitiveID) + 22 + 2], 1);

    vec3 WPos = barycenter.x*v1WPos + barycenter.y*v2WPos + (1-barycenter.x-barycenter.y)*v0WPos;
    return vec4(WPos, 1);
}