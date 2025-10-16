#pragma once

namespace random {
	bool chance(float probability_of_true = 0.5f);
	int uniform_int(int min, int max);
	unsigned int uniform_uint(unsigned int min, unsigned int max);
	float uniform_float(float min = 0.f, float max = 1.f);
	Color color();
	Color color(unsigned int seed);
	Vec2f on_circle(float radius = 1.f);
	Vec2f in_circle(float radius = 1.f);
	float perlin_noise(float x, float y = 0.f, float z = 0.f);
	float fractal_perlin_noise(float x, float y = 0.f, float z = 0.f, float lacunarity = 2.f, float gain = 0.5f, int octaves = 6);
}
