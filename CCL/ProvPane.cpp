#include "globalhelpers.h"
#include "ProvPane.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "structs.hpp"
#include "CulPane.h"
#include "RelPane.h"
#include "datamanagement.hpp"
#include "i18n.h"
#include "generated_ui.h"

nolock_stack<50, prov_id> provpanebacklist;

update_record prov_pane_rec([] {
	if (povince_display::povince_display_window->gVisible()) {
		prov_id current = provpanebacklist.peek();
		if (current != 0) {
			povince_display::update(prov_id_t(current), r_lock());
		}
	}
});

void povince_display::back_action(uiGButton* obj) {
	provpanebacklist.pop();
	prov_id toopen = provpanebacklist.pop();
	if (toopen != 0) {
		SetupProvPane(prov_id_t(toopen));
	}
}

void SetupProvPane(prov_id_t focused) noexcept {
	provpanebacklist.push(focused.value);
	if (povince_display::povince_display_window->gVisible()) {
		global::uiqueue.push([] {
			war::war_window->toFront(global::uicontainer);
		});
	} else {
		open_window_centered(povince_display::povince_display_window);
	}
	prov_pane_rec.needs_update = true;
}
