#pragma once

namespace random
{
	bool chance(float probability_of_true = 0.5f);
	float range_f(float min = 0.f, float max = 1.f);
	int range_i(int min, int max);
	unsigned int range_ui(unsigned int min, unsigned int max);
	Color color();
	Color color(uint32_t seed);
	Vec2f on_circle(float radius = 1.f);
	Vec2f in_circle(float radius = 1.f);
	float perlin_noise(float x, float y = 0.f, float z = 0.f);
	float fractal_perlin_noise(float x, float y = 0.f, float z = 0.f, float lacunarity = 2.f, float gain = 0.5f, int octaves = 6);
}
