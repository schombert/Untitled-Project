#include "globalhelpers.h"
#include "WarPane.h"
#include "uielements.hpp"
#include "wardata.h"
#include "structs.hpp"
#include "ChPane.h"
#include "peacewindow.h"
#include "i18n.h"
#include "datamanagement.hpp"
#include "living_data.h"
#include "actions.h"
#include "generated_ui.h"
#include "laws.h"
#include "prov_control.h"
#include <boost/circular_buffer.hpp>

struct war_disp {
	admin_id_t adm_for;
	war_id_t w_for;
};

reader_writer_lock wp_bk_lk;
using wp_blk_type = w_lock_t<&wp_bk_lk>;

boost::circular_buffer_space_optimized<war_disp> warpanebacklist(50);

update_record war_window_rec([]{
	if (war::war_window->gVisible()) {
		war_disp current = war_disp{admin_id_t(), war_id_t()};
		{
			wp_blk_type l;
			if(!warpanebacklist.empty())
				current = warpanebacklist.back();
		}
		if (valid_ids(current.adm_for)) {
			r_lock l;
			war_id_t backup;
			if (wars_involved_in.for_each_breakable(current.adm_for, l, [o = current.w_for, &backup](war_id_t w) { backup = w;  return w == o; })) {
				war::update(current.adm_for, current.w_for, l);
			} else {
				if (global::mapmode == MAP_MODE_WAR || global::mapmode == MAP_MODE_WARSEL) {
					global::map.displayedwar.wid = backup;
					global::map.displayedwar.adm = current.adm_for;
					global::setFlag(FLG_MAP_UPDATE);
				}

				war::update(current.adm_for, backup, l);
			}
		}
	}
});

void war::back_action(uiGButton* obj) {
	war_disp toopen = war_disp{admin_id_t(), war_id_t()};
	{
		wp_blk_type l;
		if (!warpanebacklist.empty()) {
			toopen = warpanebacklist.back();
			warpanebacklist.pop_back();
		}
		if (!warpanebacklist.empty()) {
			toopen = warpanebacklist.back();
			warpanebacklist.pop_back();
		}
	}
	if (valid_ids(toopen.adm_for)) {
		SetupWarPane(toopen.adm_for, toopen.w_for);
	}
}

void SetupWarPane(admin_id_t o, war_id_t w) {
	if (!valid_ids(o)) {
		war::war_window->setVisible(false);
		return;
	}
	warpanebacklist.push_back(war_disp{o, w});
	war::war_window->setVisible(true);
	war_window_rec.needs_update = true;

	if (global::mapmode == MAP_MODE_WAR || global::mapmode == MAP_MODE_WARSEL) {
		global::map.displayedwar.wid = w;
		global::map.displayedwar.adm = o;
		global::setFlag(FLG_MAP_UPDATE);
	}

	global::uiqueue.push([] {
		war::war_window->toFront(global::uicontainer);
	});
}

void war::war_against_options(IN(std::shared_ptr<uiDropDown>) dd, IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l) {
	if (wars_involved_in.count(obj, l) != 0) {
		wars_involved_in.for_each(obj, l, [&dd, obj, &l](war_id_t w) {
			IN(auto) wp = get_object(w, l);
			size_t params[] = {get_object(wp.attacker.primary, l).associated_title.value, get_object(wp.defender.primary, l).associated_title.value};

			dd->add_option(get_p_string(TX_WAR_NAME, params, 2), [obj, w]() {
				SetupWarPane(obj, w);
			});
		});
	} else {
		dd->add_option(get_simple_string(TX_NONE), []() {});
	}

	if (!valid_ids(p1)) {
		dd->text.setString(get_simple_string(TX_WAR_WINDOW_NO_SELECTION));
	} else {
		INOUT(auto) wp = get_object(p1, l);
		size_t params[] = {get_object(wp.attacker.primary, l).associated_title.value, get_object(wp.defender.primary, l).associated_title.value};
		dd->text.setString(get_p_string(TX_WAR_NAME, params, 2));
	}
}

