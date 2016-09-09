#include "globalhelpers.h"
#include "ai_actions.h"
#include "structs.hpp"
#include "peace.h"
#include "events.h"
#include "relations.h"
#include "wardata.h"
#include "actions.h"

class null_ai_action : public ai_action_spec {
public:
	virtual void takeaction(IN(ai_parameters) parameters, const char_id self) const {
	};
	virtual float evalaction(IN(ai_parameters) parameters, IN(ai_action_prefs) prf, const char_id self) const {
		return 0.0f;
	};
	virtual void enumerateblocks(INOUT(ai_blocks_list) vals, IN(ai_parameters) parameters, const char_id self) const {
	};
};

class declare_war_on : public ai_action_spec {
public:
	virtual void takeaction(IN(ai_parameters) parameters, const char_id self) const {
		global::actionlist.push(new nocbwar(self, parameters[0].person));
	};
	virtual float evalaction(IN(ai_parameters) parameters, IN(ai_action_prefs) prf, const char_id self) const {
		nocbwar wr(self, parameters[0].person);
		fake_lock l;
		if (wr.possible( l)) {
			return (global::GetTroopStr(self, fake_lock()) / (global::GetTroopStr(parameters[0].person, fake_lock()) + global::get_defensive_strength(parameters[0].person, fake_lock()) + global::get_defensive_against(parameters[0].person, self, l))).to_float();
		}
		return 0.0f;
	};
	virtual void enumerateblocks(INOUT(ai_blocks_list) vals, IN(ai_parameters) parameters, const char_id self) const {
		//TODO
		ai_action_param p;

		vals.emplace_back(concurrent_unique<ai_action_block>(idtoactionblockspec(ALREADY_AT_WAR)));

		vals.emplace_back(concurrent_unique<ai_action_block>(idtoactionblockspec(RAISE_MONEY)));
		p.fvalue = (std_fp::from_int(12) * troopCost(global::GetTroopStr(self, fake_lock()).to_int(), 0)).to_float();
		vals.back()->parameters.push_back(p);

		vals.emplace_back(concurrent_unique<ai_action_block>(idtoactionblockspec(RAISE_TROOPS)));
		p.person = parameters[0].person;
		vals.back()->parameters.push_back(p);
		p.ivalue = static_cast<int>(global::GetTroopStr(parameters[0].person, fake_lock()).to_double() * 1.3);
		vals.back()->parameters.push_back(p);
	};
};

