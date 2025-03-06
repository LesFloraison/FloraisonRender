#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_ray_query : require
#define gPosition texSampler[0]
#define gNormal texSampler[1]
#define gAlbedo texSampler[2]
#define gHistoricalPosition texSampler[3]
#define gHistoricalNormal texSampler[4]
#define gHistoricalDirect texSampler[5]
#define gHistoricalIndirect texSampler[6]
#define gCacheIndirect texSampler[7]
layout(location = 0) in vec2 texCoord;

layout(binding = 0) uniform UBO {
    vec3 cameraPos;
    mat4 historicalVP;
    float runingTime;
    float randSeed;
    int lightSize;
    int debugVal;
    vec3 lightInfos[50];
} ubo;

layout(push_constant) uniform constant
{
	vec4 v4; //innerW, innerH, ssp, ssp_2
	mat4 m4;
    vec4 v4_2; //radianceCacheRad, chunkSize, NEAR, FAR
} pushConstant;

layout(binding = 2) uniform samplerCube hdrSkybox[190];

layout(binding = 3, set = 0) uniform accelerationStructureEXT tlas;

layout(binding = 4, set = 0) buffer Vertices
{
  double vertices[];
};

layout(binding = 5, set = 0) buffer SSBO
{
  float sphereSamples[];
};

layout (binding = 6, rgba16) uniform image3D indirectCache_1;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(location = 0) out vec4 gHdrDirect;
layout(location = 1) out vec4 gHdrIndirect;

struct point{
    vec3 position;
    vec3 normal;
    vec3 albedo;
    vec3 arm;
};

vec3 directLighting(point shadingPoint);
vec3 indirectLighting(point shadingPoint, vec3 vpl_Position, vec3 vpl_radiance);
vec3 getDirLightVec(){
    return ubo.lightInfos[0];
}
vec3 getDirLightColor(){
    return ubo.lightInfos[1];
}
vec3 getPointLightPos(int index){
    return ubo.lightInfos[2 + 4*index + 0];
}
vec3 getPointLightScale(int index){
    return ubo.lightInfos[2 + 4*index + 1];
}
vec3 getPointLightColor(int index){
    return ubo.lightInfos[2 + 4*index + 2];
}
float getPointLightMaxRad(int index){
    return ubo.lightInfos[2 + 4*index + 3].x;
}
vec3 semiSphereSample(int index){
    int m_index = index%100;
    return vec3(sphereSamples[3*m_index + 0], sphereSamples[3*m_index + 1], sphereSamples[3*m_index + 2]);
}

vec3 sphereSample(int index){
    int m_index = (128 + index%50);
    return vec3(sphereSamples[3*m_index + 0], sphereSamples[3*m_index + 1], sphereSamples[3*m_index + 2]);
}

int getVertex(int goemetryID, int primitiveID){
    return int(vertices[goemetryID]+0.1) + 3*11*primitiveID;
}

mat4 rotate(float x, float y, float z, float angle){
 float cos0 = cos(-angle);
 float sin0 = sin(-angle);
 float t = 1.0f - cos0;
 float txx = t * x * x;
 float txy = t * x * y;
 float txz = t * x * z;
 float tyy = t * y * y;
 float tyz = t * y * z;
 float tzz = t * z * z;
 float sinx = sin0 * x;
 float siny = sin0 * y;
 float sinz = sin0 * z;
 
 return mat4(
 txx+cos0,txy-sinz,txz+siny,0,
 txy+sinz,tyy+cos0,tyz-sinx,0,
 txz-siny,tyz+sinx,tzz+cos0,0,
 0,0,0,1
 );
 }

point voidPoint;
float gamma = 2.2;

int occlusionTest(vec3 origin, vec3 direction, float tRange);
point reflectTest(vec3 origin, vec3 direction);
int reflectOcculusionTest(vec3 origin, vec3 direction);
vec4 temporalBlendDirect(vec3 Lo, point position, sampler2D gTarget);
vec4 temporalBlendIndirect(vec3 Lo, point position, sampler2D gTarget);

ivec2 innerWH;
int ssp, ssp_2;
int radianceCacheRad;
float chunkSize;
float NEAR, FAR;
float roughness;

