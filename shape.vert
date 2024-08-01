#version 430

layout(std140, binding = 0) uniform FrameUniforms
{
	mat4 view_proj_matrix;
};

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec4 vertex_color;
out vec4 color;

void main()
{
    gl_Position = view_proj_matrix * vec4(vertex_position, 0.0, 1.0);
	color = vertex_color;
}