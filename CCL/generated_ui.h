#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"

struct newfront;
struct wargoal;
struct emission_selection;
struct smission_selection;
class spy_mission;
struct influence_against;
class influence_display_data;
class influence_display_data;
class political_action;

void init_all_generated_ui();

namespace em_selection {
	void envoy_c_select_list(INOUT(cvector<char_id_t>) vec);
	void envoy_c_select_action(char_id_t id);
	void mission_type_options(IN(std::shared_ptr<uiDropDown>) dd, IN(emission_selection) obj, IN(g_lock) l);
	void def_c_select_list(INOUT(cvector<char_id_t>) vec);
	void def_c_select_action(char_id_t id);
	void na_c_select_list(INOUT(cvector<char_id_t>) vec);
	void na_c_select_action(char_id_t id);
	void start_mission_action(ui_button_disable* obj);
	void disable_start_mission(IN(std::shared_ptr<ui_button_disable>) element, IN(emission_selection) obj, IN(g_lock) l);
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
	void update(IN(emission_selection) obj, IN(g_lock) l);
}

namespace emission_display {
	void new_mission_action(ui_button_disable* obj);
	void disable_new_mission(IN(std::shared_ptr<ui_button_disable>) element, IN(admin_id_t) obj, IN(g_lock) l);

	extern std::shared_ptr<uiDragRect> emission_display_window;
		extern std::shared_ptr<ui_button_disable> new_mission;
		extern std::shared_ptr<uiScrollView> contents;
		extern std::shared_ptr<uiGButton> close_button;

	void init();
	void update(IN(admin_id_t) obj, IN(g_lock) l);
}

namespace war_participant {
	void generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(std::pair<admin_id_t, std::vector<wargoal>>) obj, IN(g_lock) l);
}

namespace name {
	void generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(char_id) obj, IN(g_lock) l);
}

namespace war {
	void war_against_options(IN(std::shared_ptr<uiDropDown>) dd, IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l);
	void info_selection_options(IN(std::shared_ptr<uiDropDown>) dd);
	std::function<void(ui_button_disable*)> offer_peace_action(IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l);
	void disable_offer_peace(IN(std::shared_ptr<ui_button_disable>) element, IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l);
	std::function<void(ui_button_disable*)> enforce_peace_action(IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l);
	void disable_enforce_peace(IN(std::shared_ptr<ui_button_disable>) element, IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l);
	void back_action(uiGButton* obj);

	extern std::shared_ptr<uiDragRect> war_window;
		extern std::shared_ptr<uiDropDown> war_against;
		extern std::shared_ptr<uiDropDown> info_selection;
		extern std::shared_ptr<uiPanes> content_panes;
			extern std::shared_ptr<uiScrollView> pariticipant_pane;
						extern std::shared_ptr<ui_button_disable> offer_peace;
						extern std::shared_ptr<ui_button_disable> enforce_peace;
					extern std::shared_ptr<uiScrollView> e_16;
					extern std::shared_ptr<uiScrollView> e_18;
			extern std::shared_ptr<uiScrollView> fronts_pane;
		extern std::shared_ptr<uiGButton> back;
		extern std::shared_ptr<uiGButton> close_button;

	void init();
	void update(IN(admin_id_t) obj, IN(war_id_t) p1, IN(g_lock) l);
}

namespace sm_select {
	void spy_c_select_list(INOUT(cvector<char_id_t>) vec);
	void spy_c_select_action(char_id_t id);
	void target_c_select_list(INOUT(cvector<char_id_t>) vec);
	void target_c_select_action(char_id_t id);
	void mission_type_options(IN(std::shared_ptr<uiDropDown>) dd, IN(smission_selection) obj, IN(g_lock) l);
	void start_mission_action(ui_button_disable* obj);
	void disable_start_mission(IN(std::shared_ptr<ui_button_disable>) element, IN(smission_selection) obj, IN(g_lock) l);
	void cancel_action(uiButton* obj);