void main() {
    voidPoint.position = vec3(0);
    voidPoint.normal = vec3(0);
    voidPoint.albedo = vec3(0);
    voidPoint.arm = vec3(0);
    ssp = int(pushConstant.v4.z);
    ssp_2 = int(pushConstant.v4.w);
    radianceCacheRad = int(pushConstant.v4_2.x);
    chunkSize = pushConstant.v4_2.y;
    NEAR = pushConstant.v4_2.z;
    FAR = pushConstant.v4_2.w;
    roughness = texelFetch(gAlbedo, ivec2(gl_FragCoord.xy), 0).w;

    innerWH = ivec2(pushConstant.v4.xy);
    int alpha = int(texelFetch(gPosition, ivec2(gl_FragCoord.xy), 0).a + 0.1);
    vec3 gamma_albedo = pow(texture(gAlbedo, texCoord).xyz, vec3(gamma));
    vec3 position = texelFetch(gPosition, ivec2(gl_FragCoord.xy), 0).xyz;
    vec3 normal = texelFetch(gNormal, ivec2(gl_FragCoord.xy), 0).xyz;

    point shadingPoint;
    shadingPoint.position = position;
    shadingPoint.normal = normal;
    shadingPoint.albedo = gamma_albedo;
    vec3 V = normalize(ubo.cameraPos - shadingPoint.position);
    vec3 R = normalize(reflect(-V,normal));

    vec3 tanget = normalize(cross(normal,vec3(0,1,0)));
    tanget = abs(dot(normal,vec3(0,1,0))) < 0.7 ? normalize(cross(normal,vec3(0,1,0))) : normalize(cross(normal,vec3(1,0,0)));
    vec3 bitanget = normalize(cross(normal,tanget));
    mat3 TBN = mat3(tanget,bitanget,normal);

    vec3 radiance = vec3(0);
    for(int i = 0; i<ssp; i++){
        int randIndex = int(1000*fract(100*sin(dot(texCoord.xy,vec2(ubo.randSeed + i,27.1437)))));
        float rand = fract(1000*sin(dot(texCoord.xy,vec2(12.9898, ubo.randSeed + i))));
        mat4 randRot = rotate(0,0,1,rand*90);

        vec3 sampleVec = normalize(TBN * vec3(randRot* vec4(semiSphereSample(randIndex),1)));
        sampleVec = normalize((1-roughness)*R*10 + sampleVec*roughness);
        point p = reflectTest(shadingPoint.position, sampleVec);
        if(p == voidPoint){
            vec3 N = normalize(shadingPoint.normal);
            vec3 L = normalize(sampleVec);
            vec3 sky_radiance = texture(hdrSkybox[0],normalize(vec3(sampleVec))).rgb;
            float NdotL = max(dot(N, L), 0);
            radiance += sky_radiance * NdotL;
        }else{

            vec3 tanget2 = normalize(cross(p.normal,vec3(0,1,0)));
            tanget2 = abs(dot(p.normal,vec3(0,1,0))) < 0.7 ? normalize(cross(p.normal,vec3(0,1,0))) : normalize(cross(p.normal,vec3(1,0,0)));
            vec3 bitanget2 = normalize(cross(p.normal,tanget2));
            mat3 TBN2 = mat3(tanget2,bitanget2,p.normal);

            if(roughness < 0.95){
                vec3 vpl_radiance = directLighting(p);
                radiance += indirectLighting(shadingPoint, p.position, vpl_radiance);
                for(int j=0; j<4; j++){
                    int randIndex_2 = int(1000*fract(100*sin(dot(texCoord.xy,vec2(ubo.randSeed + i,27.1437 + j)))));
                    float rand_2 = fract(1000*sin(dot(texCoord.xy,vec2(12.9898 + j, ubo.randSeed + i))));
                    mat4 randRot_2 = rotate(0,0,1,rand_2*90);
                    vec3 sampleVec2 = TBN2 * vec3(randRot_2* vec4(semiSphereSample(randIndex_2),1));
                    sampleVec2 = normalize(sampleVec2);
                    if(reflectOcculusionTest(p.position, sampleVec2) == 0){
                        vec3 N2 = normalize(p.normal);
                        vec3 L2 = normalize(sampleVec2);
                        vec3 sky_radiance2 = texture(hdrSkybox[0],normalize(vec3(sampleVec2))).rgb;
                        float NdotL2 = max(dot(N2, L2), 0);
                        radiance += sky_radiance2 * p.albedo *NdotL2 / 4;
                    }
                }
                continue;
            }


            ivec3 opCoord = ivec3(floor(p.position*(radianceCacheRad/chunkSize))) % radianceCacheRad;
            vec4 cacheData = imageLoad(indirectCache_1, opCoord);
            float chunkHash = int(dot(ivec3(5,3,1), ivec3(ceil(p.position / chunkSize)))) + 0.5;
            int updateFactor = int(1000*fract(1000*sin(dot(texCoord.xy,vec2(ubo.randSeed, 27.1437)))));
            if(chunkHash == cacheData.a && (updateFactor<999 || ubo.debugVal == 0)){
                radiance += indirectLighting(shadingPoint, p.position, cacheData.xyz);
                continue;
            }

            vec3 radiance_2 = vec3(0);
            radiance_2 += directLighting(p);

            for(int j =0; j<ssp_2; j++){
                int randIndex_2 = int(1000*fract(100*sin(dot(texCoord.xy,vec2(ubo.randSeed + i,27.1437 + j)))));
                float rand_2 = fract(1000*sin(dot(texCoord.xy,vec2(12.9898 + j, ubo.randSeed + i))));
                mat4 randRot_2 = rotate(0,0,1,rand_2*90);

                vec3 sampleVec_2 = TBN2 * vec3(randRot_2* vec4(semiSphereSample(randIndex_2),1));
                sampleVec_2 = normalize(sampleVec_2);
                point p_2 = reflectTest(p.position, sampleVec_2);
                if(p_2 == voidPoint){
                    vec3 N2 = normalize(p.normal);
                    vec3 L2 = normalize(sampleVec_2);
                    vec3 sky_radiance2 = texture(hdrSkybox[0],normalize(vec3(sampleVec_2))).rgb;
                    float NdotL2 = max(dot(N2, L2), 0);
                    radiance_2 += sky_radiance2 * p.albedo * NdotL2 / ssp_2;
                }else{
                    ivec3 opCoord_2 = ivec3(floor(p_2.position*(radianceCacheRad/chunkSize))) % radianceCacheRad;
                    vec4 cacheData_2 = imageLoad(indirectCache_1, opCoord_2);
                    if(ubo.debugVal == 1){
                        radiance_2 += indirectLighting(p, p_2.position, cacheData_2.xyz) / ssp_2;
                    }
                }
            }
            radiance += indirectLighting(shadingPoint, p.position, radiance_2);
            imageStore(indirectCache_1, opCoord, vec4(radiance_2, chunkHash));
        }
    }
    radiance = radiance / ssp;
    shadingPoint.albedo = vec3(1);
    gHdrDirect = temporalBlendDirect(directLighting(shadingPoint), shadingPoint, gHistoricalDirect);
    gHdrIndirect = temporalBlendIndirect(radiance, shadingPoint, gHistoricalIndirect);
}

