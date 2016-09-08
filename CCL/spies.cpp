#include "globalhelpers.h"
#include "spies.h"
#include "structs.hpp"
#include "living_data.h"
#include "i18n.h"
#include "datamanagement.hpp"
#include "relations.h"
#include "traits.h"
#include "schedule.h"
#include "finances.h"
#include "generated_ui.h"
#include "actions.h"
#include "court.h"
#include "relations.h"
#include "reputation.h"

void execute_plot_event(admin_id_t from, INOUT(spy_mission) sm, IN(plot_event) p, INOUT(w_lock) l) noexcept {
	IN(auto) adm = get_object(from, l);
	if (get_spy_decider(adm, l) == global::playerid) {
		l.unlock();
		if (make_yes_no_popup(global::uicontainer, get_simple_string(TX_I_OPPORTUNITY), [&sm, &p, &adm](IN(std::shared_ptr<uiScrollView>) sv, IN(std::shared_ptr<ui_stress_button>) y, IN(std::shared_ptr<ui_stress_button>) n) noexcept {
			size_t params[] = {sm.spy.value, to_param(p.cost)};
			sv->subelements.push_back(create_tex_block(p.text, params, 2, sv, 1, 1, sv->pos.width - 11, global::empty, global::standard_text));
	
			with_udata(head_of_state(adm), r_lock(), [&p, &y](IN(udata) ud) noexcept {
				if (ud.wealth < p.cost)
					y->disable(get_simple_string(TX_NO_MONEY));
			});
		}, 0.0)) {
			l.lock();
			with_udata(head_of_state(adm), l, [&p, &sm](INOUT(untitled_data) ud) noexcept {
				ud.wealth -= p.cost;

				size_t params[] = {sm.spy.value};
				if (global_store.get_double() <= p.base_chance) {
					sm.plot_bonus += p.plot_bonus;
					i18n_message_popup(TX_I_OPPORTUNITY, p.success, params, 1);
				} else {
					i18n_message_popup(TX_I_OPPORTUNITY, p.failure, params, 1);
				}
			});
		} else {
			l.lock();
		}
	} else {
		with_udata(head_of_state(adm), l, [&p, &sm](INOUT(untitled_data) ud) noexcept {
			if (ud.wealth >= p.cost) {
				ud.wealth -= p.cost;
				if (global_store.get_double() <= p.base_chance) {
					sm.plot_bonus += p.plot_bonus;
				}
			}
		});
	}
}

bool can_be_spy(char_id_t id, IN(g_lock) l) noexcept {
	return with_udata(id, l, false, [id](IN(untitled_data) u) noexcept {
		if ((u.flags & P_FLAG_ON_MISSION) != 0)
			return false;
		if (global::currentday - get_object(id).born <= 365 * 18)
			return false;
		return true;
	});
}

