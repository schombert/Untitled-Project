#include "globalhelpers.h"
#include "generated_ui.h"
#include "i18n.h"
#include "structs.hpp"
#include "living_data.h"
#include "envoys.h"
#include "laws.h"
#include "WarPane.h"
#include "wardata.h"
#include "spies.h"
#include "prov_control.h"
#include "political_action.h"

#pragma  warning(push)
#pragma  warning(disable:4456)

template <typename A, typename B, typename C, typename T, typename ... REST>
T param_four_type(void(*f_ptr)(A, B, C, T, REST ... rest)) { return T(); }

template <typename A, typename B, typename C, typename T, typename ... REST>
T param_four_type(void(*f_ptr)(A, B, C, T&, REST ... rest)) { return T(); }

void init_all_generated_ui() {
	em_selection::init();
	emission_display::init();
	war::init();
	sm_select::init();
	smission_display::init();
	povince_display::init();
	multi_influence::init();
}

std::shared_ptr<uiDragRect> em_selection::em_selection_window;
		std::shared_ptr<uiSimpleText> em_selection::e_2;
	std::shared_ptr<uiSimpleText> em_selection::e_3;
	std::shared_ptr<uiDropDown> em_selection::mission_type;
	std::shared_ptr<uiPanes> em_selection::content_panes;
		std::shared_ptr<uiScrollView> em_selection::def_pact;
		std::shared_ptr<uiScrollView> em_selection::def_against;
		std::shared_ptr<uiScrollView> em_selection::non_aggression;
	std::shared_ptr<ui_button_disable> em_selection::start_mission;
	std::shared_ptr<uiButton> em_selection::cancel;
	std::shared_ptr<uiGButton> em_selection::close_button;

void em_selection::init() {
	int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ em_selection_window = global::uicontainer->add_element<uiDragRect>(x + 0, y + 0, 400, 400, global::solid_border);
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			e_3 = em_selection_window->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_MISSION_TYPE), global::empty, global::standard_text);
			x = e_3->pos.width + e_3->pos.left + 5;
			y += (global::standard_text.csize + 5);
			maxy = std::max(y, maxy);
			y = tempy;
			{
				mission_type = em_selection_window->add_element<uiDropDown>(x + 0, y + 0, 170, 22, global::solid_border, global::standard_text);
				x = mission_type->pos.width + mission_type->pos.left + 5;
				y = mission_type->pos.height + mission_type->pos.top + 5;
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ content_panes = em_selection_window->add_element<uiPanes>(x + 0, y + 0, em_selection_window->pos.width - (x + 0) + -1, em_selection_window->pos.height - (y + 0) + -33);
				content_panes->init(3);
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					def_pact = content_panes->panes[0];
					content_panes->panes[0]->calcTotalHeight();
				}
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					def_against = content_panes->panes[1];
					content_panes->panes[1]->calcTotalHeight();
				}
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					non_aggression = content_panes->panes[2];
					content_panes->panes[2]->calcTotalHeight();
				}
			x = content_panes->pos.width + content_panes->pos.left + 5;
			y = content_panes->pos.height + content_panes->pos.top + 5;
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{
				start_mission = em_selection_window->add_element<ui_button_disable>(x + 10, y + 0, 140, 22, get_simple_string(TX_START_MISSION), paint_states<2>(global::solid_border, global::disabled_fill), global::standard_text, global::tooltip_text, start_mission_action);
				x = start_mission->pos.width + start_mission->pos.left + 5;
				y = start_mission->pos.height + start_mission->pos.top + 5;
			}
			maxy = std::max(y, maxy);
			y = tempy;
			{
				cancel = em_selection_window->add_element<uiButton>(x + 0, y + 0, 80, 22, get_simple_string(TX_CANCEL), global::solid_border, global::standard_text, cancel_action);
				x = cancel->pos.width + cancel->pos.left + 5;
				y = cancel->pos.height + cancel->pos.top + 5;
			}
			em_selection_window->setVisible(false);
		}
	x = em_selection_window->pos.width + em_selection_window->pos.left + 5;
	y = em_selection_window->pos.height + em_selection_window->pos.top + 5;
	}
	close_button = em_selection_window->add_element<uiGButton>(400 - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { em_selection::em_selection_window->setVisible(false); });
}

void em_selection::update(IN(emission_selection) obj, IN(g_lock) l) {
	em_selection_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(obj.envoy)};
			x = (get_linear_ui(TX_EV_LABEL, params, 1,em_selection_window, x + 0, y + 0, global::empty, global::standard_text) + 5);
			y += (global::standard_text.csize + 5);
		}
		if(!valid_ids(obj.envoy)) {
			maxy = std::max(y, maxy);
			y = tempy;
			e_2 = em_selection_window->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_ENOVY_AUTOMATIC), global::empty, global::standard_text);
			x = e_2->pos.width + e_2->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{
			const auto e = character_selection_menu(em_selection_window, x + 0, y + 0, 170, 22, get_simple_string(TX_SELECT_C), global::solid_border, global::standard_text, envoy_c_select_list, envoy_c_select_action);
			x = e->pos.width + e->pos.left + 5;
			y = e->pos.height + e->pos.top + 5;
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		e_3->pos.left = x + 0;
		e_3->pos.top = y + 0;
		x = e_3->pos.width + e_3->pos.left + 5;
		y += (global::standard_text.csize + 5);
		e_3->subelements.clear();
		em_selection_window->subelements.push_back(e_3);
		maxy = std::max(y, maxy);
		y = tempy;
		mission_type->pos.left = x + 0;
		mission_type->pos.top = y + 0;
		x = mission_type->pos.width + mission_type->pos.left + 5;
		y = mission_type->pos.height + mission_type->pos.top + 5;
		mission_type->subelements.clear();
		em_selection_window->subelements.push_back(mission_type);
		mission_type->reset_options();
		mission_type_options(mission_type, obj, l);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		content_panes->pos.left = x + 0;
		content_panes->pos.top = y + 0;
		content_panes->pos.width = em_selection_window->pos.width - (x + 0) + -1;
		content_panes->pos.height = em_selection_window->pos.height - (y + 0) + -33;
		content_panes->update_size();
		x = content_panes->pos.width + content_panes->pos.left + 5;
		y = content_panes->pos.height + content_panes->pos.top + 5;
		def_pact->subelements.clear();
		def_pact->calcTotalHeight();
		def_against->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ size_t params[1] = {to_param(obj.target)};
				x = (get_linear_ui(TX_E_AGAINST, params, 1,def_against, x + 0, y + 0, global::empty, global::standard_text) + 5);
				y += (global::standard_text.csize + 5);
			}
			maxy = std::max(y, maxy);
			y = tempy;
			{
				const auto e = character_selection_menu(def_against, x + 0, y + 0, 170, 22, get_simple_string(TX_SELECT_C), global::solid_border, global::standard_text, def_c_select_list, def_c_select_action);
				x = e->pos.width + e->pos.left + 5;
				y = e->pos.height + e->pos.top + 5;
			}
		}
		def_against->calcTotalHeight();
		non_aggression->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ size_t params[1] = {to_param(obj.target)};
				x = (get_linear_ui(TX_E_TO, params, 1,non_aggression, x + 0, y + 0, global::empty, global::standard_text) + 5);
				y += (global::standard_text.csize + 5);
			}
			maxy = std::max(y, maxy);
			y = tempy;
			{
				const auto e = character_selection_menu(non_aggression, x + 0, y + 0, 170, 22, get_simple_string(TX_SELECT_C), global::solid_border, global::standard_text, na_c_select_list, na_c_select_action);
				x = e->pos.width + e->pos.left + 5;
				y = e->pos.height + e->pos.top + 5;
			}
		}
		non_aggression->calcTotalHeight();
		em_selection_window->subelements.push_back(content_panes);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		start_mission->pos.left = x + 10;
		start_mission->pos.top = y + 0;
		x = start_mission->pos.width + start_mission->pos.left + 5;
		y = start_mission->pos.height + start_mission->pos.top + 5;
		start_mission->subelements.clear();
		em_selection_window->subelements.push_back(start_mission);
		start_mission->enable();
		disable_start_mission(start_mission, obj, l);
		maxy = std::max(y, maxy);
		y = tempy;
		cancel->pos.left = x + 0;
		cancel->pos.top = y + 0;
		x = cancel->pos.width + cancel->pos.left + 5;
		y = cancel->pos.height + cancel->pos.top + 5;
		cancel->subelements.clear();
		em_selection_window->subelements.push_back(cancel);
	}
	em_selection_window->subelements.push_back(close_button);
}

