#include "globalhelpers.h"
#include "political_action_instances.h"
#include "action_opinion.h"
#include "WarPane.h"

bool pa_war_declaration::is_possible(IN(g_lock) l) const {
	cflat_set<admin_id_t> tcontrolled;
	const auto actor = head_of_state(associated_admin, l);

	list_direct_controlled_admin(target, tcontrolled, l);
	if (!wargoal_possible(wg, actor, tcontrolled, l)) {
		return false;
	}

	const auto tprime = get_prime_leige(target, l);
	if (!can_attack(actor, target, l))
		return false;

	if (in_war_against(actor, target, l))
		return false;

	return true;
}

void pa_war_declaration::display_description(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(g_lock) l) const {
	//TODO
};

void pa_war_declaration::do_action(INOUT(w_lock) l) const {
	const auto actor = head_of_state(associated_admin, l);

	with_udata_2(actor, target, l, [](INOUT(udata) d, INOUT(udata) e) noexcept { d.activity = global::currentday; e.activity = global::currentday; });

	const auto wid = new_war_pair(actor, wg, target, l);

	if (global::mapmode == 3 || global::mapmode == 4) {
		global::setFlag(FLG_MAP_UPDATE);
	}

	if (actor == global::playerid || target == global::playerid) {
		war_window_rec.needs_update = true;
		global::setFlag(FLG_WWIN_UPDATE);
	}


	if (global::interested.in_set(actor.value) || global::interested.in_set(target.value))
		makeWarDeclaration(get_object(actor).primetitle.value, get_object(target).primetitle.value, wg.type);

}

double pa_war_declaration::evaluate_action(char_id_t by, IN(g_lock) l) const {
	return attack_opinion(by, head_of_state(associated_admin, l), target, wg, l);
}