bool recruit_plot_member(admin_id_t source, INOUT(spy_mission) sm, IN(plot_advancement) p, IN(plot_type) plottype, INOUT(w_lock) l, INOUT(int) intrigue) noexcept {
	std::vector<char_id_t> courtlst;
	global::get_extended_court(char_id_t(sm.target), courtlst, l);
	vector_erase_if(courtlst, [&sm, &l](char_id_t id) noexcept {
		return
			!can_be_spy(id, l)
			|| id == sm.target
			|| (std::find(std::begin(sm.people_involved), std::end(sm.people_involved), id) != std::end(sm.people_involved))
			|| (std::find(std::begin(sm.people_tried), std::end(sm.people_tried), id) != std::end(sm.people_tried));
	});

	const auto selected = courtlst.size() != 0 ? courtlst[global_store.get_fast_int() % courtlst.size()] : char_id_t();
	const auto prov = (!valid_ids(selected)) ? global::get_home_province(sm.target, l) : prov_id_t();
	
	//const auto sd = get_udata(selected, l);
	const float bias = with_udata(selected, l, 0.0f, [&p](IN(untitled_data) sd) {
		return is_deceitful(sd) ? 0.1f : 0.0f
			+ is_honest(sd) ? -0.5f : 0.0f
			+ ((p.violent) && is_cruel(sd)) ? 0.2f : 0.0f
			+ ((p.violent) && is_kind(sd)) ? -0.7f : 0.0f;
	});

	bool proceed = false;
	const float opin = valid_ids(selected) ? opinion(selected, sm.target, l) : tanh(base_opinon_as_generic(prov, sm.target, l));

	if ((selected == global::playerid) & valid_ids(selected)) {
		size_t params[] = {sm.target.value, p.goal, plottype.name};
		int player_bias = with_udata(char_id_t(global::playerid), l, 0, [&p, opin](IN(untitled_data) pd) noexcept {
			return (opin < -0.5f ? 2 : 0)
				+ (opin > 0.5f ? -3 : 0)
				+ (is_honest(pd) ? -1 : 0)
				+ (((p.violent) && is_cruel(pd)) ? 1 : 0)
				+ (((p.violent) && is_kind(pd)) ? -5 : 0);
		}); 
		l.unlock();
		proceed = i18n_yes_no_popup(TX_I_EVENT, TX_I_RECRUIT_PLAYER, player_bias, params, 3);
		l.lock();
	} else {
		proceed = (opin - bias) < 0.0f;
		if (!proceed) {
			const float spy_intrigue = with_udata(sm.spy, l, 0.0f, [](IN(untitled_data) d) noexcept {
				return static_cast<float>(d.stats.intrigue) / 8.0f;
			});
			const float sw = (valid_ids(selected) ? opinion(selected, sm.spy, l) : tanh(base_opinon_as_generic(prov_id_t(prov), sm.spy, l))) + 1.0f + bias + spy_intrigue;
			proceed = (global_store.get_float() * (sw + opin + 1.0f)) <= sw;
		}
	}

	IN(auto) adm = get_object(source, l);

	if (proceed) {
		for (size_t i = 0; i < MAX_PEOPLE_INVOLVED; ++i) {
			if (!valid_ids(sm.people_involved[i])) {
				sm.people_involved[i] = selected;
				break;
			}
		}

		if (get_spy_decider(adm, l) == global::playerid) {
			size_t params[] = {sm.spy.value, selected.value, p.goal};
			i18n_message_popup(TX_I_EVENT, valid_ids(selected) ? TX_I_RECRUIT_S : TX_I_RECRUIT_S_GEN, params, 3);
		}

		return true;
	} else {
		for (size_t i = 0; i < MAX_PEOPLE_TRIED; ++i) {
			if (!valid_ids(sm.people_tried[i])) {
				sm.people_tried[i] = selected;
				break;
			}
		}

		//failure conditions
		sm.plot_bonus -= 1;
		if (get_spy_decider(adm, l) == global::playerid) {
			size_t params[] = {sm.spy.value, selected.value, p.goal};
			i18n_message_popup(TX_I_EVENT, valid_ids(selected) ? TX_I_RECRUIT_F : TX_I_RECRUIT_F_GEN, params, 3);
		}

		return false;
	}
	
}

plot_event pe_servants(TX_I_EVENT_BRIBE_SERVANTS_T, TX_I_EVENT_BRIBE_SERVANTS_S, TX_I_EVENT_BRIBE_SERVANTS_F, 20.0, 75.0, 1.0);
plot_advancement pa_rumors(false, TX_I_RUMOR_GATHER, 10.0, 2.0, 0.1f);
plot_advancement pa_evidence(false, TX_I_PLANT_EVIDENCE, 0.0, 1.0, 0.3f);

#define PA_EVENT 0
#define PA_RUMORS   1
#define PA_EVIDENCE   2

plot_type plot_tyranny(TX_PLOT_TYRANNY, 2.0, [](admin_id_t from, IN(spy_mission) sm, IN(w_lock) l) noexcept {
	with_udata(sm.target, l, [&sm, from, &l](INOUT(untitled_data) ld) {
		ld.p_just -= std::min(0.1, ld.p_just * 0.2);
		IN(auto) adm = get_object(from, l);
		if (head_of_state(adm) == global::playerid || get_spy_decider(adm, l) == global::playerid) {
			size_t params[] = {sm.spy.value, sm.target.value};
			i18n_message_popup(TX_I_EVENT, TX_I_TYRANY_SRESULT, params, 2);
		} else if (sm.target == global::playerid) {
			i18n_message_popup(TX_I_EVENT, TX_I_TYRANY_TRESULT);
		}
	});
}, {PA_EVENT, PA_RUMORS, PA_EVIDENCE});