std::shared_ptr<uiDragRect> emission_display::emission_display_window;
	std::shared_ptr<ui_button_disable> emission_display::new_mission;
	std::shared_ptr<uiScrollView> emission_display::contents;
	std::shared_ptr<uiGButton> emission_display::close_button;

void emission_display::init() {
	int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ emission_display_window = global::uicontainer->add_element<uiDragRect>(x + 0, y + 0, 400, 700, global::solid_border);
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{
				new_mission = emission_display_window->add_element<ui_button_disable>(x + 0, y + 0, 150, 25, get_simple_string(TX_NEW_MISSION), paint_states<2>(global::solid_border, global::disabled_fill), global::standard_text, global::tooltip_text, new_mission_action);
				x = new_mission->pos.width + new_mission->pos.left + 5;
				y = new_mission->pos.height + new_mission->pos.top + 5;
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ contents = emission_display_window->add_element<uiScrollView>(x + 0, y + 0, emission_display_window->pos.width - (x + 0) + -1, emission_display_window->pos.height - (y + 0) + -5);
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					contents->calcTotalHeight();
				}
			x = contents->pos.width + contents->pos.left + 5;
			y = contents->pos.height + contents->pos.top + 5;
			}
			emission_display_window->setVisible(false);
		}
	x = emission_display_window->pos.width + emission_display_window->pos.left + 5;
	y = emission_display_window->pos.height + emission_display_window->pos.top + 5;
	}
	close_button = emission_display_window->add_element<uiGButton>(400 - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { emission_display::emission_display_window->setVisible(false); });
}

void emission_display::update(IN(admin_id_t) obj, IN(g_lock) l) {
	emission_display_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(head_of_state(obj, l))};
			x = (get_linear_ui(TX_DIP_MIS_FOR, params, 1,emission_display_window, x + 0, y + 0, global::empty, global::header_text) + 5);
			y += (global::header_text.csize + 5);
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		new_mission->pos.left = x + 0;
		new_mission->pos.top = y + 0;
		x = new_mission->pos.width + new_mission->pos.left + 5;
		y = new_mission->pos.height + new_mission->pos.top + 5;
		new_mission->subelements.clear();
		emission_display_window->subelements.push_back(new_mission);
		new_mission->enable();
		disable_new_mission(new_mission, obj, l);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		contents->pos.left = x + 0;
		contents->pos.top = y + 0;
		contents->pos.width = emission_display_window->pos.width - (x + 0) + -1;
		contents->pos.height = emission_display_window->pos.height - (y + 0) + -5;
		x = contents->pos.width + contents->pos.left + 5;
		y = contents->pos.height + contents->pos.top + 5;
		contents->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ int btempx = x;
				for(auto it = envoy_missions.range(obj,l).first; it != envoy_missions.range(obj,l).second; ++it) {
					x = btempx;
					display_individual_mission(contents, x, y, *it, obj, l);
					y += 5;
				}
			}
		}
		emission_display_window->subelements.push_back(contents);
		contents->calcTotalHeight();
	}
	emission_display_window->subelements.push_back(close_button);
}

void war_participant::generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(std::pair<admin_id_t, std::vector<wargoal>>) obj, IN(g_lock) l) {
	int x = ix; int y = iy; int tempy = iy; int maxy = iy; int basex = ix;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ size_t params[1] = {to_param(get_object(obj.first, l).associated_title)};
		x = (get_linear_ui(TX_L_PARTICIPANT, params, 1,parent, x + 0, y + 0, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	if(obj.second.size() != 0) {
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	x += 20;
	{ int btempx = x;
		for(auto it = std::begin(obj.second); it != std::end(obj.second); ++it) {
			x = btempx;
			display_goal_name(parent, x, y, *it, l);
			y += 5;
		}
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
}

void name::generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(char_id) obj, IN(g_lock) l) {
	int x = ix; int y = iy; int tempy = iy; int maxy = iy; int basex = ix;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{
		const auto tx = parent->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_NAME_HLINK), global::empty, global::standard_text);
		x = tx->pos.width + tx->pos.left + 5;
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
}

std::shared_ptr<uiDragRect> war::war_window;
	std::shared_ptr<uiDropDown> war::war_against;
	std::shared_ptr<uiDropDown> war::info_selection;
	std::shared_ptr<uiPanes> war::content_panes;
		std::shared_ptr<uiScrollView> war::pariticipant_pane;
					std::shared_ptr<ui_button_disable> war::offer_peace;
					std::shared_ptr<ui_button_disable> war::enforce_peace;
				std::shared_ptr<uiScrollView> war::e_16;
				std::shared_ptr<uiScrollView> war::e_18;
		std::shared_ptr<uiScrollView> war::fronts_pane;
	std::shared_ptr<uiGButton> war::back;
	std::shared_ptr<uiGButton> war::close_button;

void war::init() {
	int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ war_window = global::uicontainer->add_element<uiDragRect>(x + 0, y + 0, 600, 400, global::solid_border);
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{
				war_against = war_window->add_element<uiDropDown>(x + 0, y + 0, 370, 22, global::solid_border, global::standard_text);
				x = war_against->pos.width + war_against->pos.left + 5;
				y = war_against->pos.height + war_against->pos.top + 5;
			}
			maxy = std::max(y, maxy);
			y = tempy;
			{
				info_selection = war_window->add_element<uiDropDown>(x + 0, y + 0, 170, 22, global::solid_border, global::standard_text);
				info_selection_options(info_selection);
				x = info_selection->pos.width + info_selection->pos.left + 5;
				y = info_selection->pos.height + info_selection->pos.top + 5;
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ content_panes = war_window->add_element<uiPanes>(x + 0, y + 0, war_window->pos.width - (x + 0) + -1, war_window->pos.height - (y + 0) + -5);
				content_panes->init(2);
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					pariticipant_pane = content_panes->panes[0];
					content_panes->panes[0]->calcTotalHeight();
				}
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					fronts_pane = content_panes->panes[1];
					content_panes->panes[1]->calcTotalHeight();
				}
			x = content_panes->pos.width + content_panes->pos.left + 5;
			y = content_panes->pos.height + content_panes->pos.top + 5;
			}
			{ int x = 0; int y = 0;
				{
					back = war_window->add_element<uiGButton>(war_window->pos.width + -40, y + 0, 20, 20, global::back_tex, get_simple_string(TX_BACK), global::tooltip_text, back_action);
					x = back->pos.width + back->pos.left + 5;
					y = back->pos.height + back->pos.top + 5;
				}
			}
			war_window->setVisible(false);
		}
	x = war_window->pos.width + war_window->pos.left + 5;
	y = war_window->pos.height + war_window->pos.top + 5;
	}
	close_button = war_window->add_element<uiGButton>(600 - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { war::war_window->setVisible(false); });
}

