#include "globalhelpers.h"
#include "generated_ui.h"
#include "i18n.h"
#include "structs.hpp"
#include "living_data.h"
#include "envoys.h"

template <typename T>
T param_four_type(void (*f_ptr)(const std::shared_ptr<uiElement>&, int&, int&, const T&, const g_lock&)) {return T();}

void init_all_generated_ui() {
	em_selection::init();
	emission_display::init();
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

void em_selection::update(INOUT(emission_selection) obj, IN(g_lock) l) {
	em_selection_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(obj.envoy)};
			x = (get_linear_ui(TX_EV_LABEL, params, 1,em_selection_window, x + 0, y + 0, global::empty, global::standard_text) + 5);
			y += (global::standard_text.csize + 5);
		}
		if(obj.envoy == 0) {
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

void emission_display::update(INOUT(titled_data) obj, IN(g_lock) l) {
	emission_display_window->subelements.clear();
	{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
		x = basex;
		tempy = std::max(y, maxy);
		y = tempy;
		{ size_t params[1] = {to_param(obj.associated)};
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
				for(auto it = std::begin(obj.envoy_missions); it != std::end(obj.envoy_missions); ++it) {
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