vec3 directLighting(point shadingPoint){
    vec3 result = vec3(0);
    vec3 N = normalize(shadingPoint.normal);
    vec3 L = normalize(-getDirLightVec());
    vec3 radiance = getDirLightColor();
    float NdotL = max(dot(N, L), 0);
    if(NdotL != 0){
        if(occlusionTest(shadingPoint.position, -getDirLightVec(), 9999) == 0){
            result += radiance * NdotL * shadingPoint.albedo;
        }  
    }

    for(int i = 0; i<ubo.lightSize; i++){
        if(length(shadingPoint.position - getPointLightPos(i)) > getPointLightMaxRad(i)){
            continue;
        }
        int randIndex = int(1000*fract(100*sin(dot(texCoord.xy,vec2(ubo.randSeed,27.1437)))));
        float rand = fract(1000*sin(dot(texCoord.xy,vec2(12.9898, ubo.randSeed))));
        mat4 randRot = rotate(0,1,0,rand*90);

        vec3 biasVec = getPointLightScale(i) * vec3(randRot* vec4(sphereSample(randIndex),1));

        vec3 lightPosBlas = getPointLightPos(i) + biasVec;
        float m_blasDistance = length(lightPosBlas - shadingPoint.position);

        vec3 L = normalize(lightPosBlas - shadingPoint.position);
        float attenuation = 1.0 / (m_blasDistance * m_blasDistance + 1);
        vec3 radiance = getPointLightColor(i) * attenuation;
        float NdotL = max(dot(N, L), 0);
        if(NdotL == 0){
            continue;
        }


        if(occlusionTest(shadingPoint.position, lightPosBlas - shadingPoint.position, m_blasDistance) == 0){
            result += radiance * NdotL * shadingPoint.albedo;
        }
    }

    return result;
}