void war::update(IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l) {
	war_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(get_object(obj, l).associated_title)};
			x = (get_linear_ui(TX_WAR_OVERVIEW, params, 1,war_window, x + 0, y + 0, global::empty, global::header_text) + 5);
			y += (global::header_text.csize + 5);
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		war_against->pos.left = x + 0;
		war_against->pos.top = y + 0;
		x = war_against->pos.width + war_against->pos.left + 5;
		y = war_against->pos.height + war_against->pos.top + 5;
		war_against->subelements.clear();
		war_window->subelements.push_back(war_against);
		war_against->reset_options();
		war_against_options(war_against, obj, p1, l);
		maxy = std::max(y, maxy);
		y = tempy;
		info_selection->pos.left = x + 0;
		info_selection->pos.top = y + 0;
		x = info_selection->pos.width + info_selection->pos.left + 5;
		y = info_selection->pos.height + info_selection->pos.top + 5;
		info_selection->subelements.clear();
		war_window->subelements.push_back(info_selection);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		content_panes->pos.left = x + 0;
		content_panes->pos.top = y + 0;
		content_panes->pos.width = war_window->pos.width - (x + 0) + -1;
		content_panes->pos.height = war_window->pos.height - (y + 0) + -5;
		content_panes->update_size();
		x = content_panes->pos.width + content_panes->pos.left + 5;
		y = content_panes->pos.height + content_panes->pos.top + 5;
		pariticipant_pane->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			if(valid_ids(p1)) {
				x = basex;
				tempy = std::max(y, maxy);
				y = tempy;
				{ size_t params[2] = {to_param(get_object(get_object(p1, l).attacker.primary, l).associated_title), to_param(get_object(get_object(p1, l).defender.primary, l).associated_title)};
					x = (get_linear_ui(TX_WAR_NAME, params, 2,pariticipant_pane, x + 0, y + 0, global::empty, global::header_text) + 5);
					y += (global::header_text.csize + 5);
				}
				x = basex;
				tempy = std::max(y, maxy);
				y = tempy;
				{ size_t params[2] = {to_param(static_cast<int>(slow_war_base_value(p1, l))), to_param(static_cast<int>(slow_war_prediction(p1, l)))};
					x = (get_linear_ui(TX_WAR_STATUS, params, 2,pariticipant_pane, x + 0, y + 0, global::empty, global::standard_text) + 5);
					y += (global::standard_text.csize + 5);
				}
				if(get_diplo_decider(get_object(is_agressor(obj, p1, l) ? get_object(p1, l).attacker.primary : get_object(p1, l).defender.primary, l), l) == global::playerid) {
					x = basex;
					tempy = std::max(y, maxy);
					y = tempy;
					{
						offer_peace = pariticipant_pane->add_element<ui_button_disable>(x + 0, y + 0, 100, 22, get_simple_string(TX_OFFER_PEACE), paint_states<2>(global::solid_border, global::disabled_fill), global::standard_text, global::tooltip_text, offer_peace_action(obj, p1, l));
						x = offer_peace->pos.width + offer_peace->pos.left + 5;
						y = offer_peace->pos.height + offer_peace->pos.top + 5;
						disable_offer_peace(offer_peace, obj, p1, l);
					}
					maxy = std::max(y, maxy);
					y = tempy;
					{
						enforce_peace = pariticipant_pane->add_element<ui_button_disable>(x + 0, y + 0, 100, 22, get_simple_string(TX_A_ENFORCE), paint_states<2>(global::solid_border, global::disabled_fill), global::standard_text, global::tooltip_text, enforce_peace_action(obj, p1, l));
						x = enforce_peace->pos.width + enforce_peace->pos.left + 5;
						y = enforce_peace->pos.height + enforce_peace->pos.top + 5;
						disable_enforce_peace(enforce_peace, obj, p1, l);
					}
				}
				x = basex;
				tempy = std::max(y, maxy);
				y = tempy;
				{ e_16 = pariticipant_pane->add_element<uiScrollView>(x + 5, y + 0, 280, pariticipant_pane->pos.height - (y + 0) + -1);
					{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
						x = basex;
						tempy = std::max(y, maxy);
						y = tempy;
						{ int btempx = x;
							std::vector<std::remove_cv<decltype(param_four_type(&war_participant::generate))>::type>  vec;
							pack_war_participants(p1, true, vec, l);
							for(auto it = vec.begin(); it != vec.end(); ++it) {
								x = btempx;
								war_participant::generate(e_16, x, y, *it, l);
								y += 5;
							}
						}
						e_16->calcTotalHeight();
					}
				x = e_16->pos.width + e_16->pos.left + 5;
				y = e_16->pos.height + e_16->pos.top + 5;
				}
				maxy = std::max(y, maxy);
				y = tempy;
				{ e_18 = pariticipant_pane->add_element<uiScrollView>(x + 5, y + 0, pariticipant_pane->pos.width - (x + 5) + -5, pariticipant_pane->pos.height - (y + 0) + -1);
					{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
						x = basex;
						tempy = std::max(y, maxy);
						y = tempy;
						{ int btempx = x;
							std::vector<std::remove_cv<decltype(param_four_type(&war_participant::generate))>::type>  vec;
							pack_war_participants(p1, false, vec, l);
							for(auto it = vec.begin(); it != vec.end(); ++it) {
								x = btempx;
								war_participant::generate(e_18, x, y, *it, l);
								y += 5;
							}
						}
						e_18->calcTotalHeight();
					}
				x = e_18->pos.width + e_18->pos.left + 5;
				y = e_18->pos.height + e_18->pos.top + 5;
				}
			}
		}
		pariticipant_pane->calcTotalHeight();
		fronts_pane->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			if(valid_ids(p1)) {
			}
		}
		fronts_pane->calcTotalHeight();
		war_window->subelements.push_back(content_panes);
		{ int x = 0; int y = 0;
			back->pos.left = war_window->pos.width + -40;
			back->pos.top = y + 0;
			x = back->pos.width + back->pos.left + 5;
			y = back->pos.height + back->pos.top + 5;
		}
		back->subelements.clear();
		war_window->subelements.push_back(back);
	}
	war_window->subelements.push_back(close_button);
}

