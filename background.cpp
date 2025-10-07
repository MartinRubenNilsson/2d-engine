#include "stdafx.h"
#include "background.h"
#include "graphics.h"
#include "sprites.h"
#include "console.h"

namespace background
{
	const std::string _MOUNTAIN_DUSK_TEXTURE_PATHS[] = {
		"assets/textures/backgrounds/mountain_dusk/sky.png",
		"assets/textures/backgrounds/mountain_dusk/far-clouds.png",
		"assets/textures/backgrounds/mountain_dusk/near-clouds.png",
		"assets/textures/backgrounds/mountain_dusk/far-mountains.png",
		"assets/textures/backgrounds/mountain_dusk/mountains.png",
		"assets/textures/backgrounds/mountain_dusk/trees.png",
	};

	struct Layer
	{
		Handle<graphics::Texture> texture;
		unsigned int texture_width = 0;
		unsigned int texture_height = 0;
		float offset_x = 0.f;
	};

	Type _type = Type::None;
	std::vector<Layer> _layers;

	void set_type(Type type)
	{
		if (_type == type) return;
		_type = type;
		switch (type) {
		case Type::None: {
			_layers.clear();
		} break;
		case Type::MountainDusk: {
			_layers.clear();
			for (const std::string& path : _MOUNTAIN_DUSK_TEXTURE_PATHS) {
				const Handle<graphics::Texture> texture = graphics::load_texture(path);
				if (texture == Handle<graphics::Texture>()) continue;
				Layer& layer = _layers.emplace_back();
				layer.texture = texture;
				graphics::get_texture_size(texture, layer.texture_width, layer.texture_height);
			}
		} break;
		}
	}

	void update(float dt)
	{
		if (_type == Type::None) return;
		for (size_t i = 0; i < _layers.size(); ++i) {
			Layer& layer = _layers[i];
			layer.offset_x += i * i * i * dt; // different layers scroll at different speeds
			if (layer.offset_x >= layer.texture_width) {
				layer.offset_x -= layer.texture_width; // wrap around
			}
		}
	}

	void draw_sprites(const Vec2f& camera_min, const Vec2f& camera_max)
	{
		if (_type == Type::None) return;
		graphics::ScopedDebugGroup debug_group("background::draw_sprites()");

		sprites::Sprite sprite{};
		for (const Layer& layer : _layers) {
			if (layer.texture == Handle<graphics::Texture>()) continue;
			if (!layer.texture_width) continue;
			sprite.texture = layer.texture;
			sprite.size = { (float)layer.texture_width, (float)layer.texture_height };
			for (float x = camera_min.x - layer.offset_x; x < camera_max.x; x += layer.texture_width) {
				sprite.position = { x, camera_min.y };
				sprites::draw_sprite_later(sprite);
			}
		}

		// We don't have to sort before drawing, since the sprites were added in draw order.
		sprites::draw_all_sprites();
	}
}