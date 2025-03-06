#version 460
#extension GL_ARB_separate_shader_objects : enable
#define gPosition texSampler[0]
#define gNormal texSampler[1]
#define gAlbedo texSampler[2]
#define gSky texSampler[3]
#define gUILayer texSampler[4]
#define gHdrDirectIndirect texSampler[5]
#define gEASU texSampler[6]
#define gRCAS texSampler[7]
#define gTAAU texSampler[8]
#define gIndirectCache_1 texSampler[11]
#define gIndirectCache_2 texSampler[12]
#define gForward texSampler[15]
#define gInterface texSampler[16]
#define gAssemble texSampler[17]
#define gFont texSampler[18]
layout(location = 0) in vec2 texCoord;

layout(push_constant) uniform constant
{
	vec4 v4;
	mat4 m4;
} pushConstant;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(location = 0) out vec4 outColor;

float gamma = 2.2;

void main() {
    int displayID = int(pushConstant.v4.x);
    int UIEnable = int(pushConstant.v4.y);
    int maxDisplayID = int(pushConstant.v4.z);
    vec4 position = texture(gPosition, texCoord);
    vec4 albedo = texture(gAlbedo, texCoord);
    vec4 normal = texture(gNormal, texCoord);
    vec4 sky = texture(gSky, texCoord);
    vec4 UILayer = texture(gUILayer, texCoord);
    vec4 hdrDirectIndirect = texture(gHdrDirectIndirect, texCoord);
    vec3 directIndirect = (hdrDirectIndirect.rgb) / ( hdrDirectIndirect.rgb + vec3(1.0));
    vec3 EASU = texture(gEASU, texCoord).rgb;
    vec3 RCAS = texture(gRCAS, texCoord).rgb;
    vec4 TAAU = texture(gTAAU, texCoord);
    vec4 forward = texture(gForward, texCoord);
    vec4 m_interface = texture(gInterface, texCoord);
    vec4 assemble = texture(gAssemble, texCoord);
    vec4 font = texture(gFont, texCoord);
    TAAU.rgb = TAAU.rgb/(TAAU.rgb+vec3(1.0));
    outColor = vec4(pow(mix(directIndirect, sky.rgb, 1-position.a ),vec3(1/gamma)),1);
    if(UIEnable == 1){
        outColor = vec4(mix(UILayer.rgb, outColor.rgb, 1-UILayer.a ),1);
    }
    if(displayID >= 0 && displayID < maxDisplayID){
        outColor = texture(texSampler[displayID],texCoord);
    }
    if(displayID == 6){
        outColor = vec4(pow(mix(EASU, sky.rgb, 1-position.a ),vec3(1/gamma)),1);
    }
    if(displayID == 8){
        //outColor = vec4(pow(mix(TAAU.rgb, sky.rgb, 1-TAAU.a ),vec3(1/gamma)),1);
        outColor = vec4(pow(mix(forward.rgb, mix(TAAU.rgb, sky.rgb, 1-TAAU.a ), (1-forward.a)>0?1:0),vec3(1/gamma)),1);
    }
    if(displayID == 15){
        outColor = assemble;
    }
    if(displayID == 16){
        //outColor = vec4(mix(pow(mix(forward.rgb, mix(TAAU.rgb, sky.rgb, 1-TAAU.a ), 1-forward.a),vec3(1/gamma)), m_interface.rgb, m_interface.a),1);
        //outColor = vec4(mix(RCAS, m_interface.rgb, m_interface.a),1);
        outColor = vec4(mix(mix(RCAS, m_interface.rgb, m_interface.a), font.rgb, font.a),1);
        //outColor = vec4(m_interface.rgb,1);
    }
    if(displayID == 17){
        outColor = vec4(EASU, 1);
    }
    //if(displayID == 18){
    //    outColor = vec4(RCAS, 1);
    //}
}