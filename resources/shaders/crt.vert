#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_texCoord;

out vec3 v_color;
out vec2 v_texCoord;

void main() {
    gl_Position = vec4(a_pos, 1.0);
    v_color = a_color;
    v_texCoord = vec2(a_texCoord.x, a_texCoord.y);
}
