#include "stdafx.h"
#include "ecs_ai_knowledge.h"
#include "ecs_common.h"
#include "ecs_physics.h"

namespace ecs
{
	extern entt::registry _registry;
	AiWorld _ai_world;

	AiEntity _make_ai_entity(entt::entity entity)
	{
		if (!_registry.valid(entity)) return AiEntity();
		AiEntity info{};
		info.entity = entity;
		info.tag = get_tag(entity);
		if (b2BodyId body = get_body(entity); B2_IS_NON_NULL(body)) {
			//info.position = b2Body_GetPosition(body);
			info.position = b2Body_GetWorldCenterOfMass(body);
			info.velocity = b2Body_GetLinearVelocity(body);
		}
		//get_float_property(entity, "speed", info.p_speed); // TODO
		return info;
	} 

	void update_ai_knowledge_and_world(float dt)
	{
		_ai_world.player = _make_ai_entity(find_entity_by_tag(Tag::Player));

		for (auto [entity, knowledge] : _registry.view<AiKnowledge>().each()) {
			knowledge.me = _make_ai_entity(entity);
		}
	}

	const AiWorld& get_ai_world()
	{
		return _ai_world;
	}

	AiKnowledge& emplace_ai_knowledge(entt::entity entity)
	{
		AiKnowledge& knowledge = _registry.emplace_or_replace<AiKnowledge>(entity);
		if (b2BodyId body = get_body(entity); B2_IS_NON_NULL(body)) {
			//knowledge.initial_position = b2Body_GetPosition(body);
			knowledge.initial_position = b2Body_GetWorldCenterOfMass(body);
			knowledge.initial_velocity = b2Body_GetLinearVelocity(body);
		}
		return knowledge;
	}
}

