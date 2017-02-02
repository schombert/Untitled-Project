#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"


void init_all_generated_ui();

namespace em_selection {
	extern std::shared_ptr<uiDragRect> em_selection_window;
		extern std::shared_ptr<uiGButton> close_button;

	void init();
	void update(IN(g_lock) l);
}

