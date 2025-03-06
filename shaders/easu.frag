#version 460
#extension GL_ARB_separate_shader_objects : enable
#define inputTexture texSampler[0]
layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4  FragColor;

layout(binding = 1) uniform sampler2D texSampler[190];

layout(push_constant) uniform constant
{
	vec4 v4; //inputWidth/outputWidth, inputHeight/outputHeight
	mat4 m4;
} pushConstant;

float luma(vec3 v3){
    return (0.5*v3.r + v3.g + 0.5*v3.b);
}

float wei12(vec2 p12, vec2 pp, vec2 dir, vec2 scale, float lob, float clp){
    vec2 offsetDir = p12 - pp;
    vec2 rotDir;
    rotDir.x = offsetDir.x*dir.x + offsetDir.y*dir.y;
    rotDir.y = (-offsetDir.x*dir.y) + offsetDir.y*dir.x;
    vec2 scaleDir = scale * rotDir;
    float dis2 = scaleDir.x*scaleDir.x + scaleDir.y*scaleDir.y;
    float x2 = min(dis2, clp);
    float w = ((25.0/16.0)*(2.0/5.0*x2  - 1)*(2.0/5.0*x2  - 1) - 25.0/16.0 + 1) * (lob*x2 - 1)*(lob*x2 - 1);
    return w;
}


