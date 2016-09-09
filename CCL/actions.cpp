#include "globalhelpers.h"
#include "datamanagement.hpp"
#include "peace.h"
#include "actions.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "relations.h"
#include "events.h"
#include "prov_control.h"
#include "wardata.h"
#include "reputation.h"
#include "i18n.h"
#include "schedule.h"
#include "requests.h"
#include "envoys.h"
#include "living_data.h"
#include "WarPane.h"
#include "court.h"
#include "generate_character.h"
#include "WarPane.h"
#include "spies.h"
#include "action_opinion.h"


namespace global {
	actionable_list_class<actionbase, apply_actionbase> actionlist;
};

void war_declaration::execute() noexcept {
	{
		w_lock l;
		if (!attack_reaction(actor, target, goal, l))
			return;
		with_udata_2(actor, target, l, [](INOUT(udata) d, INOUT(udata) e) noexcept { d.activity = global::currentday; e.activity = global::currentday; });

		const auto wid = new_war_pair(actor, goal, target, l);
		//INOUT(auto) wp = get_object(wid, l);
	}

	if (global::mapmode == 3 || global::mapmode == 4) {
		global::setFlag(FLG_MAP_UPDATE);
	}
	if (actor == global::playerid || target == global::playerid) {
		war_window_rec.needs_update = true;
		global::setFlag(FLG_WWIN_UPDATE);
	}


	if (global::interested.in_set(actor.value) || global::interested.in_set(target.value))
		makeWarDeclaration(get_object(actor).primetitle.value, get_object(target).primetitle.value, goal.type);

}

bool war_declaration::possible( IN(g_lock) l) const noexcept {
	cflat_set<admin_id_t> tcontrolled;
	list_direct_controlled_admin(target, tcontrolled, l);
	if (!wargoal_possible(goal, actor, tcontrolled, l)) {
		return false;
	}
	
	const auto tprime = get_prime_leige(target, l);
	if (!can_attack(actor, target, l))
		return false;

	if (in_war_against(actor, target, l))
		return false;

	return true;
}

void execute_actions() noexcept {
	global::actionlist.execute_entries();
}


void implement_peace_offer_fac(char_id_t a, peace_deal &&b) noexcept {
	global::actionlist.add_new<implement_peace_offer>(a, std::move(b));
}

void transfer_prov_a(prov_id_t prov, admin_id_t to, INOUT(flat_set<char_id_t>) losers, IN(w_lock) l) noexcept {
	global::enum_control_by_prov(prov, l, [&losers,&l](IN(controlrecord) r){
		losers.insert(get_object(get_object(r.ad_controller,l).associated_title).holder);
	});

	global::update_prov_control(prov, to, l);
}

void planned_event::execute() noexcept {
	schedule_event(id, actor, target, w_lock());
}

bool planned_event::possible( IN(g_lock) l) const noexcept {
	return with_udata(actor, l, false, [](IN(udata) d) noexcept { return (d.flags & P_FLAG_PERPARING_EVENT) == 0; });
}

void setup_envoy_mission::execute() noexcept {
	
	fake_lock l;

	const auto i = first_free_e_index(associated_a, l);
	if (i < max_envoy_missions_a(associated_a, l)) {
		newmission->index = i;

		if (!valid_ids(newmission->envoy)) {
			w_lock wlk;
			std::vector<char_id_t> v;
			enum_court_a(associated_a, v, wlk);
			vector_erase_if(v, [&l](char_id_t id) {
				return !can_be_envoy(id, l);
			});
			if (v.size() != 0) {
				newmission->envoy = v[global_store.get_fast_int() % v.size()].value;
			} else {
				newmission->envoy = generate_courtier_a(associated_a, wlk).value;
			}
			newmission->start_mission(actor, associated_a, global::currentday, wlk);
			envoy_missions.emplace(associated_a, wlk, std::move(newmission));
		}

		
		with_udata(actor, l, [](INOUT(udata) d) noexcept { d.activity = global::currentday; });
	
		if (actor == global::playerid)
			emission_display_rec.needs_update = true;
		
	}
}

bool setup_envoy_mission::possible(IN(g_lock) l) const noexcept {
	if (valid_ids(newmission->envoy) && with_udata(newmission->envoy, l, true, [](IN(udata) d) noexcept { return (d.flags & P_FLAG_ON_MISSION) != 0; })) {
		return false;
	}

	if (actor != global::playerid) {
		return ! envoy_missions.for_each_breakable(associated_a, l, [self = this](IN(concurrent_uniq<envoy_mission>) m) {
			return m->active && m->expiration_date >= global::currentday && m->identical(*(self->newmission.get()));
		});
	} else {
		envoy_missions.for_each(associated_a, l, [self = this](IN(concurrent_uniq<envoy_mission>) m) {
			if(m->active && m->expiration_date >= global::currentday && m->identical(*(self->newmission.get()))) {
				cancel_envoy_mission cm(self->actor, self->associated_a, m->index);
				cm.execute();
			}
		});
		return true;
	}
	
}

