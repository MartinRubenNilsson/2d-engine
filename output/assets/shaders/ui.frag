#version 460

uniform sampler2D tex;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in flat uint channels;

layout(location = 0) out vec4 frag_color;

void main() {
	vec4 tex_color = texture(tex, tex_coord);
	if (channels == 1) { // Is the texture grayscale?
		tex_color.a = tex_color.r;
		tex_color.rgb = vec3(1.0, 1.0, 1.0);
	}
	frag_color = tex_color * color;
	//TODO: rounded corners
}