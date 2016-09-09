#include "globalhelpers.h"
#include "ChPane.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "RelPane.h"
#include "i18n.h"

struct {
	std::shared_ptr<uiVRect> relmenu;
	std::shared_ptr<uiSimpleText> name;
	std::shared_ptr<uiSimpleText> group;
	std::shared_ptr<uiButton> mapdisplay;
} relmenu;

void inline_update_rel_pane(rel_id_t focused) noexcept {
	if (!relmenu.relmenu->gVisible())
		return;

	relmenu.name->updateText(str_to_wstr(get_object(focused).name.get()));
	relmenu.group->updateText(str_to_wstr(get_object(focused).group.get()));
	relmenu.mapdisplay->clickaction = [focused](uiButton *b) {global::map.displayedrel = focused; global::mapmode = MAP_MODE_RELIGION; global::setFlag(FLG_MAP_UPDATE); };
}

void SetupRelPane(rel_id_t focused) noexcept {
	global::uiqueue.push([ focused] {
		global::HideAll();
		relmenu.relmenu->setVisible(true);
		global::infowindows->setVisible(true);
		global::enterPane(6, focused.value);

		global::setFlag(FLG_PANEL_UPDATE);
	});
}

void InitRelPane() noexcept {
	sf::Color mback(200, 200, 200);
	sf::Color trans(0, 0, 0, 0);

	global::infowindows->subelements.push_back(relmenu.relmenu = std::make_shared<uiVRect>(0, 0, M_WIDTH, global::infowindows->pos.height, global::infowindows));
	relmenu.relmenu->setVisible(false);

	relmenu.name = relmenu.relmenu->add_element<uiSimpleText>(10, 10, TEXT("NAME"), global::empty, global::header_text);
	relmenu.group = relmenu.relmenu->add_element<uiSimpleText>(10, 35, TEXT("GROUP"), global::empty, global::standard_text);
	relmenu.mapdisplay =  relmenu.relmenu->add_element<uiButton>(10, 50, 150, 25, get_simple_string(TX_MAP_CK), global::solid_border, global::standard_text, [](uiButton* b) {});
}

void hide_rm() noexcept {
	relmenu.relmenu->setVisible(false);
}
