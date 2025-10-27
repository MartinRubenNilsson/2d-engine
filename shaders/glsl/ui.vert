#version 460

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec4 vertex_color;
layout(location = 2) in vec2 vertex_tex_coord;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 tex_coord;
layout(location = 2) out flat uint channels;

void main() {
	// NOTE: We pre-transform the vertices to clip space on the CPU.
	gl_Position = vec4(vertex_position, 0.0, 1.0);
	color = vertex_color;
	tex_coord = vertex_tex_coord;
	channels = 4;
	// HACK: We use negative texture coordinates to indicate that the texture is grayscale
	// (only has one channel), as for example is the case for font atlas textures.
	if (tex_coord.x < 0.0 || tex_coord.y < 0.0) {
		tex_coord = -tex_coord;
		channels = 1;
	}
}