class make_peace : public ai_action_spec {
public:
	virtual void takeaction(IN(ai_parameters) parameters, const char_id self) const {
		if (global::people[self].blb) {
			fake_lock l;
			for (const auto& __restrict wr : global::people[self].blb->wars.wars) {
				if (wr->otherside->parent->primary == parameters[0].person) {
					const std_fp taxval = warTaxValue(self, parameters[0].person,l);
					const bool enforceable = canenforce(*wr, l);

					if (taxval >= std_fp::from_int(0) && (enforceable || global::GetWealth(self) < totaltroopcost(self, l) * std_fp::from_int(2))) {
						std::vector<prov_id> nomcontrolled;
						global::GetNomControlled(parameters[0].person, nomcontrolled, l);

						//start adding adjacent
						prov_id cap = global::GetCapital(global::GetPrimtitle(self));
						if (cap != 0) {
							std::sort(nomcontrolled.begin(), nomcontrolled.end(), [&](const prov_id a, const prov_id b) {
								return global::provinces[cap].dsq(global::provinces[a]) < global::provinces[cap].dsq(global::provinces[b]); });
						}


						peace_deal deal;
						deal.init(self, parameters[0].person, *wr, taxval, l);
						deal.generate_offer_to_value(*wr, l);
						/*for (const auto p : nomcontrolled) {
							deal.add_province(p);
							if (!deal.valid(l))
								deal.make_valid(l);
							if (deal.total_value() >= taxval)
								break;
						}

						if (deal.total_value() > taxval && deal.landtransfer->provs.size() > 1)
							deal.remove_province();*/

						if (enforceable) {
							deal.make_demand();
						} else {
							deal.make_offer();
						}

						wr->offeredpeace = true;
					}
				}
			}
		}
	};
	virtual float evalaction(IN(ai_parameters) parameters, IN(ai_action_prefs) prf, const char_id self) const {
		if (global::people[self].blb) {
			fake_lock l;
			for (const auto& __restrict wr : global::people[self].blb->wars.wars) {
				if (wr->otherside->parent->primary == parameters[0].person) {
					std::vector<prov_id> acontrolled;

					global::GetNomControlled(wr->otherside->parent->primary, acontrolled, l);
					for (const auto& __restrict ch : wr->otherside->otherparticipants) {
						global::GetNomControlled(ch.first, acontrolled, l);
					}

					bool alloccupied = true;
					for (const auto p : acontrolled) {
						if (wr->controlled.count(p) == 0) {
							alloccupied = false;
							break;
						}
					}

					if (alloccupied == true)
						return 1.0f;

					const bool lowmoney = global::GetWealth(self) < totaltroopcost(self, l) * std_fp::from_double(1.5);
					if (lowmoney && !wr->offeredpeace)
						return 1.0f;
					if (lowmoney && canenforce(*wr, l))
						return 1.0f;
				}
			}
		}
		return 0.0f;
	};
	virtual void enumerateblocks(INOUT(ai_blocks_list) vals, IN(ai_parameters) parameters, const char_id self) const {
	};
};


class host_private_event_with : public ai_action_spec {
public:
	virtual void takeaction(IN(ai_parameters) parameters, const char_id self) const {
		std::vector<char_id> guest;
		guest.push_back(parameters[0].person);
		//and push more??
		global::actionlist.push(new planned_event(self, parameters[1].ivalue, guest));
	};
	virtual float evalaction(IN(ai_parameters) parameters, IN(ai_action_prefs) prf, const char_id self) const {
		//TODO
		return 1.0f;
	};
	virtual void enumerateblocks(INOUT(ai_blocks_list) vals, IN(ai_parameters) parameters, const char_id self) const {
		std::vector<char_id> guest;
		guest.push_back(parameters[0].person);

		//TODO
		ai_action_param p;

		vals.emplace_back(concurrent_unique<ai_action_block>(idtoactionblockspec(RAISE_MONEY)));
		p.fvalue = event_cost(parameters[1].ivalue, guest, self).to_float();
		vals.back()->parameters.push_back(p);
	};
};

class null_ai_action_block : public ai_action_block_spec {
public:
	virtual void enumerateresolutions(INOUT(ai_action_list) vals, IN(ai_parameters), const char_id self) const {
	};
	virtual bool valid(IN(ai_parameters) parameters, const char_id self) const {
		return false;
	};
};

class raise_money : public ai_action_block_spec {
public:
	virtual void enumerateresolutions(INOUT(ai_action_list) vals, IN(ai_parameters) parameters, const char_id self) const {
		//TODO
	};
	virtual bool valid(IN(ai_parameters) parameters, const char_id self) const {
		return global::GetWealth(self).to_float() < parameters[0].fvalue;
	};
};

#define STAT_SQ(a,b,stat) (std::max(static_cast<int>(global::people[a].stats.stat) - 3, 0)) * (std::max(static_cast<int>(global::people[b].stats.stat) - 3, 0))

