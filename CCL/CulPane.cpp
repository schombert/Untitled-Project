#include "globalhelpers.h"
#include "ChPane.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "CulPane.h"
#include "i18n.h"

struct {
	std::shared_ptr<uiVRect> culmenu;
	std::shared_ptr<uiSimpleText> name;
	std::shared_ptr<uiSimpleText> group;
	std::shared_ptr<uiButton> mapdisplay;
} culmenu;

void inline_update_cul_pane(cul_id_t focused) noexcept {
	if (!culmenu.culmenu->gVisible())
		return;

	culmenu.name->updateText(str_to_wstr(get_object(focused).name.get()));
	culmenu.group->updateText(str_to_wstr(get_object(focused).group.get()));
	culmenu.mapdisplay->clickaction = [focused](uiButton *b) {global::map.displayedcul = focused; global::mapmode = MAP_MODE_CULTURE; global::setFlag(FLG_MAP_UPDATE); };
}

void SetupCulPane(cul_id_t focused) noexcept {
	global::uiqueue.push([ focused] {
		global::HideAll();
		culmenu.culmenu->setVisible(true);
		global::infowindows->setVisible(true);
		global::enterPane(5, focused.value);

		global::setFlag(FLG_PANEL_UPDATE);
	});
} 

void InitCulPane() noexcept {
	sf::Color mback(200, 200, 200);
	sf::Color trans(0, 0, 0, 0);

	global::infowindows->subelements.push_back(culmenu.culmenu = std::make_shared<uiVRect>(0, 0, M_WIDTH, global::infowindows->pos.height, global::infowindows));
	culmenu.culmenu->setVisible(false);

	culmenu.name = culmenu.culmenu->add_element<uiSimpleText>(10, 10, TEXT("NAME"), global::empty, global::header_text);
	culmenu.group = culmenu.culmenu->add_element<uiSimpleText>(10, 35, TEXT("GROUP"), global::empty, global::standard_text);
	culmenu.mapdisplay = culmenu.culmenu->add_element<uiButton>(10, 50, 150, 25, get_simple_string(TX_SHOW_ON_MAP), global::solid_border, global::standard_text, [](uiButton* b) {});
}

void hide_cm() noexcept {
	culmenu.culmenu->setVisible(false);
}
