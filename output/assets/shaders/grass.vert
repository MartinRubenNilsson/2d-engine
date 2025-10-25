#version 460

layout(std140, binding = 0) uniform FrameUniformBlock {
	float app_time;
	float game_time;
	float viewport_width;
	float viewport_height;
	mat4 view_proj_matrix;
};

layout(std140, binding = 1) uniform GrassUniformBlock {
	vec2 position;
	vec2 tex_min;
	vec2 tex_max;
};

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec4 vertex_color;
layout(location = 2) in vec2 vertex_tex_coord;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 tex_coord;

void main() {
	const vec2 local_tex_coord = (vertex_tex_coord - tex_min) / (tex_max - tex_min);
	const float sway_intensity = clamp(1.0 - local_tex_coord.y, 0.0, 1.0) * (0.5 + 0.3 * sin(0.3 * game_time));
	const float sway_time_offset = local_tex_coord.x * 2.0 + 0.008 * dot(position, vec2(1, 2));
	const float sway_time_frequency = 2.0;
	const float sway_x = 1.3 * sway_intensity * sin(1.927 * sway_time_frequency * (game_time - sway_time_offset));
	const float sway_y = 0.8 * sway_intensity * sin(1.284 * sway_time_frequency * (game_time - sway_time_offset));
	gl_Position = view_proj_matrix * vec4(vertex_position + vec2(sway_x, sway_y), 0.0, 1.0);
	color = vertex_color;
	tex_coord = vertex_tex_coord;
}