class poor_relations_with : public ai_action_block_spec {
	virtual void enumerateresolutions(INOUT(ai_action_list) vals, IN(ai_parameters) parameters, const char_id self) const {
		const auto otherperson = parameters[0].person;
		int analyticvalue = STAT_SQ(otherperson,self, analytic);
		int martialvalue = STAT_SQ(otherperson, self, martial);
		int socialvalue = STAT_SQ(otherperson, self, social);
		if (analyticvalue >= martialvalue && analyticvalue >= socialvalue && analyticvalue > 0) {
			ai_action_param p;
			vals.emplace_back(concurrent_unique<ai_action>(idtoactionspec(HOST_PRIVATE_EVENT_WITH)));
			p.person = otherperson;
			vals.back()->parameters.push_back(p);
			p.ivalue = EVENT_GAMES;
			vals.back()->parameters.push_back(p);
		} else if (martialvalue >= analyticvalue && martialvalue >= socialvalue && martialvalue > 0) {
			ai_action_param p;
			vals.emplace_back(concurrent_unique<ai_action>(idtoactionspec(HOST_PRIVATE_EVENT_WITH)));
			p.person = otherperson;
			vals.back()->parameters.push_back(p);
			p.ivalue = EVENT_HUNT;
			vals.back()->parameters.push_back(p);
		} else if (socialvalue >= analyticvalue && socialvalue >= martialvalue && socialvalue > 0) {
			ai_action_param p;
			vals.emplace_back(concurrent_unique<ai_action>(idtoactionspec(HOST_PRIVATE_EVENT_WITH)));
			p.person = otherperson;
			vals.back()->parameters.push_back(p);
			p.ivalue = EVENT_SMALL_GATHERING;
			vals.back()->parameters.push_back(p);
		}
	};
	virtual bool valid(IN(ai_parameters) parameters, const char_id self) const {
		return get_mutual_feeling(parameters[0].person, self, fake_lock()) == -1;
	};
};

#undef STAT_SQ

class not_friends_with : public poor_relations_with {
	//enumerateresolutions = that of poor_relations_with

	virtual bool valid(IN(ai_parameters) parameters, const char_id self) const {
		return get_mutual_feeling(parameters[0].person, self, fake_lock()) != 1;
	};
};

class raise_troops : public ai_action_block_spec {
public:
	virtual void enumerateresolutions(INOUT(ai_action_list) vals, IN(ai_parameters) parameters, const char_id self) const {
		//TODO
	};
	virtual bool valid(IN(ai_parameters) parameters, const char_id self) const {
		return global::GetTroopStr(self, fake_lock()).to_int() < parameters[1].ivalue;
	};
};

class already_at_war : public ai_action_block_spec {
public:
	virtual void enumerateresolutions(INOUT(ai_action_list) vals, IN(ai_parameters) parameters, const char_id self) const {
	};
	virtual bool valid(IN(ai_parameters) parameters, const char_id self) const {
		return !global::people[self].blb->wars.wars.empty();
	};
};


class occurant_war : public ai_action_block_spec {
public:
	virtual void enumerateresolutions(INOUT(ai_action_list) vals, IN(ai_parameters) parameters, const char_id self) const {
		ai_action_param p;

		vals.emplace_back(concurrent_unique<ai_action>(idtoactionspec(MAKE_PEACE)));
		p.person = parameters[0].person;
		vals.back()->parameters.push_back(p);
	};
	virtual bool valid(IN(ai_parameters) parameters, const char_id self) const {
		if (global::people[self].blb) {
			for (const auto& __restrict wr : global::people[self].blb->wars.wars) {
				if (wr->otherside->parent->primary == parameters[0].person) 
					return true;
			}
		}
		return false;
	};
};


//std::vector<std::unique_ptr<ai_action_spec>> actiontypes;
//std::vector<std::unique_ptr<ai_action_block_spec>> actionblocktypes;

null_ai_action nullaction;
declare_war_on wardec;
make_peace peacedex;
host_private_event_with hostevent;
#define NUM_ACTION_SPECS	4

const ai_action_spec* const __restrict actiontypes[NUM_ACTION_SPECS] = {
	&nullaction,		//	0
	&wardec,			//	1
	&peacedex,			//	2
	&hostevent			//	3
};

null_ai_action_block nullblock;
raise_money raisem;
raise_troops raisetr;
occurant_war occwar;
already_at_war alreadywar;
poor_relations_with poorrel;
not_friends_with notfriend;
#define NUM_ACTION_BLOCKS	7

