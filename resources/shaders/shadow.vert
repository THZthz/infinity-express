#version 330 core

layout (location = 0) in vec4 a_segment;
layout (location = 1) in vec2 a_shadow_coord;

uniform mat4 u_matrix;
uniform vec3 u_light;

out vec4 v_penumbras;
out vec3 v_edges;
out vec3 v_proj_pos;
out vec4 v_endpoints;

// column major in glsl, but row major in hlsl.
mat2 adjugate(vec2 c1, vec2 c2) { return mat2(c2[1], -c1[1], -c2[0], c1[0]); }

void main() {
    // Unpack the vertex shader input.
    vec2 endpoint_a = (u_matrix * vec4(a_segment.zw, 0.0, 1.0)).xy;
    vec2 endpoint_b = (u_matrix * vec4(a_segment.xy, 0.0, 1.0)).xy;
    vec2 endpoint = mix(endpoint_a, endpoint_b, a_shadow_coord.x);
    float light_radius = u_light.z;

    // Deltas from the segment to the light center.
    vec2 delta_a = endpoint_a - u_light.xy;
    vec2 delta_b = endpoint_b - u_light.xy;
    vec2 delta = endpoint - u_light.xy;

    // Offsets from the light center to the edge of the light volume.
    vec2 offset_a = vec2(-light_radius, light_radius) * normalize(delta_a).yx;
    vec2 offset_b = vec2(light_radius, -light_radius) * normalize(delta_b).yx;
    vec2 offset = mix(offset_a, offset_b, a_shadow_coord.x);

    // Vertex projection.
    float w = a_shadow_coord.y;
    vec3 proj_pos = vec3(mix(delta - offset, endpoint, w), w);
    gl_Position = vec4(proj_pos.xy, 0, w);

    vec2 penumbra_a = adjugate(offset_a, -delta_a) * (delta - mix(offset, delta_a, w));
    vec2 penumbra_b = adjugate(-offset_b, delta_b) * (delta - mix(offset, delta_b, w));
    v_penumbras = (light_radius > 0.0 ? vec4(penumbra_a, penumbra_b) : vec4(0, 1, 0, 1));

    // Edge values for light penetration and clipping.
    vec2 seg_delta = endpoint_b - endpoint_a;
    vec2 seg_normal = seg_delta.yx * vec2(-1.0, 1.0);
    // Calculate where the light -> pixel ray will intersect with the segment.
    v_edges.xy = -adjugate(seg_delta, delta_a + delta_b) * (delta - offset * (1.0 - w));
    v_edges.y *= 2.0; // Skip a multiply in the fragment shader.
    // Calculate a clipping coordinate that is 0 at the near edge (when w = 1)...
    // otherwise calculate the dot product with the projected coordinate.
    v_edges.z = dot(seg_normal, delta - offset) * (1.0 - w);

    // Light penetration values.
    float light_penetration = 0.01;
    v_proj_pos = vec3(proj_pos.xy, w * light_penetration);
    v_endpoints = vec4(endpoint_a, endpoint_b) / light_penetration;
}
