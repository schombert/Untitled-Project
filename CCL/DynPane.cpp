#include "globalhelpers.h"
#include "DynPane.h"
#include "uielements.hpp"
#include "structs.hpp"
#include "datamanagement.hpp"
#include "i18n.h"

struct {
	std::shared_ptr<uiVRect> dynmenu;
	std::shared_ptr<uiSimpleText> name;
	std::shared_ptr<uiScrollView> members;
} dmenu;

void inline_update_dyn_pane(dyn_id_t focused) noexcept {
	if (!dmenu.dynmenu->gVisible())
		return;

	std::string dname = get_object(focused).name.get();
	dmenu.name->updateText(str_to_wstr(dname));

	dmenu.members->subelements.clear();

	sf::Color mback(200, 200, 200);
	dmenu.members->add_element<uiSimpleText>(40, 10, get_simple_string(TX_LIVING_MEMBERS), global::empty, global::standard_text);

	int maxacross = (dmenu.members->pos.width - 15) / 45;
	int tcount = 0;
	std::vector<char_id_t> ppla;
	std::vector<char_id_t> ppld;
	global::get_dyn_members(focused, ppla, ppld);
	//global::GetLivingMembers(focused, ppl);
	for (const auto ch : ppla) {
		generateButton(char_id_t(ch), dmenu.members, (tcount % maxacross) * 45 + 5, (int)(tcount / maxacross) * 45 + 30, false);
		tcount++;
	}
	//ppl.clear();

	int down = ((int)((tcount - 1) / maxacross) + 1) * 45 + 30;
	dmenu.members->add_element<uiSimpleText>(40, down, get_simple_string(TX_DECEASED_MEMBERS), global::empty, global::standard_text);
	tcount = 0;
	//global::GetDeadMembers(focused, ppl);
	for (const auto ch : ppld) {
		generateButton(char_id_t(ch), dmenu.members, (tcount % maxacross) * 45 + 5, (int)(tcount / maxacross) * 45 + down + 30, false);
		tcount++;
	}

	dmenu.members->calcTotalHeight();
	dmenu.members->redraw();
}

void SetupDynPane(dyn_id_t focused) noexcept {
	global::uiqueue.push([ focused]{
		global::HideAll();
		dmenu.dynmenu->setVisible(true);
		global::infowindows->setVisible(true);
		global::enterPane(4, focused.value);

		global::setFlag(FLG_PANEL_UPDATE);
	});
}


void InitDynPane() noexcept {
	sf::Color mback(200, 200, 200);
	sf::Color trans(0, 0, 0, 0);

	dmenu.dynmenu = std::make_shared<uiVRect>(0, 0, M_WIDTH, global::infowindows->pos.height, global::infowindows);
	global::infowindows->subelements.push_back(dmenu.dynmenu);
	dmenu.dynmenu->setVisible(false);
	dmenu.name = dmenu.dynmenu->add_element<uiSimpleText>(10, 10, TEXT("NAME"), global::empty, global::header_text);
	dmenu.members = std::make_shared<uiScrollView>(0, 25, dmenu.dynmenu->pos.width, dmenu.dynmenu->pos.height - 25, dmenu.dynmenu);
	dmenu.members->margin = 5;
	dmenu.dynmenu->subelements.push_back(dmenu.members);
}

void hide_dm() noexcept {
	dmenu.dynmenu->setVisible(false);
}