void cancel_envoy_mission::execute() noexcept {
	if (auto sm = get_envoy_mission(associated_a, mission_number, fake_lock())) {
		const auto ubound = global::schedule.upper_bound(sm->return_date+1);
		for (auto cr = global::schedule.begin(); cr != ubound; ++cr) {
			if (cr->second->type == S_TYPE_MISSION_EXEC) {
				IN_P(auto) se = static_cast<s_pact_mission_exec*>(cr->second.get());
				if (se->index == mission_number && associated_a == se->adm) {
					se->type = S_TYPE_ERASED;
				}
			}
		}

		const auto rangeb = global::schedule.equal_range(sm->expiration_date);
		for (auto it = rangeb.first; it != rangeb.second; ++it) {
			if (it->second->type == S_TYPE_MISSION_EXPIRE) {
				IN_P(auto) se = static_cast<s_mission_expire*>(it->second.get());
				if (se->index == mission_number && associated_a == se->adm) {
					se->type = S_TYPE_ERASED;
				}
			}
		}

		sm->expire_mission(associated_a, fake_lock());

		envoy_missions.range_erase_if(associated_a, w_lock(), [m = this->mission_number](IN(std::pair<admin_id_t, concurrent_uniq<envoy_mission>>) pr) {
			return pr.second->index == m;
		});
		
		if (actor == global::playerid)
			emission_display_rec.needs_update = true;
	}
}

void accept_envoy_offer::execute() noexcept {
	if (auto sm = get_envoy_mission(associated_admin, mission_number, fake_lock())) {
		bool res;
		{
			w_lock l;
			res = sm->accept_offer(associated_admin, actor, offer_number, l);
		}
		if(res) {
			cancel_envoy_mission cm(actor, associated_admin, mission_number);
			cm.execute();
		}
	}
}

void implement_peace_offer::execute() noexcept {
	{
		w_lock wl;
		deal.implement_offer(wl);
	}

	if (global::mapmode == MAP_MODE_WAR || global::mapmode == MAP_MODE_WARSEL) {
		global::setFlag(FLG_MAP_UPDATE);
	}

	war_window_rec.needs_update = true;
}

void relation_change::execute() noexcept {
	if (!symmetric) {
		adjust_relation(actor, secondary, value, w_lock());
	} else {
		adjust_relation_symmetric(actor, secondary, value, w_lock());
	}
}

bool relation_change::possible(IN(g_lock) l) const noexcept {
	return get_object(actor).died == 0 && get_object(secondary).died == 0;
}

void adjust_front_allocation::execute() noexcept {
	{
		w_lock l;
		INOUT(auto) wp = war_pool.get(this->war_for.value, l);
		adjust_alloc_to(front, wp, attacker, target, w_lock());
	}
		
	war_window_rec.needs_update = true;
		
}

void setup_spy_mission::execute() noexcept {
	w_lock l;
	char_id_t tspy(spy);

	if (!valid_ids(tspy)) {
		std::vector<char_id_t> v;
		enum_court_a(get_prime_admin(actor, l), v, l);
		IN(auto) act = get_object(actor);

		vector_erase_if(v, [&l, dyn = act.dynasty](char_id_t id) {
			return (!can_be_spy(id, l)) | ((dyn == get_object(id).dynasty) & valid_ids(dyn));
		});
		if (v.size() != 0) {
			tspy = v[global_store.get_fast_int() % v.size()].value;
		} else if (valid_ids(act.primetitle)) {
			tspy = generate_courtier_a(get_associated_admin(actor, l), l).value;
		} else {
			return;
		}
	}

	with_udata(tspy, l, [](INOUT(untitled_data) d) { d.flags |= P_FLAG_ON_MISSION; });

	add_new_spy_mission(admsource, char_id_t(target), tspy, type, l);

	with_udata(char_id_t(actor), l, [](INOUT(udata) d) noexcept { d.activity = global::currentday; });

	if (actor == global::playerid)
		smission_display_rec.needs_update = true;
}

bool setup_spy_mission::possible(IN(g_lock) l) const noexcept {
	return new_spy_mission_possible(admsource, char_id_t(target), type, l) && (!valid_ids(spy) || can_be_spy(spy, l));
}

void cancel_spy_mission::execute() noexcept {
	w_lock l;
	terminate_spy_mission(admsource, mission_number, l);
	
	if (actor == global::playerid)
		smission_display_rec.needs_update = true;
}

void break_pact_a::execute() noexcept {
	w_lock l;
	INOUT(auto) pct = get_object(pactid, l);
	break_pact(pct, actor, l);
	global::flag_ch_update(actor.value);
}