std::shared_ptr<uiDragRect> sm_select::sm_select_window;
		std::shared_ptr<uiSimpleText> sm_select::e_22;
		std::shared_ptr<uiSimpleText> sm_select::e_24;
	std::shared_ptr<uiSimpleText> sm_select::e_25;
	std::shared_ptr<uiDropDown> sm_select::mission_type;
	std::shared_ptr<ui_button_disable> sm_select::start_mission;
	std::shared_ptr<uiButton> sm_select::cancel;
	std::shared_ptr<uiGButton> sm_select::close_button;

void sm_select::init() {
	int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ sm_select_window = global::uicontainer->add_element<uiDragRect>(x + 0, y + 0, 480, 300, global::solid_border);
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			e_25 = sm_select_window->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_MISSION_LABEL), global::empty, global::standard_text);
			x = e_25->pos.width + e_25->pos.left + 5;
			y += (global::standard_text.csize + 5);
			maxy = std::max(y, maxy);
			y = tempy;
			{
				mission_type = sm_select_window->add_element<uiDropDown>(x + 0, y + 0, 400, 22, global::solid_border, global::standard_text);
				x = mission_type->pos.width + mission_type->pos.left + 5;
				y = mission_type->pos.height + mission_type->pos.top + 5;
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{
				start_mission = sm_select_window->add_element<ui_button_disable>(x + 10, y + 0, 140, 22, get_simple_string(TX_START_MISSION), paint_states<2>(global::solid_border, global::disabled_fill), global::standard_text, global::tooltip_text, start_mission_action);
				x = start_mission->pos.width + start_mission->pos.left + 5;
				y = start_mission->pos.height + start_mission->pos.top + 5;
			}
			maxy = std::max(y, maxy);
			y = tempy;
			{
				cancel = sm_select_window->add_element<uiButton>(x + 0, y + 0, 80, 22, get_simple_string(TX_CANCEL), global::solid_border, global::standard_text, cancel_action);
				x = cancel->pos.width + cancel->pos.left + 5;
				y = cancel->pos.height + cancel->pos.top + 5;
			}
			sm_select_window->setVisible(false);
		}
	x = sm_select_window->pos.width + sm_select_window->pos.left + 5;
	y = sm_select_window->pos.height + sm_select_window->pos.top + 5;
	}
	close_button = sm_select_window->add_element<uiGButton>(480 - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { sm_select::sm_select_window->setVisible(false); });
}

void sm_select::update(IN(smission_selection) obj, IN(g_lock) l) {
	sm_select_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(obj.spy)};
			x = (get_linear_ui(TX_SPY_LABEL, params, 1,sm_select_window, x + 0, y + 0, global::empty, global::standard_text) + 5);
			y += (global::standard_text.csize + 5);
		}
		if(!valid_ids(obj.spy)) {
			maxy = std::max(y, maxy);
			y = tempy;
			e_22 = sm_select_window->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_ENOVY_AUTOMATIC), global::empty, global::standard_text);
			x = e_22->pos.width + e_22->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{
			const auto e = character_selection_menu(sm_select_window, x + 0, y + 0, 170, 22, get_simple_string(TX_SELECT_C), global::solid_border, global::standard_text, spy_c_select_list, spy_c_select_action);
			x = e->pos.width + e->pos.left + 5;
			y = e->pos.height + e->pos.top + 5;
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(obj.target)};
			x = (get_linear_ui(TX_TARGET_LABEL, params, 1,sm_select_window, x + 0, y + 0, global::empty, global::standard_text) + 5);
			y += (global::standard_text.csize + 5);
		}
		if(!valid_ids(obj.target)) {
			maxy = std::max(y, maxy);
			y = tempy;
			e_24 = sm_select_window->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_NONE), global::empty, global::standard_text);
			x = e_24->pos.width + e_24->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{
			const auto e = character_selection_menu(sm_select_window, x + 0, y + 0, 170, 22, get_simple_string(TX_SELECT_C), global::solid_border, global::standard_text, target_c_select_list, target_c_select_action);
			x = e->pos.width + e->pos.left + 5;
			y = e->pos.height + e->pos.top + 5;
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		e_25->pos.left = x + 0;
		e_25->pos.top = y + 0;
		x = e_25->pos.width + e_25->pos.left + 5;
		y += (global::standard_text.csize + 5);
		e_25->subelements.clear();
		sm_select_window->subelements.push_back(e_25);
		maxy = std::max(y, maxy);
		y = tempy;
		mission_type->pos.left = x + 0;
		mission_type->pos.top = y + 0;
		x = mission_type->pos.width + mission_type->pos.left + 5;
		y = mission_type->pos.height + mission_type->pos.top + 5;
		mission_type->subelements.clear();
		sm_select_window->subelements.push_back(mission_type);
		mission_type->reset_options();
		mission_type_options(mission_type, obj, l);
		if(obj.type != 0) {
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ size_t params[1] = {to_param(plot_by_type(obj.type).monthly_cost)};
				x = (get_linear_ui(TX_M_COST, params, 1,sm_select_window, x + 0, y + 0, global::empty, global::standard_text) + 5);
				y += (global::standard_text.csize + 5);
			}
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		start_mission->pos.left = x + 10;
		start_mission->pos.top = y + 0;
		x = start_mission->pos.width + start_mission->pos.left + 5;
		y = start_mission->pos.height + start_mission->pos.top + 5;
		start_mission->subelements.clear();
		sm_select_window->subelements.push_back(start_mission);
		start_mission->enable();
		disable_start_mission(start_mission, obj, l);
		maxy = std::max(y, maxy);
		y = tempy;
		cancel->pos.left = x + 0;
		cancel->pos.top = y + 0;
		x = cancel->pos.width + cancel->pos.left + 5;
		y = cancel->pos.height + cancel->pos.top + 5;
		cancel->subelements.clear();
		sm_select_window->subelements.push_back(cancel);
	}
	sm_select_window->subelements.push_back(close_button);
}

