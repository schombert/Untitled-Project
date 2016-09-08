#pragma once
#include "globalhelpers.h"
#include <functional>
#include <cfloat>
#include "ai_interest.h"



union ai_action_param {
	char_id person;
	title_id title;
	prov_id province;
	float fvalue;
	int ivalue;
};

class ai_action_prefs {
public:
	float friend_multiplier = 0.5f;
	float family_multiplier = 0.35f;
	float enemy_multiplier = -0.7f;
};

class ai_action;
class ai_action_block;

using ai_action_ptr = std::unique_ptr<ai_action, concurrent_delete<ai_action>>;
using ai_action_list = std::vector<ai_action_ptr, concurrent_allocator<ai_action_ptr>>;
using ai_block_ptr = std::unique_ptr<ai_action_block, concurrent_delete<ai_action_block>>;
using ai_blocks_list = std::vector<ai_block_ptr, concurrent_allocator<ai_block_ptr>>;

using ai_parameters = std::vector<ai_action_param, concurrent_allocator<ai_action_param>>;

class ai_action_block_spec {
public:
	virtual void enumerateresolutions(INOUT(ai_action_list) vals, IN(ai_parameters) parameters, const char_id self) const = 0;
	virtual bool valid(IN(ai_parameters) parameters, const char_id self) const = 0;
};

class ai_action_block {
public:
	const ai_action_block_spec* const __restrict type;
	ai_parameters parameters;
	bool initialized = false;
	ai_action_list resolutions;
	ai_action_block(const ai_action_block_spec* const __restrict t) : type(t) {};
};



class ai_action_spec {
public:
	virtual void takeaction(IN(ai_parameters) parameters, const char_id self) const = 0;
	virtual float evalaction(IN(ai_parameters) parameters, IN(ai_action_prefs) prf, const char_id self) const = 0;
	virtual void enumerateblocks(INOUT(ai_blocks_list) vals, IN(ai_parameters) parameters, const char_id self) const = 0;
};

#define PERCEPTION_HOLDER 0
#define NO_CB_WAR 1
#define MAKE_PEACE 2
#define HOST_PRIVATE_EVENT_WITH 3

#define NULL_BLOCK 0
#define RAISE_MONEY 1
#define RAISE_TROOPS 2
#define PERCEPTION_WAR 3
#define ALREADY_AT_WAR 4
#define POOR_RELATIONS_WITH 5
#define NOT_FRIENDS_WITH 6

const ai_action_spec* idtoactionspec(const size_t id);
size_t actionspectoid(const ai_action_spec* const type);
const ai_action_block_spec* idtoactionblockspec(const size_t id);
size_t actionblockspectoid(const ai_action_block_spec* const type);

void initactiontypes();
void initactionblocktypes();

class ai_action {
public:
	float evaluation = FLT_MIN;
	const ai_action_spec* const __restrict type;
	ai_parameters parameters;
	ai_blocks_list blocks;
	ai_action(const ai_action_spec* const __restrict t) : type(t) {};
};

template <>
struct std::hash<ai_action> {
	std::size_t operator()(IN(ai_action) s) const {
		return std::hash<void*>()((void*)s.type);
	}
};

template <>
struct std::hash<ai_action_block> {
	std::size_t operator()(IN(ai_action_block) s) const {
		return std::hash<void*>()((void*)s.type);
	}
};

using ai_plan = ai_action_list;

class ai_collection {
public:
	ai_action_prefs prf;
	interest_manager interests;
	std::vector<ai_plan, concurrent_allocator<ai_plan>> activeplans;

	void addPerception(size_t type, const float basedesire, ai_action_param param);
	void update(const char_id self, IN(g_lock) l);
};