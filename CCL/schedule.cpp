#include "globalhelpers.h"
#include "schedule.h"
#include "structs.hpp"
#include "envoys.h"
#include "events.h"
#include "living_data.h"
#include "wardata.h"
#include "i18n.h"
#include "WarPane.h"
#include "spies.h"

void s_do_battle(IN(s_battle) battle) noexcept {
	fake_lock l;
	INOUT(auto) wp = war_pool.get(battle.war_for.value, l);

	{
		w_lock wl;

		flat_set<admin_id_t> all_attackers;
		flat_set<admin_id_t> all_defenders;
		list_participants(wp.attacker, all_attackers, wl);
		list_participants(wp.defender, all_defenders, wl);

		const double ttroopa = std::max(static_cast<double>(totaltroops(all_attackers, wl)), battle.lossb);
		const double ttroopb = std::max(static_cast<double>(totaltroops(all_defenders, wl)), battle.loss);

		if(battle.lossb > 0.0)
			reduce_troops(all_attackers, battle.lossb / ttroopa, wl);
		if (battle.loss > 0.0)
			reduce_troops(all_defenders, battle.loss / ttroopb, wl);
	}

	const auto cha = head_of_state(wp.attacker.primary, l);
	const auto chb = head_of_state(wp.defender.primary, l);

	if (battle.lossb + battle.loss > 10 && (global::interested.in_set(cha.value) || global::interested.in_set(chb.value))) {
		if (battle.attackers_won)
				makeBattleResults(get_object(cha).primetitle.value, get_object(chb).primetitle.value, static_cast<int>(battle.loss), static_cast<int>(battle.lossb));
			else
				makeBattleResults(get_object(chb).primetitle.value, get_object(cha).primetitle.value, static_cast<int>(battle.lossb), static_cast<int>(battle.loss));
	}

	global::setFlag(FLG_WWIN_UPDATE);
	war_window_rec.needs_update = true;
}

void s_do_seige(IN(s_seige) seige) noexcept {
	fake_lock l;
	INOUT(auto) wp = get_object(seige.war_for, l);

	{
		w_lock wl;
		
		if (occupation_info.contains(seige.sprov, wl)) {
			INOUT(auto) inf = occupation_info.get(seige.sprov, wl);
			if (inf.lifting) {
				occupation_info.erase(seige.sprov, wl);
			} else {
				inf.since = 0;
			}
		}
	}

	if (global::mapmode == 3 || global::mapmode == 4) {
		global::setFlag(FLG_MAP_UPDATE);
	}

	const auto cha = head_of_state(wp.attacker.primary, l);
	const auto chb = head_of_state(wp.defender.primary, l);

	if (global::interested.in_set(cha.value) || global::interested.in_set(chb.value)) {
		if (!prov_belongs_to_attacker(seige.war_for, seige.sprov, l)) {
			makeSeigeResults(get_object(cha).primetitle.value, get_object(chb).primetitle.value, seige.sprov.value, seige.duration);
		} else {
			makeSeigeResults(get_object(chb).primetitle.value, get_object(cha).primetitle.value, seige.sprov.value, seige.duration);
		}
	}

	war_window_rec.needs_update = true;
	global::setFlag(FLG_WWIN_UPDATE);

}

void s_do_event(IN(s_event) ev) noexcept {
	execute_event(ev.event_type, ev.host, ev.target, w_lock());
}

void s_do_mission_expire(IN(s_mission_expire) ex) noexcept {
	remove_envoy_mision(ex.adm, ex.index, w_lock());
	if (ex.source == global::playerid)
		emission_display_rec.needs_update = true;
}

void s_do_mission_exec(IN(s_pact_mission_exec) ex) noexcept {
	const auto ptr = get_envoy_mission(ex.adm, ex.index, fake_lock());
	if(ptr) {
		w_lock l;
		ptr->make_offer(ex.adm, ex.source, ex.target, l);
		ptr->setup_next_offer(ex.source, ex.adm, global::currentday, l);
		if(ex.source == global::playerid)
			emission_display_rec.needs_update = true;
	}
}

void s_do_sm_stage(IN(s_sm_stage) ex) noexcept {
	w_lock l;
	update_spy_mission(ex.source, ex.index, l);
}


void execute_schedule() noexcept {
	static std::vector<std::unique_ptr<s_actionbase>> copies;

	const auto range = global::schedule.equal_range(global::currentday);
	copies.reserve(std::distance(range.first, range.second));
	for (auto it = range.first; it != range.second; ++it) {
		copies.emplace_back(std::move(it->second));
	}
	global::schedule.erase(range.first, range.second);

	for (IN(auto) si : copies) {
		switch (si->type) {
		case S_TYPE_BATTLE:
			s_do_battle(*static_cast<s_battle*>(si.get()));
			break;
		case S_TYPE_SEIGE:
			s_do_seige(*static_cast<s_seige*>(si.get()));
			break;
		case S_TYPE_EVENT:
			s_do_event(*static_cast<s_event*>(si.get()));
			break;
		case S_TYPE_MISSION_EXEC:
			s_do_mission_exec(*static_cast<s_pact_mission_exec*>(si.get()));
			break;
		case S_TYPE_MISSION_EXPIRE:
			s_do_mission_expire(*static_cast<s_mission_expire*>(si.get()));
			break;
		case S_TYPE_SM_STAGE:
			s_do_sm_stage(*static_cast<s_sm_stage*>(si.get()));
			break;
		}
		//delete si;
	}

	copies.clear();
}