void spy_mission_ui::generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(spy_mission) obj, IN(g_lock) l) {
	int x = ix; int y = iy; int tempy = iy; int maxy = iy; int basex = ix;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{
		const auto tx = parent->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_MISSION_TYPE), global::empty, global::standard_text);
		x = tx->pos.width + tx->pos.left + 5;
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	maxy = std::max(y, maxy);
	y = tempy;
	{ size_t params[1] = {to_param(plot_by_type(obj.type).name)};
		x = (get_linear_ui(TX_EMPTY, params, 1,parent, x + 0, y + 0, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ size_t params[1] = {to_param(obj.spy)};
		x = (get_linear_ui(TX_SPY_LABEL, params, 1,parent, x + 0, y + 0, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ size_t params[1] = {to_param(obj.target)};
		x = (get_linear_ui(TX_TARGET_LABEL, params, 1,parent, x + 0, y + 0, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ size_t params[1] = {to_param(plot_by_type(obj.type).monthly_cost)};
		x = (get_linear_ui(TX_M_COST, params, 1,parent, x + 0, y + 0, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ size_t params[2] = {to_param(obj.stage), to_param(plot_by_type(obj.type).stages.size())};
		x = (get_linear_ui(TX_SM_PROGRESS, params, 2,parent, x + 0, y + 0, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{
		const auto e = parent->add_element<uiButton>(x + 0, y + 0, 80, 22, get_simple_string(TX_CANCEL), global::solid_border, global::standard_text, cancel_action(obj, l));
		x = e->pos.width + e->pos.left + 5;
		y = e->pos.height + e->pos.top + 5;
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
}

std::shared_ptr<uiDragRect> smission_display::smission_display_window;
	std::shared_ptr<ui_button_disable> smission_display::new_mission;
	std::shared_ptr<uiScrollView> smission_display::contents;
	std::shared_ptr<uiGButton> smission_display::close_button;

void smission_display::init() {
	int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ smission_display_window = global::uicontainer->add_element<uiDragRect>(x + 0, y + 0, 400, 650, global::solid_border);
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{
				new_mission = smission_display_window->add_element<ui_button_disable>(x + 0, y + 0, 150, 25, get_simple_string(TX_NEW_MISSION), paint_states<2>(global::solid_border, global::disabled_fill), global::standard_text, global::tooltip_text, new_mission_action);
				x = new_mission->pos.width + new_mission->pos.left + 5;
				y = new_mission->pos.height + new_mission->pos.top + 5;
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ contents = smission_display_window->add_element<uiScrollView>(x + 0, y + 0, smission_display_window->pos.width - (x + 0) + -1, smission_display_window->pos.height - (y + 0) + -5);
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					contents->calcTotalHeight();
				}
			x = contents->pos.width + contents->pos.left + 5;
			y = contents->pos.height + contents->pos.top + 5;
			}
			smission_display_window->setVisible(false);
		}
	x = smission_display_window->pos.width + smission_display_window->pos.left + 5;
	y = smission_display_window->pos.height + smission_display_window->pos.top + 5;
	}
	close_button = smission_display_window->add_element<uiGButton>(400 - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { smission_display::smission_display_window->setVisible(false); });
}

void smission_display::update(IN(admin_id_t) obj, IN(g_lock) l) {
	smission_display_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(head_of_state(obj, l))};
			x = (get_linear_ui(TX_ESP_MIS_FOR, params, 1,smission_display_window, x + 0, y + 0, global::empty, global::header_text) + 5);
			y += (global::header_text.csize + 5);
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		new_mission->pos.left = x + 0;
		new_mission->pos.top = y + 0;
		x = new_mission->pos.width + new_mission->pos.left + 5;
		y = new_mission->pos.height + new_mission->pos.top + 5;
		new_mission->subelements.clear();
		smission_display_window->subelements.push_back(new_mission);
		new_mission->enable();
		disable_new_mission(new_mission, obj, l);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		contents->pos.left = x + 0;
		contents->pos.top = y + 0;
		contents->pos.width = smission_display_window->pos.width - (x + 0) + -1;
		contents->pos.height = smission_display_window->pos.height - (y + 0) + -5;
		x = contents->pos.width + contents->pos.left + 5;
		y = contents->pos.height + contents->pos.top + 5;
		contents->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ int btempx = x;
				for(auto it = transforming_iterator<sm_iter_f, decltype(spy_missions.range(obj,l).first)>(spy_missions.range(obj,l).first); it != spy_missions.range(obj,l).second; ++it) {
					x = btempx;
					spy_mission_ui::generate(contents, x, y, *it, l);
					y += 5;
				}
			}
		}
		smission_display_window->subelements.push_back(contents);
		contents->calcTotalHeight();
	}
	smission_display_window->subelements.push_back(close_button);
}

void cr_display::generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(unsigned int) obj, IN(g_lock) l) {
	int x = ix; int y = iy; int tempy = iy; int maxy = iy; int basex = ix;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{
		const auto e = generateTButton(get_object(global::control_pool.get(obj,l).ad_controller, l).associated_title, parent, x + 0, y + 0, false);
		x = e->pos.width + e->pos.left + 5;
		y = e->pos.height + e->pos.top + 5;
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	maxy = std::max(y, maxy);
	y = tempy;
	{ size_t params[3] = {to_param(get_object(global::control_pool.get(obj,l).ad_controller, l).associated_title), to_param(global::control_pool.get(obj,l).tax * 100.0), to_param(global::control_pool.get(obj,l).tax * get_object(global::control_pool.get(obj,l).province).tax)};
		x = (get_linear_ui(TX_CR_BODY, params, 3,parent, x + 0, y + 10, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5 + 10);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	if(!is_dj(global::control_pool.get(obj,l).province, get_object(global::control_pool.get(obj,l).ad_controller, l).associated_title ,l)) {
		maxy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(dejure_year(get_object(global::control_pool.get(obj,l).ad_controller, l).associated_title, global::control_pool.get(obj,l)))};
			x = (get_linear_ui(TX_DJ_ON, params, 1,parent, x + 15, y + 10, global::empty, global::standard_text) + 5 + 15);
			y += (global::standard_text.csize + 5 + 10);
		}
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
}

void dj_display::generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(unsigned int) obj, IN(g_lock) l) {
	int x = ix; int y = iy; int tempy = iy; int maxy = iy; int basex = ix;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{
		const auto e = generateTButton(global::dj_pool.get(obj,l).owner, parent, x + 0, y + 0, false);
		x = e->pos.width + e->pos.left + 5;
		y = e->pos.height + e->pos.top + 5;
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	maxy = std::max(y, maxy);
	y = tempy;
	{ size_t params[1] = {to_param(global::dj_pool.get(obj,l).owner)};
		x = (get_linear_ui(TX_TITLE_NAME, params, 1,parent, x + 0, y + 10, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5 + 10);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	if(!is_controlled_under_t(global::dj_pool.get(obj,l).province, global::dj_pool.get(obj,l).owner, l)) {
		maxy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(dejure_expires(global::dj_pool.get(obj,l)))};
			x = (get_linear_ui(TX_DJ_UNTIL, params, 1,parent, x + 15, y + 10, global::empty, global::standard_text) + 5 + 15);
			y += (global::standard_text.csize + 5 + 10);
		}
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
}

std::shared_ptr<uiDragRect> povince_display::povince_display_window;
	std::shared_ptr<uiGButton> povince_display::e_44;
	std::shared_ptr<uiGButton> povince_display::e_45;
	std::shared_ptr<uiTabs> povince_display::tabs;
		std::shared_ptr<uiScrollView> povince_display::details;
		std::shared_ptr<uiScrollView> povince_display::control;
			std::shared_ptr<uiSimpleText> povince_display::e_48;
			std::shared_ptr<uiSimpleText> povince_display::e_50;
		std::shared_ptr<uiScrollView> povince_display::economy;
		std::shared_ptr<uiScrollView> povince_display::holdings;
	std::shared_ptr<uiGButton> povince_display::back;
	std::shared_ptr<uiGButton> povince_display::close_button;

void povince_display::init() {
	int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ povince_display_window = global::uicontainer->add_element<uiDragRect>(x + 0, y + 0, 600, 400, global::solid_border);
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ tabs = povince_display_window->add_element<uiTabs>(x + 0, y + 0, povince_display_window->pos.width - (x + 0) + -5, povince_display_window->pos.height - (y + 0) + -5);
				const std::vector<std::wstring> tabstxt = {get_simple_string(TX_T_DETAILS), get_simple_string(TX_T_CONTROL), get_simple_string(TX_T_ECONOMY), get_simple_string(TX_T_HOLDINGS)};
				tabs->init(global::header_text, global::solid_border, tabstxt);
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					details = tabs->panes[0];
					tabs->panes[0]->calcTotalHeight();
				}
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					control = tabs->panes[1];
					x = basex;
					tempy = std::max(y, maxy);
					y = tempy;
					e_48 = tabs->panes[1]->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_CB_LABEL), global::empty, global::standard_text);
					x = e_48->pos.width + e_48->pos.left + 5;
					y += (global::standard_text.csize + 5);
					x = basex;
					tempy = std::max(y, maxy);
					y = tempy;
					e_50 = tabs->panes[1]->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_DJ_LABEL), global::empty, global::standard_text);
					x = e_50->pos.width + e_50->pos.left + 5;
					y += (global::standard_text.csize + 5);
					tabs->panes[1]->calcTotalHeight();
				}
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					economy = tabs->panes[2];
					tabs->panes[2]->calcTotalHeight();
				}
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					holdings = tabs->panes[3];
					tabs->panes[3]->calcTotalHeight();
				}
			x = tabs->pos.width + tabs->pos.left + 5;
			y = tabs->pos.height + tabs->pos.top + 5;
			}
			{ int x = 0; int y = 0;
				{
					back = povince_display_window->add_element<uiGButton>(povince_display_window->pos.width + -40, y + 0, 20, 20, global::back_tex, get_simple_string(TX_BACK), global::tooltip_text, back_action);
					x = back->pos.width + back->pos.left + 5;
					y = back->pos.height + back->pos.top + 5;
				}
			}
			povince_display_window->setVisible(false);
		}
	x = povince_display_window->pos.width + povince_display_window->pos.left + 5;
	y = povince_display_window->pos.height + povince_display_window->pos.top + 5;
	}
	close_button = povince_display_window->add_element<uiGButton>(600 - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { povince_display::povince_display_window->setVisible(false); });
}

