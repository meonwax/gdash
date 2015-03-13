<?xml version="1.0" encoding="UTF-8"?>
<!--
     GDash-TV shader
-->
<shader language="GLSL">
    <vertex><![CDATA[
    #version 120

    uniform vec2 rubyTextureSize;

    varying vec2 c00;
    varying vec2 c10;
    varying vec2 c01;
    
    void main(void)
    {
        vec2 tex = gl_MultiTexCoord0.xy;
        vec2 onetexelx = vec2(1.0 / rubyTextureSize.x, 0.0);
        vec2 onetexely = vec2(0.0, 1.0 / rubyTextureSize.y);

        c00 = tex;
        c10 = tex - onetexelx;
        c01 = tex - onetexely;

        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    }
    ]]></vertex>
  <fragment filter="linear"><![CDATA[
    #version 120

    uniform float randomSeed;

    uniform float CHROMA_TO_LUMA_STRENGTH;
    uniform float LUMA_TO_CHROMA_STRENGTH;
    uniform float SCANLINE_SHADE_LUMA;
    uniform float PHOSPHOR_SHADE;
    uniform float RANDOM_SCANLINE_DISPLACE;
    uniform float RANDOM_Y;
    uniform float RANDOM_UV;
    uniform float LUMA_X_BLUR;
    uniform float CHROMA_X_BLUR;
    uniform float CHROMA_Y_BLUR;
    uniform float RADIAL_DISTORTION;

    uniform sampler2D rubyTexture;
    uniform vec2 rubyTextureSize;

    varying vec2 c00;
    varying vec2 c10;
    varying vec2 c01;
    
    float rand(vec2 seed) {
        return fract(sin(dot(vec2(seed.x + randomSeed, seed.y + randomSeed), vec2(12.9898, 78.233))) * 43758.5453);
    }

    vec2 rand_x() {
        return vec2(fract(sin(dot(vec2(randomSeed, gl_FragCoord.y), vec2(12.9898, 78.233))) * 43758.5453), 0.0);
    }
 
    vec2 radialDistortion(vec2 coord) {
      vec2 cc = coord - vec2(0.5);
      float dist = dot(cc, cc) * RADIAL_DISTORTION * 0.5;
      return (coord + cc * (1.0 + dist) * dist);
    }            

    vec4 texture_x_linear(vec2 pos) {
        pos = radialDistortion(pos);
        return texture2D(rubyTexture, vec2(pos.x, (floor(pos.y * rubyTextureSize.y) + 0.5) / rubyTextureSize.y));
    }
    
    vec4 texture(vec2 pos) {
        pos = radialDistortion(pos);
        return texture2D(rubyTexture, pos);
    }
    
    void main(void) {
        mat3 rgb2yuv = mat3(0.299, -0.14713,  0.615,
                            0.587, -0.28886, -0.51499,
                            0.114,  0.436  , -0.10001);
        mat3 yuv2rgb = mat3(1.0,      1.0,     1.0,
                            0.0,     -0.39465, 2.03211,
                            1.13983, -0.58060, 0.0);

        bool even = mod(c00.y * rubyTextureSize.y, 2.0) < 1.0;
        vec3 phosphor = vec3(1.0, PHOSPHOR_SHADE, PHOSPHOR_SHADE);

        /* to yuv */
        vec2 dist = rand_x() / rubyTextureSize.x * RANDOM_SCANLINE_DISPLACE;
        vec3 yuvm1 = rgb2yuv * texture_x_linear(c10 + dist).rgb;
        vec3 yuv_0 = rgb2yuv * texture_x_linear(c00 + dist).rgb;
        vec3 yuvym1 = rgb2yuv * texture(c01 + dist).rgb;
        
        vec3 yuv = vec3(
            /* luma: set as blurred from original */
            /* y */ mix(yuv_0.x, yuvm1.x, LUMA_X_BLUR * 0.5),
            /* chroma: set as blurred from original */
            /* u */ mix(yuv_0.y, yuvym1.y, CHROMA_Y_BLUR),
            /* v */ mix(yuv_0.z, yuvym1.z, CHROMA_Y_BLUR)
        );

        /* edge detect for crosstalk. */
        /* also chroma x blur from the edge detect vector - otherwise it's not really visible. */
        vec3 d = yuvm1 - yuv_0;
        yuv += vec3(
            /* chroma crosstalk to luma */
            /* y */ CHROMA_TO_LUMA_STRENGTH * (even ? (d.y + d.z) : (d.z - d.y)),
            /* luma crosstalk to chroma */
            /* u */ -LUMA_TO_CHROMA_STRENGTH * d.x + CHROMA_X_BLUR * d.y,
            /* v */ LUMA_TO_CHROMA_STRENGTH * d.x + CHROMA_X_BLUR * d.z
        );

        /* random noise. the positions are used to feed the random gen. */
        yuv += vec3(
            (rand(c00) - 0.5) * RANDOM_Y,
            (rand(c01) - 0.5) * RANDOM_UV,
            (rand(c10) - 0.5) * RANDOM_UV
        );

        /* darken every second row */
        if (mod(gl_FragCoord.y, 2.0) < 1.0)
            yuv.x *= SCANLINE_SHADE_LUMA;

        /* phosphor stuff */
        float pix = mod(gl_FragCoord.x, 3.0);
        if (pix < 1.0)
            phosphor = phosphor.yzx;
        else if (pix < 2.0)
            phosphor = phosphor.zxy;

        /* back to rgb and add phosphor. and here is the result */
        gl_FragColor.rgb =  yuv2rgb * yuv * phosphor;
    }
  ]]></fragment>
</shader>