plot_type plot_aggression(TX_PLOT_AGGRESSION, 2.0, [](admin_id_t from, IN(spy_mission) sm, IN(w_lock) l) noexcept {
	with_udata(sm.target, l, [&sm, from, &l](INOUT(untitled_data) ld) {
		ld.p_peaceful -= std::min(0.1, ld.p_peaceful * 0.2);
		IN(auto) adm = get_object(from, l);
		if (head_of_state(adm) == global::playerid || get_spy_decider(adm, l) == global::playerid) {
			size_t params[] = {sm.spy.value, sm.target.value};
			i18n_message_popup(TX_I_EVENT, TX_I_AGGRESSION_SRESULT, params, 2);
		} else if (sm.target == global::playerid) {
			i18n_message_popup(TX_I_EVENT, TX_I_AGGRESSION_TRESULT);
		}
	});
}, {PA_EVENT, PA_RUMORS, PA_EVIDENCE});

plot_type plot_dishonorable(TX_PLOT_DISHONORABLE, 4.0, [](admin_id_t from, IN(spy_mission) sm, IN(w_lock) l) noexcept {
	with_udata(sm.target, l, [&sm, from, &l](INOUT(untitled_data) ld) {
		ld.p_honorable -= std::min(0.1, ld.p_honorable * 0.2);
		IN(auto) adm = get_object(from, l);
		if (head_of_state(adm) == global::playerid || get_spy_decider(adm, l) == global::playerid) {
			size_t params[] = {sm.spy.value, sm.target.value};
			i18n_message_popup(TX_I_EVENT, TX_I_DISHONORABLE_SRESULT, params, 2);
		} else if (sm.target == global::playerid) {
			i18n_message_popup(TX_I_EVENT, TX_I_DISHONORABLE_TRESULT);
		}
	});
}, {PA_EVENT, PA_RUMORS, PA_EVIDENCE});

plot_event* pevent_list[] = {&pe_servants};
plot_advancement* padv_list[] = {&pa_rumors, &pa_evidence};
plot_type* ptype_list[] = {&plot_tyranny, &plot_aggression, &plot_dishonorable};

double effective_intrigue(char_id_t ch, IN(g_lock) l) noexcept {
	const auto pa = get_prime_admin(ch, l);
	if (!valid_ids(pa)) {
		return with_udata(ch, l, 0.0, [](IN(untitled_data) d) noexcept {
			return static_cast<double>(d.stats.intrigue);
		});
	}
	return get_object(pa, l).stats.get_intrigue();
}