void povince_display::update(IN(prov_id_t) obj, IN(g_lock) l) {
	povince_display_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(obj)};
			x = (get_linear_ui(TX_L_PROVINCE, params, 1,povince_display_window, x + 0, y + 0, global::empty, global::header_text) + 5);
			y += (global::header_text.csize + 5);
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{
			e_44 = generateButton(get_object(prov_to_title(obj)).holder, povince_display_window, x + 0, y + 0, true);
			x = e_44->pos.width + e_44->pos.left + 5;
			y = e_44->pos.height + e_44->pos.top + 5;
		}
		maxy = std::max(y, maxy);
		y = tempy;
		{
			e_45 = generateTButton(P_GET_TITLE(obj.value), povince_display_window, x + 0, y + 0, true);
			x = e_45->pos.width + e_45->pos.left + 5;
			y = e_45->pos.height + e_45->pos.top + 5;
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		tabs->pos.left = x + 0;
		tabs->pos.top = y + 0;
		tabs->pos.width = povince_display_window->pos.width - (x + 0) + -5;
		tabs->pos.height = povince_display_window->pos.height - (y + 0) + -5;
		tabs->update_tab_sizes();
		x = tabs->pos.width + tabs->pos.left + 5;
		y = tabs->pos.height + tabs->pos.top + 5;
		details->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ size_t params[1] = {to_param(get_object(obj).culture)};
				x = (get_linear_ui(TX_CULTURE_LABEL, params, 1,details, x + 0, y + 0, global::empty, global::standard_text) + 5);
				y += (global::standard_text.csize + 5);
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ size_t params[1] = {to_param(get_object(obj).religion)};
				x = (get_linear_ui(TX_RELIGION_LABEL, params, 1,details, x + 0, y + 0, global::empty, global::standard_text) + 5);
				y += (global::standard_text.csize + 5);
			}
		}
		details->calcTotalHeight();
		control->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			e_48->pos.left = x + 0;
			e_48->pos.top = y + 0;
			x = e_48->pos.width + e_48->pos.left + 5;
			y += (global::standard_text.csize + 5);
			e_48->subelements.clear();
			control->subelements.push_back(e_48);
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ int btempx = x;
				std::vector<std::remove_cv<decltype(param_four_type(&cr_display::generate))>::type>  vec;
				global::provtocontrol.to_vector(obj,vec,l);
				for(auto it = vec.begin(); it != vec.end(); ++it) {
					x = btempx;
					cr_display::generate(control, x, y, *it, l);
					y += 5;
				}
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			e_50->pos.left = x + 0;
			e_50->pos.top = y + 0;
			x = e_50->pos.width + e_50->pos.left + 5;
			y += (global::standard_text.csize + 5);
			e_50->subelements.clear();
			control->subelements.push_back(e_50);
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ int btempx = x;
				std::vector<std::remove_cv<decltype(param_four_type(&dj_display::generate))>::type>  vec;
				global::provtowner.to_vector(obj,vec,l);
				for(auto it = vec.begin(); it != vec.end(); ++it) {
					x = btempx;
					dj_display::generate(control, x, y, *it, l);
					y += 5;
				}
			}
		}
		control->calcTotalHeight();
		economy->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ size_t params[1] = {to_param(get_object(obj).tax)};
				x = (get_linear_ui(TX_TAX_LABEL, params, 1,economy, x + 0, y + 0, global::empty, global::standard_text) + 5);
				y += (global::standard_text.csize + 5);
			}
		}
		economy->calcTotalHeight();
		holdings->subelements.clear();
		povince_display_window->subelements.push_back(tabs);
		{ int x = 0; int y = 0;
			back->pos.left = povince_display_window->pos.width + -40;
			back->pos.top = y + 0;
			x = back->pos.width + back->pos.left + 5;
			y = back->pos.height + back->pos.top + 5;
		}
		back->subelements.clear();
		povince_display_window->subelements.push_back(back);
	}
	povince_display_window->subelements.push_back(close_button);
}

