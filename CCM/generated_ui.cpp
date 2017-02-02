#include "globalhelpers.h"
#include "generated_ui.h"
#include "i18n.h"
#include "structs.h"

#pragma  warning(push)
#pragma  warning(disable:4456)

template <typename A, typename B, typename C, typename T, typename ... REST>
T param_four_type(void(*f_ptr)(A, B, C, T, REST ... rest)) { return T(); }

template <typename A, typename B, typename C, typename T, typename ... REST>
T param_four_type(void(*f_ptr)(A, B, C, T&, REST ... rest)) { return T(); }

void init_all_generated_ui() {
	em_selection::init();
}

std::shared_ptr<uiDragRect> em_selection::em_selection_window;
	std::shared_ptr<uiGButton> em_selection::close_button;

void em_selection::init() {
	int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
	x = basex;
	tempy = std::max(y, maxy);
	y = tempy;
	{ em_selection_window = global::uicontainer->add_element<uiDragRect>(x + 0, y + 0, 400, 400, global::solid_border);
		{ int y = 5; int x = 5; int tempy = y; int maxy = y; int basex = x;
			em_selection_window->setVisible(false);
		}
	x = em_selection_window->pos.width + em_selection_window->pos.left + 5;
	y = em_selection_window->pos.height + em_selection_window->pos.top + 5;
	}
	close_button = em_selection_window->add_element<uiGButton>(400 - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { em_selection::em_selection_window->setVisible(false); });
}

void em_selection::update(IN(g_lock) l) {
	em_selection_window->subelements.clear();
}

#pragma  warning(pop)
