#version 330 core

in vec2 v_uv;

uniform vec3 u_color;

out vec4 fragColor;

void main() {
	// A nice radial gradient with quadratic falloff.
	float brightness = max(0.0, 1.0 - pow(dot(v_uv, v_uv), 0.25));
	fragColor = vec4(brightness * u_color, 1.0);
}