vec3 indirectLighting(point shadingPoint, vec3 vpl_Position, vec3 vpl_radiance){
    vec3 result = vec3(0);
    vec3 N = normalize(shadingPoint.normal);
    vec3 L = normalize(vpl_Position  - shadingPoint.position);
    float NdotL = max(dot(N, L), 0);
    result += vpl_radiance * NdotL;
    return result;
}

int occlusionTest(vec3 origin, vec3 direction, float tRange){
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, tlas, gl_RayFlagsOpaqueEXT | gl_RayFlagsCullFrontFacingTrianglesEXT, 0xFF, origin, 0.0, normalize(direction), tRange);
    rayQueryProceedEXT(rayQuery);

    if(rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT){
        return 0;
    }else{
        return 1;
    }
}

point reflectTest(vec3 origin, vec3 direction){
    point crossPoint;
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, tlas, gl_RayFlagsOpaqueEXT | gl_RayFlagsCullFrontFacingTrianglesEXT, 0xFF, origin, 0.0, normalize(direction), 10000);
    rayQueryProceedEXT(rayQuery);
    vec2 barycenter = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
    int geometryID = rayQueryGetIntersectionInstanceIdEXT(rayQuery, true);
    int primitiveID = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
    mat4x3 M = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
    if(rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT){
        return voidPoint;
    }

    vec3 v0WPos = M * vec4(vertices[getVertex(geometryID, primitiveID) + 0], vertices[getVertex(geometryID, primitiveID) + 1], vertices[getVertex(geometryID, primitiveID) + 2], 1);
    vec3 v1WPos = M * vec4(vertices[getVertex(geometryID, primitiveID) + 11 + 0], vertices[getVertex(geometryID, primitiveID) + 11 + 1], vertices[getVertex(geometryID, primitiveID) + 11 + 2], 1);
    vec3 v2WPos = M * vec4(vertices[getVertex(geometryID, primitiveID) + 22 + 0], vertices[getVertex(geometryID, primitiveID) + 22 + 1], vertices[getVertex(geometryID, primitiveID) + 22 + 2], 1);

    vec3 WPos = barycenter.x*v1WPos + barycenter.y*v2WPos + (1-barycenter.x-barycenter.y)*v0WPos;

    vec3 v0Normal = normalize(vec3(vertices[getVertex(geometryID, primitiveID) + 8],vertices[getVertex(geometryID, primitiveID)+9],vertices[getVertex(geometryID, primitiveID)+10]));
    vec3 v1Normal = normalize(vec3(vertices[getVertex(geometryID, primitiveID) + 11 +8],vertices[getVertex(geometryID, primitiveID)+ 11 +9],vertices[getVertex(geometryID, primitiveID)+ 11 +10]));
    vec3 v2Normal = normalize(vec3(vertices[getVertex(geometryID, primitiveID) + 22 +8],vertices[getVertex(geometryID, primitiveID)+ 22 +9],vertices[getVertex(geometryID, primitiveID)+ 22 +10]));

    vec3 compNormal = barycenter.x*v1Normal + barycenter.y*v2Normal + (1-barycenter.x-barycenter.y)*v0Normal;

    vec2 v0TC = vec2(vertices[getVertex(geometryID, primitiveID) + 3], vertices[getVertex(geometryID, primitiveID) + 4]);
    vec2 v1TC = vec2(vertices[getVertex(geometryID, primitiveID) + 11 + 3], vertices[getVertex(geometryID, primitiveID) + 11 + 4]);
    vec2 v2TC = vec2(vertices[getVertex(geometryID, primitiveID) + 22 + 3], vertices[getVertex(geometryID, primitiveID) + 22 + 4]);

    vec2 TC = barycenter.x*v1TC + barycenter.y*v2TC + (1-barycenter.x-barycenter.y)*v0TC;

    crossPoint.position = WPos;
    crossPoint.normal = compNormal;
    crossPoint.albedo = pow(texture(texSampler[8 + int(vertices[getVertex(geometryID, primitiveID) + 5] + 0.1)], TC).rgb, vec3(gamma));
    crossPoint.arm = vec3(0);
    return crossPoint;
}

int reflectOcculusionTest(vec3 origin, vec3 direction){
    point crossPoint;
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery, tlas, gl_RayFlagsOpaqueEXT | gl_RayFlagsCullFrontFacingTrianglesEXT, 0xFF, origin, 0.0, normalize(direction), 10000);
    rayQueryProceedEXT(rayQuery);
    vec2 barycenter = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
    int geometryID = rayQueryGetIntersectionInstanceIdEXT(rayQuery, true);
    int primitiveID = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
    mat4x3 M = rayQueryGetIntersectionObjectToWorldEXT(rayQuery, true);
    if(rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT){
        return 0;
    }
    return 1;
}