bool execute_plot_stage(admin_id_t from, INOUT(spy_mission) sm, IN(plot_type) plottype, INOUT(w_lock) l) noexcept {
	if (get_object(sm.spy).died != 0 || get_object(sm.target).died != 0)
		return true;
	
	IN(auto) adm = get_object(from, l);
	const auto hos = head_of_state(adm);

	if (plottype.stages[sm.stage] == 0) {
		size_t eseen = 0;
		for (size_t i = 0; i < MAX_EVENTS_SEEN; ++i) {
			if (sm.events_seen[i] == 0)
				break;
			++eseen;
		}
		unsigned char chosen;
		if (eseen >= std::extent<decltype(pevent_list)>::value) {
			chosen = static_cast<unsigned char>((global_store.get_fast_int() % std::extent<decltype(pevent_list)>::value) + 1);
		} else {
			chosen = static_cast<unsigned char>((global_store.get_fast_int() % (std::extent<decltype(pevent_list)>::value - eseen)) + 1);
			for (size_t i = 0; i < MAX_EVENTS_SEEN; ++i) {
				if (sm.events_seen[i] == 0) {
					sm.events_seen[i] = chosen;
					std::sort(sm.events_seen, &sm.events_seen[i + 1]);
					break;
				}
				if (sm.events_seen[i] != 0 && sm.events_seen[i] <= chosen) {
					++chosen;
				}
			}
		}

		execute_plot_event(from, sm, plot_event_by_type(chosen), l);
		++sm.stage;
	} else {
		IN(auto) adv = plot_adv_by_type(plottype.stages[sm.stage]);

		int intrigueadded = 0;
		if (recruit_plot_member(from, sm, adv, plottype, l, intrigueadded)) {
			//check if stage can advance
			const double target_intrigue = effective_intrigue(sm.target, l);
			const double spy_intrigue = with_udata(sm.spy, l, 0.0, [](IN(untitled_data) d) noexcept { return static_cast<double>(d.stats.intrigue); });
			const double plot_intrigue = adv.adv_chance_multiplier * (adv.adv_chance_bonus + static_cast<double>(intrigueadded) + sm.plot_bonus + spy_intrigue);

			if (global_store.get_double() * (target_intrigue + plot_intrigue) > target_intrigue) {
				++sm.stage;
			} else if(global_store.get_float() < adv.plot_failure_chance) {
				//plot failure, check for escape
				size_t params[] = {sm.target.value, sm.spy.value, plottype.name};

				const double escape_weight = (sm.plot_bonus + spy_intrigue) * 3.0;
				if (global_store.get_double() * (target_intrigue + escape_weight) > target_intrigue) {
					//escape
					if(hos == global::playerid || get_spy_decider(adm, l) == global::playerid)
						i18n_message_popup(TX_I_EVENT, TX_I_PLOT_FAILURE_ESCAPE, params, 3);
					if (sm.target == global::playerid)
						i18n_message_popup(TX_I_EVENT, TX_I_PLOT_FOILDED_TARGET, &plottype.name, 1);
				} else {
					//capture/death
					if (global_store.get_float() < 0.5f) {
						if (hos == global::playerid || get_spy_decider(adm, l) == global::playerid)
							i18n_message_popup(TX_I_EVENT, TX_I_PLOT_FAILURE_CAPTURE, params, 3);

						//TODO: notify target of capture
					} else {
						if (hos == global::playerid || get_spy_decider(adm, l) == global::playerid)
							i18n_message_popup(TX_I_EVENT, TX_I_PLOT_FAILURE_DEATH, params, 3);

						//TODO: notify target of kill
					}

					//check for reveal
					const double source_intrigue = get_object(from,l).stats.get_intrigue();

					if (global_store.get_double() * (target_intrigue + source_intrigue) <= target_intrigue) {
						if (hos == global::playerid || get_spy_decider(adm, l) == global::playerid)
							i18n_message_popup(TX_I_EVENT, TX_I_PLOT_REVEALED);
						else if (sm.target == global::playerid) {
							size_t iparams[] = {plottype.name, hos.value};
							i18n_message_popup(TX_I_EVENT, TX_I_PLOT_REVEALED_TARGET, iparams, 2);
						}
						adjust_relation(sm.target, hos, -4, l);
						with_udata(hos, l, [](INOUT(untitled_data) d) noexcept { d.p_honorable = update_reputation(reputation::p_plotting_reliable, reputation::p_plotting_unreliable, d.p_honorable); });
						
					} else {
						//merely foiled
						if (sm.target == global::playerid)
							i18n_message_popup(TX_I_EVENT, TX_I_PLOT_FOILDED_TARGET, &plottype.name, 1);
					}
				}
				return true;
			}
		}
	}

	if (sm.stage == plottype.stages.size()) {
		plottype.finish(from, sm, l);
		return true;
	}
	if (valid_ids(sm.people_tried[MAX_PEOPLE_TRIED - 1])) {
		size_t params[] = {sm.target.value, sm.spy.value, plottype.name};
		if (hos == global::playerid || get_spy_decider(adm, l) == global::playerid)
			i18n_message_popup(TX_I_EVENT, TX_I_PLOT_FAILURE_ESCAPE, params, 3);
		return true;
	}
	return false;
}

v_pool_t<spy_mission, SM_TAG_TYPE> sm_pool;

spy_mission& get_spy_mission(SM_TAG_TYPE tg, IN(g_lock) l) noexcept {
	return sm_pool.get(tg, l);
}

const plot_type& plot_by_type(unsigned char type) noexcept {
	return *(ptype_list[type - 1]);
}

const plot_event& plot_event_by_type(unsigned char type) noexcept {
	return *(pevent_list[type - 1]);
}

const plot_advancement& plot_adv_by_type(unsigned char type) noexcept {
	return *(padv_list[type - 1]);
}


void free_spy_mission(SM_TAG_TYPE id, IN(w_lock) l) noexcept {
	sm_pool.free(id, l);
}


