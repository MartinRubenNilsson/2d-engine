#version 460

uniform sampler2D tex;

layout(std140, binding = 1) uniform ScreenTransitionUniformBlock {
	float progress;
    float pixel_size;
};

layout(location = 0) out vec4 frag_color;

void main() {
    // https://ddrkirby.com/articles/shader-based-transitions/shader-based-transitions.html

    const float DIAMOND_SIZE_IN_PIXELS = 20.0;

    const vec2 tex_size = textureSize(tex, 0);
    const vec2 tex_size_in_pixels = tex_size / pixel_size;
    const vec2 pixel = floor(gl_FragCoord.xy / pixel_size);
    const vec2 pixel_fractions = fract(pixel / DIAMOND_SIZE_IN_PIXELS);
    const vec2 pixel_distances = abs(pixel_fractions - 0.5);
    const vec2 tex_size_in_diamonds = tex_size_in_pixels / DIAMOND_SIZE_IN_PIXELS;
    const float max_diamond_distance = tex_size_in_diamonds.x + tex_size_in_diamonds.y;
    const vec2 diamond = floor(pixel / DIAMOND_SIZE_IN_PIXELS);
    const vec2 diamond_distances = diamond / max_diamond_distance;

    const float pixel_progress = pixel_distances.x + pixel_distances.y; // <= 1.0
    const float diamond_progress = diamond_distances.x + diamond_distances.y; // <= 1.0
    const float total_progress = (pixel_progress + diamond_progress) / 2.0; // <= 1.0

    if (progress < 0.0 && 1.0 + progress < total_progress) {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    } else if (progress > 0.0 && total_progress < progress) {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        frag_color = texture(tex, gl_FragCoord.xy / tex_size);
	}
}