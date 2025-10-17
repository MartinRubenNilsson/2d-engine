#include "stdafx.h"
#include "player_outfit.h"
#include "graphics.h"
#include "graphics_globals.h"
#include "random.h"

namespace player {
	const int C3_LUT_COLORS = 48;
	const int C4_LUT_COLORS = 58;

	const int SKIN_COLORS = 18;
	const int HAIR_COLORS = 58;
	const int SOCK_COLORS = C3_LUT_COLORS;
	const int SHOE_COLORS = C3_LUT_COLORS;
	const int LOWERWEAR_COLORS = C3_LUT_COLORS;
	const int SHIRT_COLORS = C3_LUT_COLORS;
	const int GLOVES_COLORS = C3_LUT_COLORS;
	const int OUTERWEAR_COLORS = C3_LUT_COLORS;
	const int NECKWEAR_COLORS_1 = C3_LUT_COLORS;
	const int NECKWEAR_COLORS_2 = C4_LUT_COLORS;
	const int GLASSES_COLORS = C3_LUT_COLORS;
	const int HAT_COLORS_1 = C3_LUT_COLORS;
	const int HAT_COLORS_2 = C4_LUT_COLORS;

	void randomize_outfit(Outfit& outfit) {
		outfit.body = (Outfit::Body)(random::uniform_int(1, (int)Outfit::Body::Count - 1));
		outfit.skin_color = random::uniform_int(0, SKIN_COLORS - 1);
		outfit.socks = (Outfit::Socks)(random::uniform_int(0, (int)Outfit::Socks::Count - 1));
		outfit.socks_color = random::uniform_int(0, SOCK_COLORS - 1);
		outfit.shoes = (Outfit::Shoes)(random::uniform_int(0, (int)Outfit::Shoes::Count - 1));
		outfit.shoes_color = random::uniform_int(0, SHOE_COLORS - 1);
		outfit.lowerwear = (Outfit::Lowerwear)(random::uniform_int(0, (int)Outfit::Lowerwear::Count - 1));
		outfit.lowerwear_color = random::uniform_int(0, LOWERWEAR_COLORS - 1);
		outfit.shirt = (Outfit::Shirt)(random::uniform_int(0, (int)Outfit::Shirt::Count - 1));
		outfit.shirt_color = random::uniform_int(0, SHIRT_COLORS - 1);
		outfit.gloves = (Outfit::Gloves)(random::uniform_int(0, (int)Outfit::Gloves::Count - 1));
		outfit.gloves_color = random::uniform_int(0, GLOVES_COLORS - 1);
		outfit.outerwear = (Outfit::Outerwear)(random::uniform_int(0, (int)Outfit::Outerwear::Count - 1));
		outfit.outerwear_color = random::uniform_int(0, OUTERWEAR_COLORS - 1);
		outfit.neckwear = (Outfit::Neckwear)(random::uniform_int(0, (int)Outfit::Neckwear::Count - 1));
		outfit.neckwear_color_1 = random::uniform_int(0, NECKWEAR_COLORS_1 - 1);
		outfit.neckwear_color_2 = random::uniform_int(0, NECKWEAR_COLORS_2 - 1);
		outfit.glasses = (Outfit::Glasses)(random::uniform_int(0, (int)Outfit::Glasses::Count - 1));
		outfit.glasses_color = random::uniform_int(0, GLASSES_COLORS - 1);
		outfit.hair = (Outfit::Hair)(random::uniform_int(0, (int)Outfit::Hair::Count - 1));
		outfit.hair_color = random::uniform_int(0, HAIR_COLORS - 1);
		outfit.hat = (Outfit::Hat)(random::uniform_int(0, (int)Outfit::Hat::Count - 1));
		outfit.hat_color_1 = random::uniform_int(0, HAT_COLORS_1 - 1);
		outfit.hat_color_2 = random::uniform_int(0, HAT_COLORS_2 - 1);
	}