size_t count_spy_missions(admin_id_t a, IN(g_lock) l) noexcept {
	return spy_missions.count(a, l);
}

SM_TAG_TYPE new_spy_mission(char_id_t target, char_id_t spy, unsigned char type, IN(w_lock) l) noexcept {
	return sm_pool.emplace(l, target, spy, type);
}

void smission_display::new_mission_action(ui_button_disable* obj) {
	open_spy_mission_select();
}

void smission_display::disable_new_mission(IN(std::shared_ptr<ui_button_disable>) element, IN(admin_id_t) obj, IN(g_lock) l) {
	if (!new_spy_mission_possible(obj, l))
		element->disable(get_simple_string(TX_NO_MIS_SLOTS));
}

std::function<void(uiButton*)> spy_mission_ui::cancel_action(IN(spy_mission) obj, IN(g_lock) l) {
	SM_TAG_TYPE mission_number = sm_pool.get_index(obj, l);
	const auto pa = get_prime_admin(char_id_t(global::playerid), l);
	return [mission_number, pa](IN_P(uiButton) b) noexcept { global::actionlist.add_new<cancel_spy_mission>(char_id_t(global::playerid), pa, mission_number); };
}

void update_spy_mission(admin_id_t owner, SM_TAG_TYPE tg, INOUT(w_lock) l) noexcept {
	INOUT(auto) sm = sm_pool.get(tg, l);

	IN(auto) plottype = plot_by_type(sm.type);
	if (execute_plot_stage(owner, sm, plottype, l)) {
		//plot finished
		with_udata(sm.spy, l, [](INOUT(untitled_data) d) noexcept { d.flags &= ~P_FLAG_ON_MISSION; });
		remove_expense(head_of_state(owner, l), EXPENSE_ESPIONAGE, plottype.monthly_cost, l);

		sm_pool.free(tg, l);
		spy_missions.erase(owner, tg, l);
	} else {
		//schedule next stage
		global::schedule.emplace(global::currentday + 31, new s_sm_stage(owner, tg));
	}

}

void terminate_spy_mission(admin_id_t owner, SM_TAG_TYPE tg, INOUT(w_lock) l) noexcept {
	INOUT(auto) sm = sm_pool.get(tg, l);
		
	auto cr = global::schedule.begin();
	const auto ubound = global::schedule.upper_bound(global::currentday + 32);
	for (; cr != ubound; ++cr) {
		if (cr->second->type == S_TYPE_SM_STAGE) {
			IN_P(auto) se = static_cast<s_sm_stage*>(cr->second.get());
			if (se->index == tg && owner == se->source) {
				se->type = S_TYPE_ERASED;
			}
		}
	}

	with_udata(sm.spy, l, [](INOUT(untitled_data) d) noexcept { d.flags &= ~P_FLAG_ON_MISSION; });
	remove_expense(head_of_state(owner, l), EXPENSE_ESPIONAGE, plot_by_type(sm.type).monthly_cost, l);

	sm_pool.free(tg, l);
	spy_missions.erase(owner, tg, l);
}

size_t max_spy_missions_a(admin_id_t a, IN(g_lock) l) noexcept {
	return static_cast<size_t>(get_object(a, l).stats.get_intrigue() + 1.0);
}


bool new_spy_mission_possible(admin_id_t source, IN(g_lock) l) noexcept {
	return count_spy_missions(source, l) < max_spy_missions_a(source, l);
}

bool new_spy_mission_possible(admin_id_t source, char_id_t target, unsigned char type, IN(g_lock) l) noexcept {
	if (count_spy_missions(source, l) >= max_spy_missions_a(source, l) || global::get_wealth(head_of_state(source, l), l) < 0.0)
		return false;
	return !plot_exists(source, target, type, l);
}

bool plot_exists(admin_id_t source, char_id_t target, unsigned char type, IN(g_lock) l) noexcept {
	return spy_missions.for_each_breakable(source, l, [&l, target, type](SM_TAG_TYPE t) noexcept {
		IN(auto) m = sm_pool.get(t, l);
		return m.target == target && m.type == type;
	});
}

