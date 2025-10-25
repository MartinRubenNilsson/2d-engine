#include "stdafx.h"
#include "random.h"
#include <random>
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

namespace random {
	const float PI = 3.14159265359f;
	const float PI_2 = 6.28318530718f;

	std::default_random_engine _engine(std::random_device{}());
	std::default_random_engine _seeded_engine;

	bool boolean(float probability_of_true) {
		std::bernoulli_distribution dist(probability_of_true);
		return dist(_engine);
	}

	int uniform_int(int min, int max) {
		std::uniform_int_distribution<int> dist(min, max);
		return dist(_engine);
	}

	unsigned int uniform_uint(unsigned int min, unsigned int max) {
		std::uniform_int_distribution<unsigned int> dist(min, max);
		return dist(_engine);
	}

	float uniform_float(float min, float max) {
		std::uniform_real_distribution<float> dist(min, max);
		return dist(_engine);
	}

	// Convert HSV to RGB
	// h: hue [0.0, 1.0]
	// s: saturation [0.0, 1.0]
	// v: value/brightness [0.0, 1.0]
	Color _hsv_to_rgb(float h, float s, float v) {
		const int h_i = static_cast<int>(h * 6.f);
		const float f = h * 6.f - h_i;
		const float p = v * (1.f - s);
		const float q = v * (1.f - f * s);
		const float t = v * (1.f - (1.f - f) * s);
		float r, g, b;
		switch (h_i) {
			case 0:
				r = v; g = t; b = p;
				break;
			case 1:
				r = q; g = v; b = p;
				break;
			case 2:
				r = p; g = v; b = t;
				break;
			case 3:
				r = p; g = q; b = v;
				break;
			case 4:
				r = t; g = p; b = v;
				break;
			case 5:
			default:
				r = v; g = p; b = q;
				break;
		}
		Color color;
		color.r = static_cast<uint8_t>(r * 255);
		color.g = static_cast<uint8_t>(g * 255);
		color.b = static_cast<uint8_t>(b * 255);
		color.a = 255;
		return color;
	}

	Color color() {
		std::uniform_real_distribution<float> dist(0.f, 1.f);
		return _hsv_to_rgb(dist(_engine), 0.5f, 0.95f);
	}

	Color color(unsigned int seed) {
		constexpr float _1_DIV_GOLDEN_RATIO = 0.618033988749895f;
		float h = seed * _1_DIV_GOLDEN_RATIO;
		h = h - (int)h;
		return _hsv_to_rgb(h, 0.5f, 0.95f);
	}

	Vec2f on_circle(float radius) {
		float angle = uniform_float(0, PI_2);
		return Vec2f(cos(angle) * radius, sin(angle) * radius);
	}

	Vec2f in_circle(float radius) {
		// http://mathworld.wolfram.com/DiskPointPicking.html
		float r = sqrt(uniform_float(0.0f, radius));
		return on_circle(r);
	}

	float perlin_noise(float x, float y, float z) {
		return stb_perlin_noise3(x, y, z, 0, 0, 0);
	}

	float fractal_perlin_noise(float x, float y, float z, float lacunarity, float gain, int octaves) {
		return stb_perlin_fbm_noise3(x, y, z, lacunarity, gain, octaves);
	}
}