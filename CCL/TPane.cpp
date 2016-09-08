#include "globalhelpers.h"
#include "TPane.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "mapdisplay.h"
#include "datamanagement.hpp"
#include "i18n.h"
#include "court.h"
#include "laws.h"
#include "prov_control.h"

struct {
	std::shared_ptr<uiVRect> titlemenu;
	std::shared_ptr<uiGButton> holder;

	//std::shared_ptr<uiVRect> leigechain;

	std::shared_ptr<uiSimpleText> name;

	std::shared_ptr<uiButton> showcontrolled;
	std::shared_ptr<uiButton> showdj;

	std::shared_ptr<uiTabs> tabs;

	std::shared_ptr<uiPropList> attributes;

	std::shared_ptr<uiHozContainer> courtiers;
} tmenu;

void inline_upate_title_pane(title_id_t focused) noexcept {
	if (!tmenu.titlemenu->gVisible())
		return;

	std::wstring titlename = global::w_title_name(focused);

	IN(auto) t = get_object(focused);

	int type = t.type;
	char_id_t holder = t.holder;
	// title_id leige = global::titles[focused].leige;

	
	UpdateChIcon(tmenu.holder, holder, 64); //consider no holder
	

	/*tmenu.leigechain->subelements.clear();
	std::vector<title_id> ttls;
	global::LeigeChain(focused, [&ttls](title_id t, char_id c, int type) {
		ttls.push_back(t);
	});
	for (size_t i = 0; i < ttls.size(); ++i) {
		generateTButton(ttls[i], tmenu.leigechain, 0 + static_cast<int>(i) * 64 + (4 - static_cast<int>(ttls.size())) * 64, 0, true);
	}*/



	if (type == 4) {
		prov_id_t prov = title_to_prov(focused);
		if (valid_ids(prov) && global::focused != prov) {
			make_vec_visible(get_object(prov).centroid, global::lockWindow());
		}

	}

	tmenu.name->updateText(titlename);

	tmenu.courtiers->subelements.clear();
	tmenu.tabs->panes[1]->subelements.clear();


	
	const auto ad = t.associated_admin;
	if (!valid_ids(ad)) {
		tmenu.showcontrolled->setVisible(false);

		tmenu.attributes->values[0] = L"NA";
		tmenu.attributes->values[1] = L"NA";
		tmenu.attributes->values[2] = L"NA";
		tmenu.attributes->values[3] = L"NA";

	} else {
		r_lock l;

		IN(auto) ao = get_object(ad, l);
		tmenu.attributes->values[0] = std::to_wstring(ao.stats.get_analytic());
		tmenu.attributes->values[1] = std::to_wstring(ao.stats.get_martial());
		tmenu.attributes->values[2] = std::to_wstring(ao.stats.get_social());
		tmenu.attributes->values[3] = std::to_wstring(ao.stats.get_intrigue());

		int i = 0;
		
		global::enum_vassals(ad, l, [&l, &i](admin_id_t id) {
			const auto t = get_object(id, l).associated_title;
			generateTButton(t, tmenu.tabs->panes[1], 5, 5 + 45 * i, false);
			tmenu.tabs->panes[1]->add_element<uiSimpleText>(50, 20 + 45 * i, global::w_title_name(t), global::empty, global::standard_text);
			++i;
		});
		tmenu.tabs->panes[1]->calcTotalHeight();

		

		
		cvector<char_id_t> ppl;
		enum_court_a(ad, ppl, l);
		for (const auto ch : ppl)
			generateButton(ch, tmenu.courtiers, 0, 0, false);
		tmenu.courtiers->RecalcPos();
		

		tmenu.showcontrolled->setVisible(true);
		tmenu.showcontrolled->clickaction = [ad](uiButton* b) {
			global::uiTasks.run([ad] {
				global::clear_highlight();
				cvector<prov_id_t> provs;
				{
					r_lock l;
					global::get_controlled_by_admin(ad, provs, l);
				}
				for (const auto p : provs)
					get_object(p).pflags |= PROV_FLAG_HIHGLIGHT;

				bool hasrect = false;
				sf::IntRect base;
				for (const auto x : provs) {
					INOUT(auto) px = get_object(x);
					if (hasrect) {
						MergeRects(base, px.bounds);
					} else {
						base = px.bounds; hasrect = true;
					}
				}
				if (hasrect) {
					make_point_visible(glm::dvec2(base.left + base.width / 2, base.top + base.height / 2), global::lockWindow());
				}
			});
		};
	}
	

	tmenu.showdj->clickaction = [f = focused](uiButton* b) {
		global::uiTasks.run([focused = f] {
			global::clear_highlight();
			cvector<prov_id_t> provs;
			global::get_dj_controlled_by_t(focused, provs, r_lock());
			for (const auto p : provs)
				get_object(p).pflags |= PROV_FLAG_HIHGLIGHT;

			bool hasrect = false;
			sf::IntRect base;
			for (const auto x : provs) {
				INOUT(auto) px = get_object(x);
				if (hasrect) {
					MergeRects(base, px.bounds);
				} else {
					base = px.bounds; hasrect = true;
				}

			}
			if (hasrect) {
				make_point_visible(glm::dvec2(base.left + base.width / 2, base.top + base.height / 2), global::lockWindow());
			}
		});
	};

	
}