void add_new_spy_mission(admin_id_t source, char_id_t target, char_id_t spy, unsigned char type, IN(w_lock) l) noexcept {
	if (new_spy_mission_possible(source, target, type, l)) {
		const auto tg = sm_pool.emplace(l, target, spy, type);
		spy_missions.insert(source, tg, l);

		const auto hos = head_of_state(source, l);
		add_expense(hos, EXPENSE_ESPIONAGE, 0, plot_by_type(type).monthly_cost, l);
		global::schedule.emplace(global::days_between(hos, target, l) + 31 + global::currentday, new s_sm_stage(source, tg));
	}
	return;
}

smission_selection sm_selection_data;

void sm_select::spy_c_select_list(INOUT(cvector<char_id_t>) vec) {
	r_lock l;
	const auto pa = get_prime_admin(char_id_t(global::playerid), l);
	if (valid_ids(pa)) {
		enum_court_a(pa, vec, l);
		vector_erase_if(vec, [&l](char_id_t id) noexcept {
			return !can_be_spy(id, l);
		});
	}
}

void sm_select::spy_c_select_action(char_id_t id) {
	sm_selection_data.spy = id.value;
	smission_selection_rec.needs_update = true;
}

void sm_select::target_c_select_list(INOUT(cvector<char_id_t>) vec) {
	global::get_recent_people(vec);
	vector_erase(vec, char_id_t(global::playerid));
}

void sm_select::target_c_select_action(char_id_t id) {
	sm_selection_data.target = id.value;
	smission_selection_rec.needs_update = true;
}

void sm_select::mission_type_options(IN(std::shared_ptr<uiDropDown>) dd, IN(smission_selection) obj, IN(g_lock) l) {
	for (unsigned char i = PLOT_TYRANNY; i <= PLOT_DISHONORERABLE; ++i) {
		dd->add_option(get_simple_string(plot_by_type(i).name), i);
		if (sm_selection_data.type == 0)
			sm_selection_data.type = static_cast<unsigned char>(i);
	}

	dd->chooseaction = [](uiDropDown* dd, int option) noexcept {
		sm_selection_data.type = static_cast<unsigned char>(option);
		smission_selection_rec.needs_update = true;
	};
}

void sm_select::start_mission_action(ui_button_disable* obj) {
	global::actionlist.add_new<setup_spy_mission>(global::playerid, get_prime_admin(global::playerid, r_lock()), sm_selection_data.target, sm_selection_data.spy, sm_selection_data.type);
	sm_select_window->setVisible(false);
}

void sm_select::disable_start_mission(IN(std::shared_ptr<ui_button_disable>) element, IN(smission_selection) obj, IN(g_lock) l) {
	if (!new_spy_mission_possible(get_prime_admin(global::playerid, l), l))
		element->disable(get_simple_string(TX_NO_MIS_SLOTS));
	if (!valid_ids(sm_selection_data.target))
		element->disable(get_simple_string(TX_NO_TARGET));
	if (sm_selection_data.type == 0)
		element->disable(get_simple_string(TX_NO_MISSION));
	if (plot_exists(get_prime_admin(global::playerid, l), sm_selection_data.target, sm_selection_data.type, l)) {
		element->disable(get_simple_string(TX_MISSION_EXISTS));
	}
}

void sm_select::cancel_action(uiButton* obj) {
	sm_select_window->setVisible(false);
}

void open_spy_mission_select() noexcept {
	sm_select::sm_select_window->setVisible(true);
	sm_selection_data.spy = char_id_t::NONE;
	sm_selection_data.target = char_id_t::NONE;
	sm_selection_data.type = 0;
	smission_selection_rec.needs_update = true;

	global::uiqueue.push([] {
		sm_select::sm_select_window->toFront(global::uicontainer);
	});
}

void open_intrigue_window() noexcept {
	smission_display::smission_display_window->setVisible(true);
	smission_display_rec.needs_update = true;

	global::uiqueue.push([] {
		smission_display::smission_display_window->toFront(global::uicontainer);
	});
}

update_record smission_selection_rec([] {
	if (sm_select::sm_select_window->gVisible()) {
		sm_select::update(sm_selection_data, r_lock());
	}
});

update_record smission_display_rec([] {
	if (smission_display::smission_display_window->gVisible()) {
		r_lock l;
		smission_display::update(get_prime_admin(global::playerid, l), l);
	}
});