vec4 temporalBlendDirect(vec3 Lo, point p, sampler2D gTarget){
    vec2 historicalUV = (((ubo.historicalVP * vec4(p.position,1)).xy / (ubo.historicalVP * vec4(p.position,1)).w)) * 0.5 + 0.5;
    vec3 historicalPosition = texelFetch(gHistoricalPosition, ivec2(historicalUV * innerWH * 2), 0).rgb;
    vec3 historicalNormal = texelFetch(gHistoricalNormal, ivec2(historicalUV * innerWH * 2), 0).rgb;
    float historicalZ = (((ubo.historicalVP * vec4(historicalPosition,1)).z / (ubo.historicalVP * vec4(historicalPosition,1)).w));
    historicalZ = (2.0 * NEAR * FAR) / (FAR + NEAR - historicalZ * (FAR - NEAR));
    float Z = (((ubo.historicalVP * vec4(p.position,1)).z / (ubo.historicalVP * vec4(p.position,1)).w));
    Z = (2.0 * NEAR * FAR) / (FAR + NEAR - Z * (FAR - NEAR));
    vec3 historicalLo = texelFetch(gTarget, ivec2(historicalUV * innerWH), 0).rgb;
    if(dot(historicalNormal, p.normal)<0.9 || abs(historicalZ - Z)>1 || historicalUV.x>1 || historicalUV.x<0 ||historicalUV.y>1 ||historicalUV.y<0){
        return vec4(Lo, 0);
    }
    vec3 blendLo;
    blendLo = historicalLo*(0.9) + Lo*(0.1);
    return vec4(blendLo, 1);
}


vec4 temporalBlendIndirect(vec3 Lo, point p, sampler2D gTarget){
    vec2 historicalUV = (((ubo.historicalVP * vec4(p.position,1)).xy / (ubo.historicalVP * vec4(p.position,1)).w)) * 0.5 + 0.5;
    vec3 historicalPosition = texelFetch(gHistoricalPosition, ivec2(historicalUV * innerWH * 2), 0).rgb;
    vec3 historicalNormal = texelFetch(gHistoricalNormal, ivec2(historicalUV * innerWH * 2), 0).rgb;
    float historicalZ = (((ubo.historicalVP * vec4(historicalPosition,1)).z / (ubo.historicalVP * vec4(historicalPosition,1)).w));
    historicalZ = (2.0 * NEAR * FAR) / (FAR + NEAR - historicalZ * (FAR - NEAR));
    float Z = (((ubo.historicalVP * vec4(p.position,1)).z / (ubo.historicalVP * vec4(p.position,1)).w));
    Z = (2.0 * NEAR * FAR) / (FAR + NEAR - Z * (FAR - NEAR));
    vec3 historicalLo = texelFetch(gTarget, ivec2(historicalUV * innerWH), 0).rgb;
    int historicalAccu = int(texelFetch(gTarget, ivec2(historicalUV * innerWH), 0).w);
    vec4 cacheData = texelFetch(gCacheIndirect, ivec2(gl_FragCoord.xy), 0);
    int cacheHit = cacheData.a==0 ? 0 : 1;
    if((dot(historicalNormal, p.normal)<0.9 || abs(historicalZ - Z)>1 || historicalUV.x>1 || historicalUV.x<0 ||historicalUV.y>1 ||historicalUV.y<0) && cacheHit == 0){
        return vec4(Lo, 0);
    }
    if((dot(historicalNormal, p.normal)<0.9 || abs(historicalZ - Z)>1 || historicalUV.x>1 || historicalUV.x<0 ||historicalUV.y>1 ||historicalUV.y<0) && cacheHit == 1){
        if(roughness < 0.95){
            return vec4(Lo, 0);
        }
        return vec4(cacheData.rgb, 20);
    }
    historicalAccu = historicalAccu == 30 ? 30 : (historicalAccu+1);
    historicalAccu = min(historicalAccu, (pushConstant.m4 == mat4(0) ? historicalAccu : int(round(roughness*30))));
    vec3 blendLo;
    blendLo = historicalLo*(historicalAccu/30.4) + Lo*(1-historicalAccu/30.4);
    return vec4(blendLo, historicalAccu);
}
