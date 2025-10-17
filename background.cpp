#include "stdafx.h"
#include "background.h"
#include "graphics.h"
#include "sprites.h"

namespace background {
	const std::string _MOUNTAIN_DUSK_TEXTURE_PATHS[] = {
		"assets/textures/backgrounds/mountain_dusk/sky.png",
		"assets/textures/backgrounds/mountain_dusk/far-clouds.png",
		"assets/textures/backgrounds/mountain_dusk/near-clouds.png",
		"assets/textures/backgrounds/mountain_dusk/far-mountains.png",
		"assets/textures/backgrounds/mountain_dusk/mountains.png",
		"assets/textures/backgrounds/mountain_dusk/trees.png",
	};

	struct Layer {
		Handle<graphics::Texture> texture;
		Vec2u texture_size;
		float offset_x = 0.f;
	};

	Type _type = Type::None;
	std::vector<Layer> _layers;

	void set_type(Type type) {
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
					layer.texture_size = graphics::get_texture_size(texture);
				}
			} break;
		}
	}

	void update(float dt) {
		if (_type == Type::None) return;
		for (size_t i = 0; i < _layers.size(); ++i) {
			Layer& layer = _layers[i];
			layer.offset_x += i * i * i * dt; // different layers scroll at different speeds
			if (layer.offset_x >= layer.texture_size.x) {
				layer.offset_x -= layer.texture_size.x; // wrap around
			}
		}
	}

	void draw_sprites_now(const Vec2f& camera_min, const Vec2f& camera_max) {
		if (_type == Type::None) return;

		sprites::Sprite sprite{};
		for (const Layer& layer : _layers) {
			if (layer.texture == Handle<graphics::Texture>()) continue;
			if (!layer.texture_size.x) continue;
			sprite.texture = layer.texture;
			sprite.size = layer.texture_size;
			for (float x = camera_min.x - layer.offset_x; x < camera_max.x; x += layer.texture_size.x) {
				sprite.position = { x, camera_min.y };
				sprites::draw_later(sprite);
			}
		}

		// We don't have to sort before drawing, since the sprites were added in draw order.
		sprites::draw_all_now(__FUNCTION__);
	}
}