void disp_influence_by_issue::generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	int x = ix; int y = iy; int tempy = iy; int maxy = iy; int basex = ix;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	x += 15;
			display_issue(parent, x, y, obj, l);
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	{ int x = 0; int y = tempy;
		{
			const auto e = parent->add_element<uiCheckBox>(x + 200, y + 0, get_simple_string(TX_BLANK_SPACE), global::standard_text, global::tooltip_text, false, call_favor_action(obj, p1, p2, l));
			x = e->pos.width + e->pos.left + 5;
			y = e->pos.height + e->pos.top + 5;
			disable_call_favor(e, obj, p1, p2, l);
		}
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	{ int x = 0; int y = tempy;
		{
			const auto e = parent->add_element<uiCheckBox>(x + 325, y + 0, get_simple_string(TX_BLANK_SPACE), global::standard_text, global::tooltip_text, false, offer_favor_action(obj, p1, p2, l));
			x = e->pos.width + e->pos.left + 5;
			y = e->pos.height + e->pos.top + 5;
			disable_offer_favor(e, obj, p1, p2, l);
		}
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	{ int x = 0; int y = tempy;
		{
			const auto e = parent->add_element<uiCheckBox>(x + 450, y + 0, get_simple_string(TX_BLANK_SPACE), global::standard_text, global::tooltip_text, false, friendship_action(obj, p1, p2, l));
			x = e->pos.width + e->pos.left + 5;
			y = e->pos.height + e->pos.top + 5;
			disable_friendship(e, obj, p1, p2, l);
		}
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	{ int x = 0; int y = tempy;
		{
			const auto e = parent->add_element<uiCheckBox>(x + 575, y + 0, get_simple_string(TX_BLANK_SPACE), global::standard_text, global::tooltip_text, false, blackmail_action(obj, p1, p2, l));
			x = e->pos.width + e->pos.left + 5;
			y = e->pos.height + e->pos.top + 5;
			disable_blackmail(e, obj, p1, p2, l);
		}
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
}

void disp_influence_by_person::generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(char_id_t) obj, IN(influence_display_data) p1, IN(g_lock) l) {
	int x = ix; int y = iy; int tempy = iy; int maxy = iy; int basex = ix;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ size_t params[1] = {to_param(obj)};
		x = (get_linear_ui(TX_NAME_HLINK, params, 1,parent, x + 0, y + 0, global::empty, global::standard_text) + 5);
		y += (global::standard_text.csize + 5);
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ int btempx = x;
		for(auto it = std::begin(p1.infl_against); it != std::end(p1.infl_against); ++it) {
			x = btempx;
			disp_influence_by_issue::generate(parent, x, y, *it, obj, p1, l);
			y += 5;
		}
	}
	ix = std::max(x, ix);
	iy = std::max(y, iy);
}

std::shared_ptr<uiDragRect> multi_influence::multi_influence_window;
	std::shared_ptr<uiSimpleText> multi_influence::e_56;
		std::shared_ptr<uiDropDown> multi_influence::stance;
	std::shared_ptr<uiSimpleText> multi_influence::e_59;
	std::shared_ptr<uiSimpleText> multi_influence::e_60;
	std::shared_ptr<uiSimpleText> multi_influence::e_61;
	std::shared_ptr<uiSimpleText> multi_influence::e_62;
	std::shared_ptr<uiSimpleText> multi_influence::e_63;
	std::shared_ptr<uiScrollView> multi_influence::contents;
	std::shared_ptr<uiButton> multi_influence::done_button;

void multi_influence::init() {
	int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ multi_influence_window = global::uicontainer->add_element<uiDragRect>(x + 0, y + 0, global::uicontainer->pos.width - (x + 0) + 0, global::uicontainer->pos.height - (y + 0) + 0, global::solid_border);
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			e_56 = multi_influence_window->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_PROPOSAL_REACT), global::empty, global::header_text);
			x = e_56->pos.width + e_56->pos.left + 5;
			y += (global::header_text.csize + 5);
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			e_59 = multi_influence_window->add_element<uiSimpleText>(x + 0, y + 0, get_simple_string(TX_BLANK_SPACE), global::empty, global::standard_text);
			x = e_59->pos.width + e_59->pos.left + 5;
			y += (global::standard_text.csize + 5);
			{ int x = 0; int y = tempy;
				e_60 = multi_influence_window->add_element<uiSimpleText>(x + 200, y + 0, get_simple_string(TX_CALL_FAVOR), global::empty, global::standard_text);
				x = e_60->pos.width + e_60->pos.left + 5;
				y += (global::standard_text.csize + 5);
			}
			{ int x = 0; int y = tempy;
				e_61 = multi_influence_window->add_element<uiSimpleText>(x + 325, y + 0, get_simple_string(TX_OFFER_FAVOR), global::empty, global::standard_text);
				x = e_61->pos.width + e_61->pos.left + 5;
				y += (global::standard_text.csize + 5);
			}
			{ int x = 0; int y = tempy;
				e_62 = multi_influence_window->add_element<uiSimpleText>(x + 450, y + 0, get_simple_string(TX_APPL_FRIENDSHIP), global::empty, global::standard_text);
				x = e_62->pos.width + e_62->pos.left + 5;
				y += (global::standard_text.csize + 5);
			}
			{ int x = 0; int y = tempy;
				e_63 = multi_influence_window->add_element<uiSimpleText>(x + 575, y + 0, get_simple_string(TX_BLACKMAIL), global::empty, global::standard_text);
				x = e_63->pos.width + e_63->pos.left + 5;
				y += (global::standard_text.csize + 5);
			}
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ contents = multi_influence_window->add_element<uiScrollView>(x + 0, y + 0, multi_influence_window->pos.width - (x + 0) + -1, multi_influence_window->pos.height - (y + 0) + -35);
				{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
					contents->calcTotalHeight();
				}
			x = contents->pos.width + contents->pos.left + 5;
			y = contents->pos.height + contents->pos.top + 5;
			}
			multi_influence_window->setVisible(false);
		}
	x = multi_influence_window->pos.width + multi_influence_window->pos.left + 5;
	y = multi_influence_window->pos.height + multi_influence_window->pos.top + 5;
	}
}