const ai_action_block_spec* const __restrict actionblocktypes[NUM_ACTION_BLOCKS] = {
	&nullblock,		//	0
	&raisem,		//	1
	&raisetr,		//	2
	&occwar,		//	3
	&alreadywar,	//	4
	&poorrel,		//	5
	&notfriend		//	6
};

const ai_action_spec* idtoactionspec(const size_t id) {
	return actiontypes[id];
}

size_t actionspectoid(const ai_action_spec* const type) {
	for (size_t it = 0; it < NUM_ACTION_SPECS; ++it) {
		if (actiontypes[it] == type) {
			return it;
		}
	}
	return -1;
}

const ai_action_block_spec* idtoactionblockspec(const size_t id) {
	return actionblocktypes[id];
}


size_t actionblockspectoid(const ai_action_block_spec* const type) {
	for (size_t it = 0; it < NUM_ACTION_BLOCKS; ++it) {
		if (actionblocktypes[it] == type) {
			return it;
		}
	}
	return -1;
}

void initactiontypes() {
	/*actiontypes.emplace_back(std::make_unique<null_ai_action>());
	actiontypes.emplace_back(std::make_unique<declare_war_on>());
	actiontypes.emplace_back(std::make_unique<make_peace>());
	actiontypes.emplace_back(std::make_unique<host_private_event_with>());/**/
}

void initactionblocktypes() {
	/*actionblocktypes.emplace_back(std::make_unique<null_ai_action_block>());
	actionblocktypes.emplace_back(std::make_unique<raise_money>());
	actionblocktypes.emplace_back(std::make_unique<raise_troops>());
	actionblocktypes.emplace_back(std::make_unique<occurant_war>());
	actionblocktypes.emplace_back(std::make_unique<already_at_war>());
	actionblocktypes.emplace_back(std::make_unique<poor_relations_with>());
	actionblocktypes.emplace_back(std::make_unique<not_friends_with>());/**/
}

void enum_target_actions(INOUT(ai_action_list) vals, char_id target) {
	ai_action_param p;
	p.person = target;
	vals.emplace_back(concurrent_unique<ai_action>(idtoactionspec(NO_CB_WAR)));
	vals.back()->parameters.push_back(p);
}

void enum_threat_actions(INOUT(ai_action_list) vals, char_id target) {
}

void sortactions(INOUT(ai_action_list) vals, IN(ai_action_prefs) prf, const char_id self, const float basedesire) {
	std::sort(vals.begin(), vals.end(), [&prf, self, basedesire](IN(ai_action_ptr) a, IN(ai_action_ptr) b) {
		if (a->evaluation == FLT_MIN)
			a->evaluation = a->type->evalaction(a->parameters, prf, self) + basedesire;
		if (b->evaluation == FLT_MIN)
			b->evaluation = b->type->evalaction(b->parameters, prf, self) + basedesire;
		return a->evaluation > b->evaluation;
	});
	while (vals.size() > 0 && vals.back()->evaluation <= 0.0f) {
		if(vals.back()->evaluation == FLT_MIN)
			vals.back()->evaluation = vals.back()->type->evalaction(vals.back()->parameters, prf, self) + basedesire;
		else
			vals.pop_back();
	}
	std::reverse(vals.begin(), vals.end());
}

