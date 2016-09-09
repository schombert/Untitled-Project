#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"

struct emission_selection;
class titled_data;

void init_all_generated_ui();

namespace em_selection {
	void envoy_c_select_list(INOUT(std::vector<char_id>) vec);
	void envoy_c_select_action(char_id id);
	void mission_type_options(IN(std::shared_ptr<uiDropDown>) dd, INOUT(emission_selection) obj, IN(g_lock) l);
	void def_c_select_list(INOUT(std::vector<char_id>) vec);
	void def_c_select_action(char_id id);
	void na_c_select_list(INOUT(std::vector<char_id>) vec);
	void na_c_select_action(char_id id);
	void start_mission_action(ui_button_disable* obj);
	void disable_start_mission(IN(std::shared_ptr<ui_button_disable>) element, INOUT(emission_selection) obj, IN(g_lock) l);
	void cancel_action(uiButton* obj);

	extern std::shared_ptr<uiDragRect> em_selection_window;
			extern std::shared_ptr<uiSimpleText> e_2;
		extern std::shared_ptr<uiSimpleText> e_3;
		extern std::shared_ptr<uiDropDown> mission_type;
		extern std::shared_ptr<uiPanes> content_panes;
			extern std::shared_ptr<uiScrollView> def_pact;
			extern std::shared_ptr<uiScrollView> def_against;
			extern std::shared_ptr<uiScrollView> non_aggression;
		extern std::shared_ptr<ui_button_disable> start_mission;
		extern std::shared_ptr<uiButton> cancel;
		extern std::shared_ptr<uiGButton> close_button;

	void init();
	void update(INOUT(emission_selection) obj, IN(g_lock) l);
}

namespace emission_display {
	void new_mission_action(ui_button_disable* obj);
	void disable_new_mission(IN(std::shared_ptr<ui_button_disable>) element, INOUT(titled_data) obj, IN(g_lock) l);

	extern std::shared_ptr<uiDragRect> emission_display_window;
		extern std::shared_ptr<ui_button_disable> new_mission;
		extern std::shared_ptr<uiScrollView> contents;
		extern std::shared_ptr<uiGButton> close_button;

	void init();
	void update(INOUT(titled_data) obj, IN(g_lock) l);
}