	Handle<graphics::Texture> create_outfit_texture(Outfit& outfit) {
		enum LookupTextureType {
			LUT_SKIN,
			LUT_HAIR,
			LUT_C3, // 3-color
			LUT_C4, // 4-color
		};

		struct Layer {
			const char* texture_path = nullptr;
			int lut1_type = -1;
			int lut1_y = -1;
			int lut2_type = -1;
			int lut2_y = -1;
		};

		std::vector<Layer> layers;

		// 00undr

		switch (outfit.neckwear) {
		case Outfit::Neckwear::CloakPlain:
			layers.emplace_back("00undr/fbas_00undr_cloakplain_00d.png", LUT_C3, outfit.neckwear_color_1, LUT_C4, outfit.neckwear_color_2);
			break;
		case Outfit::Neckwear::CloakWithMantlePlain:
			layers.emplace_back("00undr/fbas_00undr_cloakwithmantleplain_00b.png", LUT_C4, outfit.neckwear_color_1);
			break;
		}

		// 01body

		switch (outfit.body) {
		case Outfit::Body::Human:
			layers.emplace_back("01body/fbas_01body_human_00.png", LUT_SKIN, outfit.skin_color);
			break;
		}

		// 02sock

		switch (outfit.socks) {
		case Outfit::Socks::SocksHigh:
			layers.emplace_back("02sock/fbas_02sock_sockshigh_00a.png", LUT_C3, outfit.socks_color);
			break;
		case Outfit::Socks::SocksLow:
			layers.emplace_back("02sock/fbas_02sock_sockslow_00a.png", LUT_C3, outfit.socks_color);
			break;
		case Outfit::Socks::Stockings:
			layers.emplace_back("02sock/fbas_02sock_sockslow_00a.png", LUT_C3, outfit.socks_color);
			break;
		}

		// 03fot1

		switch (outfit.shoes) {
		case Outfit::Shoes::Boots:
			layers.emplace_back("03fot1/fbas_03fot1_boots_00a.png", LUT_C3, outfit.shoes_color);
			break;
		case Outfit::Shoes::Sandals:
			layers.emplace_back("03fot1/fbas_03fot1_sandals_00a.png", LUT_C3, outfit.shoes_color);
			break;
		case Outfit::Shoes::Shoes:
			layers.emplace_back("03fot1/fbas_03fot1_shoes_00a.png", LUT_C3, outfit.shoes_color);
			break;
		}

		// 04lwr1

		switch (outfit.lowerwear) {
		case Outfit::Lowerwear::LongPants:
			layers.emplace_back("04lwr1/fbas_04lwr1_longpants_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::Onepiece:
			layers.emplace_back("04lwr1/fbas_04lwr1_onepiece_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::OnepieceBoobs:
			layers.emplace_back("04lwr1/fbas_04lwr1_onepieceboobs_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::Shorts:
			layers.emplace_back("04lwr1/fbas_04lwr1_shorts_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::Undies:
			layers.emplace_back("04lwr1/fbas_04lwr1_undies_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		}

		// 05shrt

		switch (outfit.shirt) {
		case Outfit::Shirt::Bra:
			layers.emplace_back("05shrt/fbas_05shrt_bra_00a.png", LUT_C3, outfit.shirt_color);
			break;
		case Outfit::Shirt::LongShirt:
			layers.emplace_back("05shrt/fbas_05shrt_longshirt_00a.png", LUT_C3, outfit.shirt_color);
			break;
		case Outfit::Shirt::LongShirtBoobs:
			layers.emplace_back("05shrt/fbas_05shrt_longshirtboobs_00a.png", LUT_C3, outfit.shirt_color);
			break;
		case Outfit::Shirt::ShortShirt:
			layers.emplace_back("05shrt/fbas_05shrt_shortshirt_00a.png", LUT_C3, outfit.shirt_color);
			break;
		case Outfit::Shirt::ShortShirtBoobs:
			layers.emplace_back("05shrt/fbas_05shrt_shortshirtboobs_00a.png", LUT_C3, outfit.shirt_color);
			break;
		case Outfit::Shirt::TankTop:
			layers.emplace_back("05shrt/fbas_05shrt_tanktop_00a.png", LUT_C3, outfit.shirt_color);
			break;
		case Outfit::Shirt::TankTopBoobs:
			layers.emplace_back("05shrt/fbas_05shrt_tanktopboobs_00a.png", LUT_C3, outfit.shirt_color);
			break;
		}

		// 06lwr2

		switch (outfit.lowerwear) {
		case Outfit::Lowerwear::Overalls:
			layers.emplace_back("06lwr2/fbas_06lwr2_overalls_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::OverallsBoobs:
			layers.emplace_back("06lwr2/fbas_06lwr2_overallsboobs_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::Shortalls:
			layers.emplace_back("06lwr2/fbas_06lwr2_shortalls_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::ShortallsBoobs:
			layers.emplace_back("06lwr2/fbas_06lwr2_shortallsboobs_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		}

		// 07fot2

		switch (outfit.shoes) {
		case Outfit::Shoes::CuffedBoots:
			layers.emplace_back("07fot2/fbas_07fot2_cuffedboots_00a.png", LUT_C3, outfit.shoes_color);
			break;
		case Outfit::Shoes::CurlyToeShoes:
			layers.emplace_back("07fot2/fbas_07fot2_curlytoeshoes_00a.png", LUT_C3, outfit.shoes_color);
			break;
		}

		// 08lwr3

		switch (outfit.lowerwear) {
		case Outfit::Lowerwear::FrillyDress:
			layers.emplace_back("08lwr3/fbas_08lwr3_frillydress_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::FrillyDressBoobs:
			layers.emplace_back("08lwr3/fbas_08lwr3_frillydressboobs_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::FrillySkirt:
			layers.emplace_back("08lwr3/fbas_08lwr3_frillyskirt_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::LongDress:
			layers.emplace_back("08lwr3/fbas_08lwr3_longdress_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::LongDressBoobs:
			layers.emplace_back("08lwr3/fbas_08lwr3_longdressboobs_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		case Outfit::Lowerwear::LongSkirt:
			layers.emplace_back("08lwr3/fbas_08lwr3_longskirt_00a.png", LUT_C3, outfit.lowerwear_color);
			break;
		}

		// 09hand

		switch (outfit.gloves) {
		case Outfit::Gloves::Gloves:
			layers.emplace_back("09hand/fbas_09hand_gloves_00a.png", LUT_C3, outfit.gloves_color);
			break;
		}

		// 10outr

		switch (outfit.outerwear) {
		case Outfit::Outerwear::Suspenders:
			layers.emplace_back("10outr/fbas_10outr_suspenders_00a.png", LUT_C3, outfit.outerwear_color);
			break;
		case Outfit::Outerwear::Vest:
			layers.emplace_back("10outr/fbas_10outr_vest_00a.png", LUT_C3, outfit.outerwear_color);
			break;
		}

		// 11neck

		switch (outfit.neckwear) {
		case Outfit::Neckwear::CloakPlain:
			layers.emplace_back("11neck/fbas_11neck_cloakplain_00d.png", LUT_C3, outfit.neckwear_color_1, LUT_C4, outfit.neckwear_color_2);
			break;
		case Outfit::Neckwear::CloakWithMantlePlain:
			layers.emplace_back("11neck/fbas_11neck_cloakwithmantleplain_00b.png", LUT_C4, outfit.neckwear_color_1);
			break;
		case Outfit::Neckwear::MantlePlain:
			layers.emplace_back("11neck/fbas_11neck_mantleplain_00b.png", LUT_C4, outfit.neckwear_color_1);
			break;
		case Outfit::Neckwear::Scarf:
			layers.emplace_back("11neck/fbas_11neck_scarf_00b.png", LUT_C4, outfit.neckwear_color_1);
			break;
		}

		// 12face

		switch (outfit.glasses) {
		case Outfit::Glasses::Glasses:
			layers.emplace_back("12face/fbas_12face_glasses_00a.png", LUT_C3, outfit.glasses_color);
			break;
		case Outfit::Glasses::Shades:
			layers.emplace_back("12face/fbas_12face_shades_00a.png", LUT_C3, outfit.glasses_color);
			break;
		}

		// 13hair

		bool headwear_replaces_hair = false;
		switch (outfit.hat) {
		case Outfit::Hat::Bandana:
		case Outfit::Hat::Headscarf:
			headwear_replaces_hair = true;
			break;
		}

		if (!headwear_replaces_hair) {
			switch (outfit.hair) {
			case Outfit::Hair::Afro:
				layers.emplace_back("13hair/fbas_13hair_afro_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::AfroPuffs:
				layers.emplace_back("13hair/fbas_13hair_afropuffs_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Bob1:
				layers.emplace_back("13hair/fbas_13hair_bob1_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Bob2:
				layers.emplace_back("13hair/fbas_13hair_bob2_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Dapper:
				layers.emplace_back("13hair/fbas_13hair_dapper_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Flattop:
				layers.emplace_back("13hair/fbas_13hair_flattop_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::LongWavy:
				layers.emplace_back("13hair/fbas_13hair_longwavy_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Ponytail1:
				layers.emplace_back("13hair/fbas_13hair_ponytail1_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Spiky1:
				layers.emplace_back("13hair/fbas_13hair_spiky1_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Spiky2:
				layers.emplace_back("13hair/fbas_13hair_spiky2_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Twintail:
				layers.emplace_back("13hair/fbas_13hair_twintail_00.png", LUT_HAIR, outfit.hair_color);
				break;
			case Outfit::Hair::Twists:
				layers.emplace_back("13hair/fbas_13hair_twists_00.png", LUT_HAIR, outfit.hair_color);
				break;
			}
		}

		// 14head

		switch (outfit.hat) {
		case Outfit::Hat::Bandana:
			layers.emplace_back("14head/fbas_14head_bandana_00b_e.png", LUT_C4, outfit.hat_color_1);
			break;
		case Outfit::Hat::BoaterHat:
			layers.emplace_back("14head/fbas_14head_boaterhat_00d.png", LUT_C3, outfit.hat_color_1, LUT_C4, outfit.hat_color_2);
			break;
		case Outfit::Hat::CowboyHat:
			layers.emplace_back("14head/fbas_14head_cowboyhat_00d.png", LUT_C3, outfit.hat_color_1, LUT_C4, outfit.hat_color_2);
			break;
		case Outfit::Hat::FloppyHat:
			layers.emplace_back("14head/fbas_14head_floppyhat_00d.png", LUT_C3, outfit.hat_color_1, LUT_C4, outfit.hat_color_2);
			break;
		case Outfit::Hat::Headscarf:
			layers.emplace_back("14head/fbas_14head_headscarf_00b_e.png", LUT_C4, outfit.hat_color_1);
			break;
		case Outfit::Hat::StrawHat:
			layers.emplace_back("14head/fbas_14head_strawhat_00d.png", LUT_C3, outfit.hat_color_1, LUT_C4, outfit.hat_color_2);
			break;
		}

		if (graphics::player_outfit_frag == Handle<graphics::FragmentShader>()) return Handle<graphics::Texture>();

		graphics::Viewport prev_viewport = graphics::get_viewport();
		graphics::set_viewport({ .width = 1024.f, .height = 1024.f });

		constexpr float CLEAR_COLOR[4] = { 0.f, 0.f, 0.f, 0.f };
		graphics::clear_framebuffer(graphics::player_outfit_framebuffer, CLEAR_COLOR);
		graphics::bind_framebuffer(graphics::player_outfit_framebuffer);
		graphics::bind_vertex_shader(graphics::fullscreen_vert);
		graphics::bind_fragment_shader(graphics::player_outfit_frag);

		const std::string base_dir = "assets/textures/character/";

		for (const Layer& layer : layers) {
			const Handle<graphics::Texture> texture = graphics::load_texture(base_dir + layer.texture_path);
			if (texture == Handle<graphics::Texture>()) continue;

			Handle<graphics::Texture> lut1_texture;
			switch (layer.lut1_type) {
			case LUT_SKIN:
				lut1_texture = graphics::load_texture(base_dir + "palettes/mana seed skin ramps.png");
				break;
			case LUT_HAIR:
				lut1_texture = graphics::load_texture(base_dir + "palettes/mana seed hair ramps.png");
				break;
			case LUT_C3:
				lut1_texture = graphics::load_texture(base_dir + "palettes/mana seed 3-color ramps.png");
				break;
			case LUT_C4:
				lut1_texture = graphics::load_texture(base_dir + "palettes/mana seed 4-color ramps.png");
				break;
			}

			Handle<graphics::Texture> lut2_texture;
			switch (layer.lut2_type) {
			case LUT_SKIN:
				lut2_texture = graphics::load_texture(base_dir + "palettes/mana seed skin ramps.png");
				break;
			case LUT_HAIR:
				lut2_texture = graphics::load_texture(base_dir + "palettes/mana seed hair ramps.png");
				break;
			case LUT_C3:
				lut2_texture = graphics::load_texture(base_dir + "palettes/mana seed 3-color ramps.png");
				break;
			case LUT_C4:
				lut2_texture = graphics::load_texture(base_dir + "palettes/mana seed 4-color ramps.png");
				break;
			}

			graphics::PlayerOutfitUniformBlock uniform_block{};
			uniform_block.lut1_type = layer.lut1_type;
			uniform_block.lut1_y = layer.lut1_y;
			uniform_block.lut2_type = layer.lut2_type;
			uniform_block.lut2_y = layer.lut2_y;

			graphics::update_buffer(graphics::player_outfit_uniform_buffer, &uniform_block, sizeof(uniform_block));
			graphics::bind_uniform_buffer(1, graphics::player_outfit_uniform_buffer);
			graphics::bind_texture(0, texture);
			graphics::bind_texture(1, lut1_texture);
			graphics::bind_texture(2, lut2_texture);
			graphics::set_primitives(graphics::Primitives::TriangleList);
			graphics::draw(3); // draw a fullscreen-covering triangle
		}

		graphics::set_viewport(prev_viewport);

		return graphics::get_framebuffer_texture(graphics::player_outfit_framebuffer);
	}
}