int selected_pane = 0;

void war::info_selection_options(IN(std::shared_ptr<uiDropDown>) dd) {
	dd->add_option(get_simple_string(TX_WAR_WINDOW_OVERVIEW), [](){
		war::content_panes->activate_pane(war::pariticipant_pane);
		selected_pane = 0;
	});
	dd->add_option(get_simple_string(TX_WAR_WINDOW_FRONTS), [](){
		war::content_panes->activate_pane(war::fronts_pane);
		selected_pane = 1;
	});
	if (selected_pane == 0)
		dd->text.setString(get_simple_string(TX_WAR_WINDOW_OVERVIEW));
	else
		dd->text.setString(get_simple_string(TX_WAR_WINDOW_FRONTS));
}

/*
std::function<void(ui_toggle_button*)> front::lock_action(IN(std::pair<newfront, newfront>) obj, IN(bool) p1, IN(war_id_t) p2, IN(admin_id_t) p3, IN(g_lock) l) {
	size_t front = 0;
	IN(auto) wp = get_object(p2, l);

	front = &obj - &wp.fronts[0];

	//for (size_t fn = 0; fn != wp.fronts.size(); ++fn) {
	//	if (&wp.fronts[fn] == &obj)
	//		front = fn;
	//}

	return[wid = p2, aggressor = p1, front](ui_toggle_button* b) {
		r_lock l;
		INOUT(auto) wp = const_cast<war_pair&>(get_object(wid, l));

		if (wp.fronts.size() > front) {
			if(aggressor && get_war_decider(get_object(wp.attacker.primary,l),l) == global::playerid)
				wp.fronts[front].first.locked = !wp.fronts[front].first.locked;
			else if(get_war_decider(get_object(wp.defender.primary, l), l) == global::playerid)
				wp.fronts[front].second.locked = !wp.fronts[front].second.locked;
		}
	};
}


std::function<void(uiGButton*)> front::leftarrow_action(IN(std::pair<newfront, newfront>) obj, IN(bool) p1, IN(war_id_t) p2, IN(admin_id_t) p3, IN(g_lock) l) {
	size_t front = 0;
	IN(auto) wp = get_object(p2, l);
	front = &obj - &wp.fronts[0];

	//for (size_t fn = 0; fn != wp.fronts.size(); ++fn) {
	//	if (wp.fronts[fn] == obj)
	//		front = fn;
	//}

	return[wid = p2, aggressor = p1, front](uiGButton* b) {
		r_lock l;
		IN(auto) wp = get_object(wid, l);

		if (aggressor && get_war_decider(get_object(wp.attacker.primary, l), l) == global::playerid)
			global::actionlist.add_new<adjust_front_allocation>(char_id_t(global::playerid), aggressor, war_id_t(wid), static_cast<unsigned short>(front), 0.0f);
		else if (get_war_decider(get_object(wp.defender.primary, l), l) == global::playerid)
			global::actionlist.add_new<adjust_front_allocation>(char_id_t(global::playerid), aggressor, war_id_t(wid), static_cast<unsigned short>(front), 0.0f);
	};
}

std::function<void(uiBar*, double)> front::bar_action(IN(std::pair<newfront, newfront>) obj, IN(bool) p1, IN(war_id_t) p2, IN(admin_id_t) p3, IN(g_lock) l) {
	size_t front = 0;
	IN(auto) wp = get_object(p2, l);
	front = &obj - &wp.fronts[0];

	//for (size_t fn = 0; fn != wp.fronts.size(); ++fn) {
	//	if (wp.fronts[fn] == obj)
	//		front = fn;
	//}

	return[wid = p2, aggressor = p1, front](uiBar* b, double p) {
		r_lock l;
		IN(auto) wp = get_object(wid, l);

		if (aggressor && get_war_decider(get_object(wp.attacker.primary, l), l) == global::playerid)
			global::actionlist.add_new<adjust_front_allocation>(char_id_t(global::playerid), aggressor, war_id_t(wid), static_cast<unsigned short>(front), static_cast<float>(p));
		else if (get_war_decider(get_object(wp.defender.primary, l), l) == global::playerid)
			global::actionlist.add_new<adjust_front_allocation>(char_id_t(global::playerid), aggressor, war_id_t(wid), static_cast<unsigned short>(front), static_cast<float>(p));
		else
			b->barpos = static_cast<unsigned int>(b->pos.width * (aggressor ? wp.fronts[front].first.allocation : wp.fronts[front].second.allocation));
	};
}

std::function<void(uiGButton*)> front::rightarrow_action(IN(std::pair<newfront, newfront>) obj, IN(bool) p1, IN(war_id_t) p2, IN(admin_id_t) p3, IN(g_lock) l) {
	size_t front = 0;
	IN(auto) wp = get_object(p2, l);
	front = &obj - &wp.fronts[0];

	//for (size_t fn = 0; fn != wp.fronts.size(); ++fn) {
	//	if (wp.fronts[fn] == obj)
	//		front = fn;
	//}

	return[wid = p2, aggressor = p1, front](uiGButton* b) {
		r_lock l;
		IN(auto) wp = get_object(wid, l);

		if (aggressor && get_war_decider(get_object(wp.attacker.primary, l), l) == global::playerid)
			global::actionlist.add_new<adjust_front_allocation>(char_id_t(global::playerid), aggressor, war_id_t(wid), static_cast<unsigned short>(front), 1.0f);
		else if (get_war_decider(get_object(wp.defender.primary, l), l) == global::playerid)
			global::actionlist.add_new<adjust_front_allocation>(char_id_t(global::playerid), aggressor, war_id_t(wid), static_cast<unsigned short>(front), 1.0f);
	};
}
*/

