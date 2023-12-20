#version 330 core

in vec4 v_penumbras;
in vec3 v_edges;
in vec3 v_proj_pos;
in vec4 v_endpoints;

out vec4 fragColor;

void main() {
	// Calculate the light intersection point, but clamp to endpoints to avoid artifacts.
	float intersection_t = clamp(v_edges.x / abs(v_edges.y), -0.5, 0.5);
	vec2 intersection_point = (0.5 - intersection_t) * v_endpoints.xy + (0.5 + intersection_t) * v_endpoints.zw;
	// The delta from the intersection to the pixel.
	vec2 penetration_delta = intersection_point - v_proj_pos.xy / v_proj_pos.z;
	// Apply a simple falloff function.
	float bleed = min(dot(penetration_delta, penetration_delta), 1.0);

	// Penumbra mixing.
	vec2 penumbras = smoothstep(-1.0, 1.0, v_penumbras.xz / v_penumbras.yw);
	float penumbra = dot(penumbras, step(v_penumbras.yw, vec2(0.0)));
	penumbra -= 1.0 / 64.0; // Numerical precision fudge factor.

	fragColor = vec4(bleed * (1.0 - penumbra) * step(v_edges.z, 0.0));
}