void main(){
    vec2 ido = vec2(pushConstant.v4.x, pushConstant.v4.y);
    int a = int(floor(gl_FragCoord.x));
    int b = int(floor(gl_FragCoord.y));
    vec2 pp = ido * vec2(a,b);
    ivec2 Fpos = ivec2(floor(pp));
    pp = pp - Fpos;
    float bwf = (1.0f-pp.x) * (1.0f-pp.y);
    float bwg = pp.x * (1-pp.y);
    float bwj = (1-pp.x) * pp.y;
    float bwk = pp.x * pp.y;

    vec3 cb = texelFetch(inputTexture,Fpos+ivec2(0,-1),0).rgb;
    vec3 cc = texelFetch(inputTexture,Fpos+ivec2(1,-1),0).rgb;

    vec3 ce = texelFetch(inputTexture,Fpos+ivec2(-1,0),0).rgb;
    vec3 cf = texelFetch(inputTexture,Fpos+ivec2(0,0),0).rgb;
    vec3 cg = texelFetch(inputTexture,Fpos+ivec2(1,0),0).rgb;
    vec3 ch = texelFetch(inputTexture,Fpos+ivec2(2,0),0).rgb;

    vec3 ci = texelFetch(inputTexture,Fpos+ivec2(-1,1),0).rgb;
    vec3 cj = texelFetch(inputTexture,Fpos+ivec2(0,1),0).rgb;
    vec3 ck = texelFetch(inputTexture,Fpos+ivec2(1,1),0).rgb;
    vec3 cl = texelFetch(inputTexture,Fpos+ivec2(2,1),0).rgb;

    vec3 cn = texelFetch(inputTexture,Fpos+ivec2(0,2),0).rgb;
    vec3 co = texelFetch(inputTexture,Fpos+ivec2(1,2),0).rgb;

    float lb = luma(cb);
    float lc = luma(cc);
    float le = luma(ce);
    float lf = luma(cf);
    float lg = luma(cg);
    float lh = luma(ch);
    float li = luma(ci);
    float lj = luma(cj);
    float lk = luma(ck);
    float ll = luma(cl);
    float ln = luma(cn);
    float lo = luma(co);

    vec2 dirf = vec2(lg - le, lj - lb);
    vec2 dirg = vec2(lh - lf, lk - lc);
    vec2 dirj = vec2(lk - li, ln - lf);
    vec2 dirk = vec2(ll - lj, lo - lg);

    vec2 dir = vec2(bwf*dirf + bwg*dirg + bwj*dirj + bwk*dirk);

    vec2 dir2 = dir*dir;
    float dirR = dir2.x + dir2.y;
    int isZro = dirR < (1.0/32678.0) ? 1 : 0;
    dirR = 1.0/sqrt(dirR);
    dirR = (isZro == 1 ? 1.0 : dirR);
    vec2 dirFinal = dir;
    dirFinal.x = (isZro == 1 ? 1.0 : dirFinal.x);
    dirFinal = dirFinal * dirR; 



    float lenfX = clamp(abs(lg - le)/max(abs(lf - lg), abs(lf - le)), 0 ,1);
    float lengX = clamp(abs(lh - lf)/max(abs(lg - lh), abs(lg - lf)), 0 ,1);
    float lenjX = clamp(abs(lk - li)/max(abs(lj - lk), abs(lj - li)), 0 ,1);
    float lenkX = clamp(abs(ll - lj)/max(abs(lk - ll), abs(lk - lj)), 0 ,1);

    float lenfY = clamp(abs(lj - lb)/max(abs(lf - lj), abs(lf - lb)), 0 ,1);
    float lengY = clamp(abs(lk - lc)/max(abs(lg - lk), abs(lg - lc)), 0 ,1);
    float lenjY = clamp(abs(ln - lf)/max(abs(lj - ln), abs(lj - lf)), 0 ,1);
    float lenkY = clamp(abs(lo - lg)/max(abs(lk - lo), abs(lk - lg)), 0 ,1);

    float len2f = lenfX*lenfX + lenfY*lenfY;
    float len2g = lengX*lengX + lengY*lengY;
    float len2j = lenjX*lenjX + lenjY*lenjY;
    float len2k = lenkX*lenkX + lenkY*lenkY;

    float lenFinal = bwf*len2f + bwg*len2g + bwj*len2j + bwk*len2k;
    lenFinal = (lenFinal/2.0) * (lenFinal/2.0);


    float stretch = (1.0 / max(abs(dirFinal.x), abs(dirFinal.y)));
    float scaleX = 1 + (stretch - 1)*lenFinal;
    float scaleY = 1 - 0.5*lenFinal;
    vec2 scale = vec2(scaleX, scaleY);

    float lob = 0.5 - 0.254*lenFinal;
    float clp = 1/lob;

    float wb = wei12(ivec2(0,-1), pp, dirFinal, scale, lob, clp);
    float wc = wei12(ivec2(1,-1), pp, dirFinal, scale, lob, clp);

    float we = wei12(ivec2(-1,0), pp, dirFinal, scale, lob, clp);
    float wf = wei12(ivec2(0,0), pp, dirFinal, scale, lob, clp);
    float wg = wei12(ivec2(1,0), pp, dirFinal, scale, lob, clp);
    float wh = wei12(ivec2(2,0), pp, dirFinal, scale, lob, clp);

    float wi = wei12(ivec2(-1,1), pp, dirFinal, scale, lob, clp);
    float wj = wei12(ivec2(0,1), pp, dirFinal, scale, lob, clp);
    float wk = wei12(ivec2(1,1), pp, dirFinal, scale, lob, clp);
    float wl = wei12(ivec2(2,1), pp, dirFinal, scale, lob, clp);

    float wn = wei12(ivec2(0,2), pp, dirFinal, scale, lob, clp);
    float wo = wei12(ivec2(1,2), pp, dirFinal, scale, lob, clp);

    vec3 accuColor = wb*cb + wc*cc + we*ce + wf*cf + wg*cg +wh*ch + wi*ci + wj*cj + wk*ck + wl*cl + wn*cn + wo*co;
    float accuWeight = wb + wc + we + wf + wg + wh + wi + wj + wk + wl + wn + wo;

    vec3 min4rgb = min(min(min(cf,cg), cj), ck);
    vec3 max4rgb = max(max(max(cf,cg), cj), ck);

    vec3 finalColor = min(max((accuColor/accuWeight), min4rgb), max4rgb);

    FragColor = vec4(finalColor, 1.0);
}