void multi_influence::open(IN(political_action) obj, IN(influence_display_data) p1) {
	event signal;
	{ r_lock l;
	multi_influence_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		e_56->pos.left = x + 0;
		e_56->pos.top = y + 0;
		x = e_56->pos.width + e_56->pos.left + 5;
		y += (global::header_text.csize + 5);
		e_56->subelements.clear();
		multi_influence_window->subelements.push_back(e_56);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
				display_political_action(multi_influence_window, x, y, obj, l);
		if(!p1.only_positive) {
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{
				stance = multi_influence_window->add_element<uiDropDown>(x + 0, y + 0, 170, 22, global::solid_border, global::standard_text);
				stance_options(stance);
				x = stance->pos.width + stance->pos.left + 5;
				y = stance->pos.height + stance->pos.top + 5;
			}
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		e_59->pos.left = x + 0;
		e_59->pos.top = y + 0;
		x = e_59->pos.width + e_59->pos.left + 5;
		y += (global::standard_text.csize + 5);
		e_59->subelements.clear();
		multi_influence_window->subelements.push_back(e_59);
		{ int x = 0; int y = tempy;
			e_60->pos.left = x + 200;
			e_60->pos.top = y + 0;
			x = e_60->pos.width + e_60->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		e_60->subelements.clear();
		multi_influence_window->subelements.push_back(e_60);
		{ int x = 0; int y = tempy;
			e_61->pos.left = x + 325;
			e_61->pos.top = y + 0;
			x = e_61->pos.width + e_61->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		e_61->subelements.clear();
		multi_influence_window->subelements.push_back(e_61);
		{ int x = 0; int y = tempy;
			e_62->pos.left = x + 450;
			e_62->pos.top = y + 0;
			x = e_62->pos.width + e_62->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		e_62->subelements.clear();
		multi_influence_window->subelements.push_back(e_62);
		{ int x = 0; int y = tempy;
			e_63->pos.left = x + 575;
			e_63->pos.top = y + 0;
			x = e_63->pos.width + e_63->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		e_63->subelements.clear();
		multi_influence_window->subelements.push_back(e_63);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		contents->pos.left = x + 0;
		contents->pos.top = y + 0;
		contents->pos.width = multi_influence_window->pos.width - (x + 0) + -1;
		contents->pos.height = multi_influence_window->pos.height - (y + 0) + -35;
		x = contents->pos.width + contents->pos.left + 5;
		y = contents->pos.height + contents->pos.top + 5;
		contents->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ int btempx = x;
				for(auto it = std::begin(p1.to_influence); it != std::end(p1.to_influence); ++it) {
					x = btempx;
					disp_influence_by_person::generate(contents, x, y, *it, p1, l);
					y += 5;
				}
			}
		}
		multi_influence_window->subelements.push_back(contents);
		contents->calcTotalHeight();
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{
			done_button = multi_influence_window->add_element<uiButton>(multi_influence_window->pos.width + -210, y + 0, 200, 25, get_simple_string(TX_DONE), global::solid_border, global::standard_text, done_button_action(obj, p1, signal, l));
			x = done_button->pos.width + done_button->pos.left + 5;
			y = done_button->pos.height + done_button->pos.top + 5;
		}
	}
	}
	
	open_window_centered(multi_influence_window);
	event* earray[] = {&signal, &global::quitevent};
	event::wait_for_multiple(earray, 2, false);
	multi_influence_window->setVisible(false);
}
void multi_influence::update(IN(political_action) obj, IN(influence_display_data) p1, IN(g_lock) l) {
	multi_influence_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		e_56->pos.left = x + 0;
		e_56->pos.top = y + 0;
		x = e_56->pos.width + e_56->pos.left + 5;
		y += (global::header_text.csize + 5);
		e_56->subelements.clear();
		multi_influence_window->subelements.push_back(e_56);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
				display_political_action(multi_influence_window, x, y, obj, l);
		if(!p1.only_positive) {
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{
				stance = multi_influence_window->add_element<uiDropDown>(x + 0, y + 0, 170, 22, global::solid_border, global::standard_text);
				stance_options(stance);
				x = stance->pos.width + stance->pos.left + 5;
				y = stance->pos.height + stance->pos.top + 5;
			}
		}
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		e_59->pos.left = x + 0;
		e_59->pos.top = y + 0;
		x = e_59->pos.width + e_59->pos.left + 5;
		y += (global::standard_text.csize + 5);
		e_59->subelements.clear();
		multi_influence_window->subelements.push_back(e_59);
		{ int x = 0; int y = tempy;
			e_60->pos.left = x + 200;
			e_60->pos.top = y + 0;
			x = e_60->pos.width + e_60->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		e_60->subelements.clear();
		multi_influence_window->subelements.push_back(e_60);
		{ int x = 0; int y = tempy;
			e_61->pos.left = x + 325;
			e_61->pos.top = y + 0;
			x = e_61->pos.width + e_61->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		e_61->subelements.clear();
		multi_influence_window->subelements.push_back(e_61);
		{ int x = 0; int y = tempy;
			e_62->pos.left = x + 450;
			e_62->pos.top = y + 0;
			x = e_62->pos.width + e_62->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		e_62->subelements.clear();
		multi_influence_window->subelements.push_back(e_62);
		{ int x = 0; int y = tempy;
			e_63->pos.left = x + 575;
			e_63->pos.top = y + 0;
			x = e_63->pos.width + e_63->pos.left + 5;
			y += (global::standard_text.csize + 5);
		}
		e_63->subelements.clear();
		multi_influence_window->subelements.push_back(e_63);
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		contents->pos.left = x + 0;
		contents->pos.top = y + 0;
		contents->pos.width = multi_influence_window->pos.width - (x + 0) + -1;
		contents->pos.height = multi_influence_window->pos.height - (y + 0) + -35;
		x = contents->pos.width + contents->pos.left + 5;
		y = contents->pos.height + contents->pos.top + 5;
		contents->subelements.clear();
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			x = basex;
			tempy = std::max(y, maxy);
			y = tempy;
			{ int btempx = x;
				for(auto it = std::begin(p1.to_influence); it != std::end(p1.to_influence); ++it) {
					x = btempx;
					disp_influence_by_person::generate(contents, x, y, *it, p1, l);
					y += 5;
				}
			}
		}
		multi_influence_window->subelements.push_back(contents);
		contents->calcTotalHeight();
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		done_button->pos.left = multi_influence_window->pos.width + -210;
		done_button->pos.top = y + 0;
		x = done_button->pos.width + done_button->pos.left + 5;
		y = done_button->pos.height + done_button->pos.top + 5;
		done_button->subelements.clear();
		multi_influence_window->subelements.push_back(done_button);
	}
}

#pragma  warning(pop)
