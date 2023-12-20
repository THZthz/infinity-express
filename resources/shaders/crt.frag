#version 330 core

out vec4 o_fragColor;

in vec3 v_color;
in vec2 v_texCoord;

uniform sampler2D u_texture1;
uniform float u_time;
uniform vec2 u_resolution;

// retro crt effect
vec4 CRT(vec2 uv, sampler2D tex, float curvature) {
    uv = uv * 2. - 1.;
    vec2 offset = uv.yx / curvature;
    uv += uv * offset * offset;
    uv = uv * .5 + .5;

    // distance from center of image, used to adjust blur
    float d = length(uv - vec2(0.5, 0.5));

    // blur amount
    float blur = (1.0 + sin(u_time * 0.2)) * 0.4;
    blur *= 1.0 + sin(u_time * 1.0) * 0.1;
    blur = pow(blur, 3.0);
    blur *= 0.05;
    // reduce blur towards center
    blur *= d;

    float edge_blur = 0.021;
    vec2 edge = smoothstep(0., edge_blur, uv) * (1. - smoothstep(1. - edge_blur, 1., uv));

    // chromatic aberration
    vec3 col;
    col.r = texture(tex, vec2(uv.x + blur, uv.y)).r;
    col.g = texture(tex, uv).g;
    col.b = texture(tex, vec2(uv.x - blur, uv.y)).b;
    col *= edge.x * edge.y;

    // scanline
    float scanline = sin(uv.y * u_resolution.y * 2) * 0.035;
    col -= scanline;

    // vignette
    col *= 1.0 - d * 0.5;

    return vec4(col, 1.0);
}

void main() {
    o_fragColor = CRT(v_texCoord, u_texture1, 5.9);
}