void SetupTPane( title_id_t focused) noexcept {
	global::uiqueue.push([ focused]{
		global::HideAll();
		tmenu.titlemenu->setVisible(true);
		global::infowindows->setVisible(true);
		global::enterPane(3, focused.value);

		global::setFlag(FLG_PANEL_UPDATE);
	});
}


void InitTPane() noexcept {
	sf::Color mback(200, 200, 200);
	sf::Color trans(0, 0, 0, 0);


	

	tmenu.titlemenu = std::make_shared<uiVRect>(0, 0, M_WIDTH, global::infowindows->pos.height, global::infowindows);
	global::infowindows->subelements.push_back(tmenu.titlemenu);
	tmenu.titlemenu->setVisible(false);
	tmenu.titlemenu->add_element<uiSimpleText>(15, 15, get_simple_string(TX_L_TITLE), global::empty, global::header_text);

	tmenu.holder = generateTButton( 0, tmenu.titlemenu, 5, 30, true);

	// tmenu.titlemenu->subelements.push_back(tmenu.leigechain = std::make_shared<uiVRect>(M_WIDTH - (5+64*4), 30, 64 * 4, 64, tmenu.titlemenu));

	tmenu.name = tmenu.titlemenu->add_element<uiSimpleText>(45 + 35, 5 + 40, TEXT("NAME"), global::empty, global::header_text);

	static const std::vector<std::wstring> ttabstxt = { get_simple_string(TX_T_TERRITORY), get_simple_string(TX_T_VASSALS), get_simple_string(TX_T_COURT), get_simple_string(TX_T_LAWS)};
	tmenu.tabs = std::make_shared<uiTabs>(0, 100, M_WIDTH, tmenu.titlemenu->pos.height - 100, tmenu.titlemenu);
	tmenu.tabs->init(global::header_text, global::solid_border, ttabstxt);
	tmenu.titlemenu->subelements.push_back(tmenu.tabs);

	static const std::vector<std::wstring> statnames = {get_simple_string(TX_L_ANALYTIC), get_simple_string(TX_L_MARTIAL), get_simple_string(TX_L_SOCIAL), get_simple_string(TX_L_INTRIGUE)};
	tmenu.attributes = tmenu.tabs->panes[2]->add_element<uiPropList>(5, 5, 300, global::empty, global::standard_text, statnames);

	tmenu.courtiers = tmenu.tabs->panes[2]->add_element<uiHozContainer>(5, 80, tmenu.tabs->panes[2]->pos.width, 68, 10, get_simple_string(TX_L_COURTIERS), global::empty, global::header_text);

	tmenu.showcontrolled = tmenu.tabs->panes[0]->add_element<uiButton>(5, 5, 200, 20, get_simple_string(TX_SHOW_CONTROL), global::solid_border, global::standard_text, [](uiButton* b) {});
	
	tmenu.showdj = tmenu.tabs->panes[0]->add_element<uiButton>(5, 30, 200, 20, get_simple_string(TX_SHOW_DJ), global::solid_border, global::standard_text, [](uiButton* b) {});
}

void hide_tm() noexcept {
	tmenu.titlemenu->setVisible(false);
}