	extern std::shared_ptr<uiDragRect> sm_select_window;
			extern std::shared_ptr<uiSimpleText> e_22;
			extern std::shared_ptr<uiSimpleText> e_24;
		extern std::shared_ptr<uiSimpleText> e_25;
		extern std::shared_ptr<uiDropDown> mission_type;
		extern std::shared_ptr<ui_button_disable> start_mission;
		extern std::shared_ptr<uiButton> cancel;
		extern std::shared_ptr<uiGButton> close_button;

	void init();
	void update(IN(smission_selection) obj, IN(g_lock) l);
}

namespace spy_mission_ui {
	std::function<void(uiButton*)> cancel_action(IN(spy_mission) obj, IN(g_lock) l);

	void generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(spy_mission) obj, IN(g_lock) l);
}

namespace smission_display {
	void new_mission_action(ui_button_disable* obj);
	void disable_new_mission(IN(std::shared_ptr<ui_button_disable>) element, IN(admin_id_t) obj, IN(g_lock) l);

	extern std::shared_ptr<uiDragRect> smission_display_window;
		extern std::shared_ptr<ui_button_disable> new_mission;
		extern std::shared_ptr<uiScrollView> contents;
		extern std::shared_ptr<uiGButton> close_button;

	void init();
	void update(IN(admin_id_t) obj, IN(g_lock) l);
}

namespace cr_display {
	void generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(unsigned int) obj, IN(g_lock) l);
}

namespace dj_display {
	void generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(unsigned int) obj, IN(g_lock) l);
}

namespace povince_display {
	void back_action(uiGButton* obj);

	extern std::shared_ptr<uiDragRect> povince_display_window;
		extern std::shared_ptr<uiGButton> e_44;
		extern std::shared_ptr<uiGButton> e_45;
		extern std::shared_ptr<uiTabs> tabs;
			extern std::shared_ptr<uiScrollView> details;
			extern std::shared_ptr<uiScrollView> control;
				extern std::shared_ptr<uiSimpleText> e_48;
				extern std::shared_ptr<uiSimpleText> e_50;
			extern std::shared_ptr<uiScrollView> economy;
			extern std::shared_ptr<uiScrollView> holdings;
		extern std::shared_ptr<uiGButton> back;
		extern std::shared_ptr<uiGButton> close_button;

	void init();
	void update(IN(prov_id_t) obj, IN(g_lock) l);
}

namespace disp_influence_by_issue {
	std::function<void(uiCheckBox*)> call_favor_action(IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);
	void disable_call_favor(IN(std::shared_ptr<uiCheckBox>) element, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);
	std::function<void(uiCheckBox*)> offer_favor_action(IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);
	void disable_offer_favor(IN(std::shared_ptr<uiCheckBox>) element, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);
	std::function<void(uiCheckBox*)> friendship_action(IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);
	void disable_friendship(IN(std::shared_ptr<uiCheckBox>) element, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);
	std::function<void(uiCheckBox*)> blackmail_action(IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);
	void disable_blackmail(IN(std::shared_ptr<uiCheckBox>) element, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);

	void generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l);
}

namespace disp_influence_by_person {
	void generate(IN(std::shared_ptr<uiElement>) parent, int &ix, int &iy, IN(char_id_t) obj, IN(influence_display_data) p1, IN(g_lock) l);
}

namespace multi_influence {
	void stance_options(IN(std::shared_ptr<uiDropDown>) dd);
	std::function<void(uiButton*)> done_button_action(IN(political_action) obj, IN(influence_display_data) p1, INOUT(event) signal, IN(g_lock) l);

	extern std::shared_ptr<uiDragRect> multi_influence_window;
		extern std::shared_ptr<uiSimpleText> e_56;
			extern std::shared_ptr<uiDropDown> stance;
		extern std::shared_ptr<uiSimpleText> e_59;
		extern std::shared_ptr<uiSimpleText> e_60;
		extern std::shared_ptr<uiSimpleText> e_61;
		extern std::shared_ptr<uiSimpleText> e_62;
		extern std::shared_ptr<uiSimpleText> e_63;
		extern std::shared_ptr<uiScrollView> contents;
		extern std::shared_ptr<uiButton> done_button;

	void init();
	void open(IN(political_action) obj, IN(influence_display_data) p1);
	void update(IN(political_action) obj, IN(influence_display_data) p1, IN(g_lock) l);
}