bool ai_update(INOUT(ai_action_list) activeactions, IN(ai_action_prefs) prf, const char_id self) {
	while (activeactions.size() != 0 && !activeactions.back())
		activeactions.pop_back();
	if (activeactions.size() == 0)
		return false;

	const auto& act = activeactions.back();

	bool validblocks = false;
	for (auto& bl : act->blocks) {
		if (bl && bl->type->valid(bl->parameters, self)) {
			validblocks = true;
			break;
		}
	}

	if (!validblocks) {
		act->type->takeaction(act->parameters, self);
		const bool result = act->type != idtoactionspec(PERCEPTION_HOLDER);
		activeactions.pop_back();
		return result;
	} else {
		size_t offset = global::randint().to_uint() % act->blocks.size();
		bool foundaction = false;

		for (size_t indx = 0; indx < act->blocks.size(); ++indx) {
			auto& blk = act->blocks[(indx + offset) % act->blocks.size()];
			if (blk && !blk->initialized) {
				blk->type->enumerateresolutions(blk->resolutions, blk->parameters, self);
				sortactions(blk->resolutions, prf, self, act->evaluation);
				blk->initialized = true;
			}

			if (blk && blk->resolutions.size() > 0) {
				blk->resolutions.back()->type->enumerateblocks(blk->resolutions.back()->blocks, blk->resolutions.back()->parameters, self);
				activeactions.emplace_back(std::move(blk->resolutions.back()));
				blk->resolutions.pop_back();
				foundaction = true;
				break;
			}
		}

		if (!foundaction) {
			// no way to advance this action
			if (act->type == idtoactionspec(PERCEPTION_HOLDER)) {
				for (auto& it : act->blocks) {
					it->initialized = false;
					it->resolutions.clear();
				}
				return true;
			} else {
				activeactions.pop_back();
			}
		}
		return false;
	}
}

void ai_collection::addPerception(size_t type, const float basedesire, ai_action_param param) {
	activeplans.emplace_back();
	ai_plan &it = activeplans.back();
	it.emplace_back(concurrent_unique<ai_action>(idtoactionspec(PERCEPTION_HOLDER)));
	it.back()->evaluation = basedesire;
	ai_action_param p;
	p.ivalue = static_cast<int>(type);
	it.back()->parameters.push_back(p);
	it.back()->blocks.emplace_back(concurrent_unique<ai_action_block>(idtoactionblockspec(type)));
	it.back()->blocks.back()->parameters.push_back(param);
}

#define _STEPS_ 5

void ai_collection::update(const char_id self, IN(g_lock) l) {
	
	std::vector<size_t> mapping(std::max(activeplans.size(),(size_t)1), 0);
	for (size_t z = 0; z < mapping.size(); ++z)
		mapping[z] = z;

	bool once = true;

	for (size_t cnt = 0; cnt < _STEPS_; ++cnt) {
		if (once && activeplans.empty()) {
			ai_action_list vals;
			char_id target = interests.get_random_person();

			if (interests.is_target(target))
				enum_target_actions(vals, target);
			if (interests.is_threat(target))
				enum_threat_actions(vals, target);

			sortactions(vals, prf, self, 0.0f);

			if (!vals.empty()) {
				size_t bits = get_random_store().get_value();
				auto action = &vals.back();
				const size_t sz = vals.size();
				for (size_t i = 0; i < sz; ++i) {
					if ((bits & 0x01) != 0) {
						action = &vals[(sz - 1) - i];
						break;
					} else {
						bits = bits >> 1;
					}
				}

				(*action)->type->enumerateblocks((*action)->blocks, (*action)->parameters, self);
				activeplans.emplace_back();
				activeplans.back().emplace_back(std::move(*action));
			}
		}

		if (!activeplans.empty() && !mapping.empty()) {
			const size_t indx = mapping[cnt % mapping.size()];
			const bool acted = ai_update(activeplans[indx], prf, self); // do only one action per update cycle

			if (activeplans[indx].empty()) {
				activeplans[indx] = std::move(activeplans.back());
				if(mapping.size() > 1)
					mapping.pop_back();
				activeplans.pop_back();

				if(acted)
					once = false;
			} 
			else if (acted) {
				if (activeplans[indx].back()->type != idtoactionspec(PERCEPTION_HOLDER))
					once = false;

				for (size_t i = cnt % mapping.size(); i < mapping.size()-1; ++i)
					mapping[i] = mapping[i + 1];
				mapping.pop_back();
			}
		}
	}
}

#undef _STEPS_
