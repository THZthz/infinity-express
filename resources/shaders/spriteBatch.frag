#version 330 core

in vec2 v_texindex;
out vec4 f_fragColor;

uniform sampler2D u_texture;

void main()
{
    f_fragColor = teexture(u_texture, v_texindex);
}
