#version 330 core

layout(location = 0) in vec2 a_vertex;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

uniform mat4 u_matrix;

void main() {
	gl_Position = u_matrix * vec4(a_vertex, 0, 1);
	v_uv = a_uv;
}
