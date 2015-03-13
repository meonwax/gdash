<?xml version="1.0" encoding="UTF-8"?>
<shader language="GLSL">
    <vertex><![CDATA[
    uniform vec2 rubyInputSize;
    uniform vec2 rubyOutputSize;
    uniform vec2 rubyTextureSize;
    
    uniform float randomSeed;
    
    varying vec2 c00;
    varying vec2 c10;
    varying vec2 c20;
    varying vec2 c01;
    
    void main(void)
    {
        vec2 onetexelx = vec2(1.0 / rubyTextureSize.x, 0.0);
        vec2 onetexely = vec2(0.0, 1.0 / rubyTextureSize.y);

        vec2 tex = gl_MultiTexCoord0.xy;
        c00 = tex;
        c10 = tex - onetexelx;
        c20 = tex - onetexelx * 2.0;
        c01 = tex - onetexely;

        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    }
    ]]></vertex>
  <fragment filter="linear"><![CDATA[
    #version 120

    uniform float CHROMA_TO_LUMA_STRENGTH = 0.00;
    uniform float LUMA_TO_CHROMA_STRENGTH = 0.00;
    uniform float SCANLINE_SHADE_LUMA = 1.0;
    uniform float PHOSPHOR_SHADE = 1.0;
    uniform float RANDOM_SCANLINE_DISPLACE = 0.00;
    uniform float RANDOM_Y = 0.00;
    uniform float RANDOM_UV = 0.00;
    uniform float LUMA_X_BLUR = 0.0;
    uniform float CHROMA_X_BLUR = 0.0;
    uniform float CHROMA_Y_BLUR = 0.0;
    uniform float RADIAL_DISTORTION = 0.10;

    uniform mat3 rgb2yuv = mat3(0.299,-0.14713, 0.615,
                                0.587,-0.28886,-0.51499,
                                0.114, 0.436  ,-0.10001);
    uniform mat3 yuv2rgb = mat3(1.0, 1.0, 1.0,
                                0.0,-0.39465,2.03211,
                                1.13983,-0.58060,0.0);
    
    uniform sampler2D rubyTexture;
    uniform vec2 rubyInputSize;
    uniform vec2 rubyOutputSize;
    uniform vec2 rubyTextureSize;

    varying vec2 c00;
    varying vec2 c10;
    varying vec2 c20;
    varying vec2 c01;

    uniform float randomSeed;
    float random;
    
    float rand() {
        random = fract(sin(dot(c00.xy + vec2(random, randomSeed), vec2(12.9898, 78.233))) * 43758.5453);
        return random;
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
        bool even = mod(c00.y * rubyTextureSize.y, 2.0) < 1.0;
        vec3 phosphor = vec3(1.0, PHOSPHOR_SHADE, PHOSPHOR_SHADE);

        /* to yuv */
        vec2 dist = rand_x() / rubyTextureSize.x * RANDOM_SCANLINE_DISPLACE;
        vec3 yuvm2 = rgb2yuv * texture_x_linear(c20 + dist).rgb;
        vec3 yuvm1 = rgb2yuv * texture_x_linear(c10 + dist).rgb;
        vec3 yuv_0 = rgb2yuv * texture_x_linear(c00 + dist).rgb;
        vec3 yuvym1 = rgb2yuv * texture(c01 + dist).rgb;
        
        vec3 yuv = vec3(
            /* luma: set as blurred from original */
            /* y */ mix(yuv_0.x, yuvm1.x, LUMA_X_BLUR * 0.5),
            /* chroma: set as blurred from original */
            /* u */ mix(mix(yuv_0.y, yuvm1.y, CHROMA_X_BLUR * 0.5), yuvym1.y, CHROMA_Y_BLUR),
            /* v */ mix(mix(yuv_0.z, yuvm1.z, CHROMA_X_BLUR * 0.5), yuvym1.z, CHROMA_Y_BLUR)
        );

        /* edge detect for crosstalk */
        vec3 d = yuvm1 - yuv_0;
        yuv += vec3(
            /* chroma crosstalk to luma */
            /* y */ CHROMA_TO_LUMA_STRENGTH * (even ? (d.y + d.z) : (d.z - d.y)),
            /* luma crosstalk to chroma */
            /* u */ -LUMA_TO_CHROMA_STRENGTH * d.x,
            /* v */ LUMA_TO_CHROMA_STRENGTH * d.x
        );

        /* random */
        yuv += vec3(
            (rand()-0.5) * RANDOM_Y,
            (rand()-0.5) * RANDOM_UV,
            (rand()-0.5) * RANDOM_UV
        );

        /* darken every second row */
        if (mod(gl_FragCoord.y, 2.0) < 1.0)
            yuv.x *= SCANLINE_SHADE_LUMA;


        /* back to rgb */
        vec3 rgb = yuv2rgb * yuv;
        
        /* phosphor stuff */
        float pix = mod(gl_FragCoord.x, 3.0);
        if (pix < 1)
            phosphor = phosphor.yzx;
        else if (pix < 2)
            phosphor = phosphor.zxy;
        
        /* result */
        gl_FragColor.rgb = rgb * phosphor;
    }
  ]]></fragment>
</shader>