std::function<void(ui_button_disable*)> war::offer_peace_action(IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l) {
	return [p1, obj](ui_button_disable* b) {
		SetupPeaceWindow(p1, obj, false);
	};
}

void war::disable_offer_peace(IN(std::shared_ptr<ui_button_disable>) element, IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l) {
	IN(auto) wp = get_object(p1, l);
	bool attacker = is_agressor(obj, p1, l);

	cvector<prov_id_t> acontrolled;
	cvector<prov_id_t> bcontrolled;

	cflat_set<admin_id_t> all_attackers;
	cflat_set<admin_id_t> all_defenders;

	list_participants(wp.attacker, all_attackers, l);
	list_participants(wp.defender, all_defenders, l);

	for (const auto a : all_attackers)
		controlled_by_admin(a, acontrolled, l);
	for (const auto a : all_defenders)
		controlled_by_admin(a, bcontrolled, l);


	const double warbase = war_base_tax_value(p1, acontrolled, bcontrolled, l);
	const double warprediction = war_prediction_value(p1, attacker, acontrolled, bcontrolled, l);

	if ((attacker && warbase + warprediction > 0) ||
		(!attacker && warbase + warprediction < 0)) {
		element->disable(get_simple_string(TX_INSUF_SCORE));
	}
}

std::function<void(ui_button_disable*)> war::enforce_peace_action(IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l) {
	return [p1, obj](ui_button_disable* b) {
		SetupPeaceWindow(p1, obj, true);
	};
}

void war::disable_enforce_peace(IN(std::shared_ptr<ui_button_disable>) element, IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l) {

	IN(auto) wp = get_object(p1, l);
	bool attacker = is_agressor(obj, p1, l);
	IN(auto) aside = attacker ? wp.attacker : wp.defender;
	IN(auto) bside = !attacker ? wp.attacker : wp.defender;

	cflat_set<admin_id_t> all_aside;
	cflat_set<admin_id_t> all_bside;

	list_participants(aside, all_aside, l);
	list_participants(bside, all_bside, l);

	cvector<prov_id_t> bcontrolled;
	for (const auto a : all_bside)
		controlled_by_admin(a, bcontrolled, l);

	if (!can_enforce(bside, all_aside, all_bside, bcontrolled, l)) {
		element->disable(get_simple_string(TX_INSUF_LEVERAGE));
	}
}