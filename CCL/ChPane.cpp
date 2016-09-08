#include "globalhelpers.h"
#include "ChPane.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "DynPane.h"
#include "mapdisplay.h"
#include "pacts.h"
#include "CulPane.h"
#include "RelPane.h"
#include "DynPane.h"
#include "TPane.h"
#include "PlansWindow.h"
#include "WarPane.h"
#include "relations.h"
#include "living_data.h"
#include "traits.h"
#include "i18n.h"
#include "finances.h"
#include "datamanagement.hpp"
#include "spies.h"
#include "actions.h"
#include "laws.h"

using namespace boost::gregorian;

struct _cmenu {
	std::shared_ptr<uiVRect> chmenu;
	std::shared_ptr<uiGButton> self;

	std::shared_ptr<uiGButton> mind;
	std::shared_ptr<ui_toggle_button> focusedbutton;
	std::shared_ptr<uiROButton> takeaction;
	std::shared_ptr<uiGButton> wars;
	std::shared_ptr<uiGButton> envoys;
	std::shared_ptr<uiGButton> spies;

	std::shared_ptr<uiSimpleText> name;
	std::shared_ptr<uiHLink> culture;
	std::shared_ptr<uiHLink> religion;
	std::shared_ptr<uiHLink> dynasty;

	std::shared_ptr<uiSimpleText> born;
	std::shared_ptr<uiSimpleText> died;

	std::shared_ptr<uiSimpleText> troops;

	std::shared_ptr<uiTabs> tabs;

	std::shared_ptr<uiScrollView> relations;
	std::shared_ptr<uiScrollView> polrelations;

	std::shared_ptr<uiHozContainer> parents;
	std::shared_ptr<uiHozContainer> spouse;
	std::shared_ptr<uiHozContainer> children;
	std::shared_ptr<uiHozContainer> siblings;
	std::shared_ptr<uiHozContainer> friends;
	std::shared_ptr<uiHozContainer> enemies;
	std::shared_ptr<uiHozContainer> hated;

	std::shared_ptr<uiHozContainer> traits;

	std::shared_ptr<uiHozContainer> peacetreaty;

	std::shared_ptr<uiScrollView> titles;
	std::shared_ptr<uiScrollView> finances;

	std::shared_ptr<uiButton> showcontrolled;
	std::shared_ptr<uiButton> showdj;

	std::shared_ptr<uiPropList> attributes;
} cmenu;

void inline_update_ch_pane(char_id_t focused) noexcept {
	if (!cmenu.chmenu->gVisible())
		return;

	
	r_lock l;

	double tax = global::project_income(focused, l);
	sf::Color mback(200, 200, 200);

	IN(auto) p = get_object(focused);

	if (valid_ids(p.primetitle)) {
		cmenu.self->setVisible(true);
		UpdateTIcon(cmenu.self, p.primetitle, 64);
	} else {
		cmenu.self->setVisible(false);
	}

	cmenu.parents->subelements.clear();
	cmenu.spouse->subelements.clear();
	cmenu.children->subelements.clear();
	cmenu.friends->subelements.clear();
	cmenu.siblings->subelements.clear();
	cmenu.enemies->subelements.clear();
	cmenu.hated->subelements.clear();
	cmenu.titles->subelements.clear();
	cmenu.peacetreaty->subelements.clear();

	cmenu.polrelations->subelements.clear();
	cmenu.polrelations->subelements.push_back(cmenu.peacetreaty);

	cmenu.hated->setVisible(focused == global::playerid);


	cmenu.mind->clickaction = [focused](uiGButton* b) {SetupPlansWindow(char_id_t(focused)); };

	cmenu.focusedbutton->set_state(global::interested.in_set(focused.value));
	cmenu.focusedbutton->clickaction = [focused](ui_toggle_button* b) {
		if (focused == global::playerid) {
			b->set_state(true);
			return;
		}
		if (global::interested.in_set(focused.value)) {
			global::interested.remove(focused.value);
		} else {
			global::interested.add(focused.value);
		}
	};


	cmenu.takeaction->clickaction = [focused](uiROButton *b) {
		createChActionMenu(global::uicontainer, global::infowindows->pos.left + cmenu.takeaction->pos.left + cmenu.takeaction->pos.width, global::infowindows->pos.top + cmenu.takeaction->pos.top, focused.value);
	};

	cmenu.wars->clickaction = [focused](uiGButton* b) {
		const auto pa = get_prime_admin(char_id_t(focused), r_lock());
		if (valid_ids(pa)) {
			global::mapmode = MAP_MODE_WAR;
			global::map.displayedwar.adm = pa.value;
			global::map.displayedwar.wid = newwar::NONE;
			global::setFlag(FLG_MAP_UPDATE);
			SetupWarPane(global::map.displayedwar.adm, global::map.displayedwar.wid);
		}
	};

	cmenu.finances->subelements.clear();

	if (p.died == 0) {
		date bday = date(1400, Jan, 1) + days(p.born);
		date cday = date(1400, Jan, 1) + days(global::currentday);
		int age = (cday.year() - bday.year()) - ((bday.month() < cday.month() || (bday.month() == cday.month() && bday.day() < cday.day())) ? 1 : 0);
		cmenu.died->updateText(get_simple_string(TX_L_AGE) + std::to_wstring(age));

		double money = with_udata(focused, l, 0.0, [](IN(udata) d) noexcept { return d.wealth; });
		size_t param = dumb_cast<size_t>(money);
		cmenu.finances->add_element<uiSimpleText>(5, 5, get_p_string(TX_L_WEALTH, &param, 1), global::empty, global::standard_text);

		int y = 25;
		param = dumb_cast<size_t>(tax);
		cmenu.finances->add_element<uiSimpleText>(5, y, get_p_string(TX_FORMAT_INCOME, &param, 1), global::empty, global::standard_text);
		y += global::standard_text.csize + 2;
		ex_income_to_ui(cmenu.finances, 5, y, char_id_t(focused), l);
		y += 15;
		expenses_to_ui(cmenu.finances, 5, y, char_id_t(focused), l);
	} else {
		cmenu.died->updateText(std::wstring(TEXT("D: ")) + w_day_to_string_short(p.died));
		//cmenu.money->updateText(TEXT(""));

		//cmenu.tax->updateText(TEXT(""));
	}
	cmenu.finances->calcTotalHeight();

	if (with_udata(focused, l, true, [](IN(udata) d) noexcept {
		cmenu.attributes->values[0] = std::to_wstring(d.stats.analytic);
		cmenu.attributes->values[1] = std::to_wstring(d.stats.martial);
		cmenu.attributes->values[2] = std::to_wstring(d.stats.social);
		cmenu.attributes->values[3] = std::to_wstring(d.stats.intrigue);

		cmenu.traits->subelements.clear();
		for (unsigned char i = 0; i < MAX_TRAITS; ++i) {
			if (d.attrib.has_negative_N(i))
				get_neg_icon(i, 0, 0, cmenu.traits);
			else if (d.attrib.has_positive_N(i))
				get_pos_icon(i, 0, 0, cmenu.traits);
		}
		cmenu.traits->RecalcPos();

		cmenu.attributes->values[4] = std::to_wstring(d.p_honorable);
		cmenu.attributes->values[5] = std::to_wstring(d.p_peaceful);
		cmenu.attributes->values[6] = std::to_wstring(d.activity);


		cmenu.culture->updateText(str_to_wstr(get_object(d.culture).name.get()));
		cmenu.culture->updateAction([culture = d.culture](uiHLink* b) {SetupCulPane(culture); });

		cmenu.religion->updateText(str_to_wstr(get_object(d.religion).name.get()));
		cmenu.religion->updateAction([religion = d.religion](uiHLink* b) {SetupRelPane(religion); });

		return false;
	})) {
		cmenu.traits->subelements.clear();
		cmenu.attributes->values[0] = TEXT("?");
		cmenu.attributes->values[1] = TEXT("?");
		cmenu.attributes->values[2] = TEXT("?");
		cmenu.attributes->values[3] = TEXT("?");
		cmenu.attributes->values[4] = TEXT("?");
		cmenu.attributes->values[5] = TEXT("?");
		cmenu.attributes->values[6] = TEXT("?");
	}


	cmenu.died->pos.left = cmenu.chmenu->pos.width - static_cast<int>(cmenu.died->textBounds(global::font).width) - 20;

	cmenu.born->updateText(TEXT("B: ") + w_day_to_string_short(p.born));
	cmenu.born->pos.left = cmenu.chmenu->pos.width - static_cast<int>(cmenu.born->textBounds(global::font).width) - 20;

	float sg_sq = 0.0;
	float mu = 0.0;
	get_force_estimate(focused, mu, sg_sq, l);

	if (mu > 0.0) {
		size_t params[] = {dumb_cast<size_t>(static_cast<double>(mu)), dumb_cast<size_t>(sqrt(static_cast<double>(sg_sq)))};
		cmenu.troops->updateText(get_p_string(TX_L_TROOPS, params, 2));
	} else {
		cmenu.troops->updateText(get_simple_string(TX_L_NOARMY));
	}
	cmenu.troops->pos.left = cmenu.chmenu->pos.width - static_cast<int>(cmenu.troops->textBounds(global::font).width) - 20;

	if (valid_ids(p.mother)) {
		generateButton(p.mother, cmenu.parents, 0, 0, false);
	}
	if (valid_ids(p.father)) {
		generateButton(p.father, cmenu.parents, 0, 0, false);
	}
	cmenu.parents->RecalcPos();

	std::vector<char_id_t> ppl;
	global::get_spouses(focused, ppl);
	for (const auto ch : ppl)
		generateButton(char_id_t(ch), cmenu.spouse, 0, 0, false);
	ppl.clear();

	global::get_children(focused, ppl);
	for (const auto ch : ppl)
		generateButton(ch, cmenu.children, 0, 0, false);
	ppl.clear();

	global::get_siblings(focused, ppl);
	for(const auto c : ppl)
		generateButton(c, cmenu.siblings, 0, 0, false);

	cmenu.siblings->RecalcPos();
	cmenu.spouse->RecalcPos();
	cmenu.children->RecalcPos();

	enum_friends(focused, l, [](char_id_t c) {
		generateButton(c, cmenu.friends, 0, 0, false);
	});
	cmenu.friends->RecalcPos();
	enum_enemies(focused, l, [](char_id_t c) {
		generateButton(c, cmenu.enemies, 0, 0, false);
	});
	cmenu.enemies->RecalcPos();
	if (focused == global::playerid) {
		enum_hated(char_id_t(focused), l, [](char_id_t c) {
			generateButton(c, cmenu.hated, 0, 0, false);
		});
		cmenu.hated->RecalcPos();
	}

	enum_peace_treaties(focused, l, [](char_id_t c, unsigned int duedate) {
		const auto button = generateButton(c, cmenu.peacetreaty, 0, 0, false);
		date unt = date(1400, Jan, 1) + days(duedate);
		size_t params[3] = {static_cast<size_t>(unt.month()), static_cast<size_t>(unt.year() - 1400), c.value };
		button->tt_text = get_p_string(TX_W_UNTIL, params, 3);
		//std::string("With ") + button->tt_text +  " until " + MonthToText(unt.month()) + " " + std::to_string(unt.day()) + ", " + std::to_string(unt.year() - 1400);
	});
	cmenu.peacetreaty->RecalcPos();

	cmenu.polrelations->add_element<uiSimpleText>(5, 68, get_simple_string(TX_L_PACTS), global::empty, global::header_text);
	int yoff = 68 + 20;

	enum_pacts_for(focused, l, [&l, focused, pane = cmenu.polrelations, &yoff](pact_id_t id) {
		pact_to_ui(5, yoff, pane, get_object(id, l), l);
		if (focused == global::playerid) {
			
			pane->add_element<uiButton>(5, yoff+2, 200, 20, get_simple_string(TX_BREAK_PACT), global::solid_border, global::standard_text, [id](uiButton*) {
				size_t param = 0;
				{
					r_lock l;
					double hloss = honor_loss_on_break_val(get_object(id, l), char_id_t(global::playerid), l);
					param = to_param(hloss);
				}

				int strss = with_udata(char_id_t(global::playerid), r_lock(), -1, [](IN(udata) d) noexcept {
					if (is_deceitful(d))
						return 0;
					else if (is_honest(d))
						return -2;
					return -1;
				});

				i18n_modeless_yes_no_popup(TX_BREAK_PACT, TX_BREAK_PACT_WARNING, strss, &param, 1, [id](bool b) {
					if (b)
						global::actionlist.add_new<break_pact_a>(char_id_t(global::playerid), id);
				});
				
				
			});
			yoff += 25;
		}
	});


	cmenu.name->updateText(str_to_wstr(p.name.get()));
	cmenu.dynasty->pos.left = cmenu.name->pos.left + cmenu.name->pos.width + static_cast<int>((global::font->getTextBounds(" ", 18)).width);
	cmenu.dynasty->updateText(str_to_wstr(get_object(p.dynasty).name.get()));
	cmenu.dynasty->updateAction([dynint = p.dynasty](uiHLink* b) {SetupDynPane(dynint); });


	cmenu.showcontrolled = cmenu.titles->add_element<uiButton>(5, 5, 200, 20, get_simple_string(TX_SHOW_CONTROL), global::solid_border, global::standard_text, [fo = focused](uiButton* b) {
		global::uiTasks.run([f = fo] {
			global::clear_highlight();
			cvector<prov_id_t> provs;
			{
				r_lock l;
				global::get_nom_controlled(char_id_t(f), provs, l);
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
	});
	cmenu.showdj = cmenu.titles->add_element<uiButton>(5, 30, 200, 20, get_simple_string(TX_SHOW_DJ), global::solid_border, global::standard_text, [fo = focused](uiButton* b) {
		global::uiTasks.run([f = fo] {
			global::clear_highlight();
			cvector<prov_id_t> provs;
			{
				r_lock l;
				global::get_dj_controlled(f, provs, l);
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
	});

	int tcount = 0;
	global::holdertotitle.for_each(focused, l, [&tcount](title_id_t t) {
		generateTButton(t, cmenu.titles, 5, 5 + 55 + 45 * tcount, false);
		cmenu.titles->add_element<uiSimpleText>(50, 20 + 55 + 45 * tcount, global::w_title_name(t), global::empty, global::standard_text);
		tcount++;
	});

	cmenu.titles->calcTotalHeight();
	cmenu.polrelations->calcTotalHeight();
	cmenu.relations->calcTotalHeight();
}

void SetupChPane(char_id_t focused) noexcept {
	global::uiqueue.push([ focused]{
		global::HideAll();
		cmenu.chmenu->setVisible(true);
		global::infowindows->setVisible(true);
		global::enterPane(2, focused.value);

		
		global::setFlag(FLG_PANEL_UPDATE);
	});
}

void InitChPane() noexcept {
	sf::Color mback(200, 200, 200);
	sf::Color trans(0, 0, 0, 0);

	cmenu.chmenu = std::make_shared<uiVRect>(0, 0, M_WIDTH, global::infowindows->pos.height, global::infowindows);
	global::infowindows->subelements.push_back(cmenu.chmenu);
	cmenu.chmenu->setVisible(false);
	cmenu.chmenu->add_element<uiSimpleText>(15, 15, get_simple_string(TX_L_CHARACTER), global::empty, global::header_text);
	cmenu.self = generateTButton( 0, cmenu.chmenu, 5, 30, true); //std::make_shared<uiGButton>(5, 30, 64, 64, global::cmenu.chmenu, &global::ctex, sf::IntRect((64 * 5), (3 * 64), 64, 64), "Tool-tip", font, 14, [](uiGButton *a) {});
	//global::cmenu.chmenu->subelements.push_back(global::cmenu.self);
	cmenu.name = cmenu.chmenu->add_element<uiSimpleText>(45 + 35, 5 + 40, TEXT("NAME"), global::empty, global::header_text);

	cmenu.dynasty = cmenu.chmenu->add_element<uiHLink>(45 + 40, 5 + 40, TEXT("DNAMW"), global::empty, global::header_text, global::whandle);

	cmenu.culture = cmenu.chmenu->add_element<uiHLink>(45 + 35, 27 + 40, TEXT("CULTURE"), global::empty, global::standard_text, global::whandle);
	cmenu.religion = cmenu.chmenu->add_element<uiHLink>(45 + 35, 42 + 40, TEXT("RELIGION"), global::empty, global::standard_text, global::whandle);

	cmenu.born = cmenu.chmenu->add_element<uiSimpleText>(M_WIDTH, 5 + 40, TEXT("BORN"), global::empty, global::standard_text);

	cmenu.died = cmenu.chmenu->add_element<uiSimpleText>(M_WIDTH, 20 + 40, TEXT("DIED"), global::empty, global::standard_text);
	cmenu.troops = cmenu.chmenu->add_element<uiSimpleText>(M_WIDTH, 35 + 40, TEXT("TROOPS"), global::empty, global::standard_text);
	
	cmenu.mind = cmenu.chmenu->add_element<uiGButton>(205, 8, 32, 32, global::iconstrip.get(25, 0), get_simple_string(TX_SHOW_PLANS), global::tooltip_text, [](uiGButton *a) { });
	cmenu.focusedbutton = cmenu.chmenu->add_element<ui_toggle_button>(170, 8, 32, 32, paint_states<2>(global::messages_button_tex.get(1,0), global::messages_button_tex.get(0,0)), get_simple_string(TX_SHOW_MESSAGE), global::tooltip_text, [](ui_toggle_button *a) {});
	
	cmenu.takeaction = cmenu.chmenu->add_element<uiROButton>(125, 1, 40, 40, paint_states<2>(global::takeaction_tex.get(0,0), global::takeaction_tex.get(1, 0)), get_simple_string(TX_TAKE_ACTION), global::tooltip_text, [](uiROButton *a) {});
	
	cmenu.wars = cmenu.chmenu->add_element<uiGButton>(241, 8, 32, 32, global::iconstrip.get(2,0), get_simple_string(TX_SHOW_WARS), global::tooltip_text, [](uiGButton *a) {});
	cmenu.envoys = cmenu.chmenu->add_element<uiGButton>(277, 8, 32, 32, global::iconstrip.get(6, 0), get_simple_string(TX_DIPLO_MISSIONS), global::tooltip_text, [](uiGButton *a) {
		const auto t = global::get_prime_title(char_id_t(global::playerid));
			
		IN(auto) to = get_object(t);
		if(valid_ids(t, to.associated_admin))
			open_envoys_window_a(to.associated_admin);
	});
	cmenu.spies = cmenu.chmenu->add_element<uiGButton>(313, 8, 32, 32, global::iconstrip.get(11, 0), get_simple_string(TX_ESPIONAGE_MISSIONS), global::tooltip_text, [](uiGButton *a) {open_intrigue_window(); });

	const std::vector<std::wstring> ctabstxt = { get_simple_string(TX_T_RELATIONSHIPS), get_simple_string(TX_T_TITLES), get_simple_string(TX_T_POLITICAL), get_simple_string(TX_T_DETAILS), get_simple_string(TX_T_FINANCES)};
	cmenu.tabs = std::make_shared<uiTabs>(0, 100, cmenu.chmenu->pos.width, cmenu.chmenu->pos.height - 100, cmenu.chmenu );
	cmenu.tabs->init(global::header_text, global::solid_border, ctabstxt);
	cmenu.chmenu->subelements.push_back(cmenu.tabs);

	cmenu.finances = cmenu.tabs->panes[4];

	cmenu.relations = cmenu.tabs->panes[0];
	cmenu.relations->margin = 5;


	//parents
	cmenu.parents = cmenu.relations->add_element<uiHozContainer>(5, 0, 120, 68, 10, get_simple_string(TX_L_PARENTS), global::empty, global::header_text);
	cmenu.spouse = cmenu.relations->add_element<uiHozContainer>(180, 0, 100, 68, 10, get_simple_string(TX_L_SPOUSE), global::empty, global::header_text);
	cmenu.siblings = cmenu.relations->add_element<uiHozContainer>(5, 68 * 1, cmenu.relations->pos.width - 15, 68, 10, get_simple_string(TX_L_SIBLINGS), global::empty, global::header_text);
	cmenu.children = cmenu.relations->add_element<uiHozContainer>(5, 68 * 2, cmenu.relations->pos.width - 15, 68, 10, get_simple_string(TX_L_CHILDREN), global::empty, global::header_text);

	cmenu.friends = cmenu.relations->add_element<uiHozContainer>(5, 68 * 3, cmenu.relations->pos.width - 5, 68, 10, get_simple_string(TX_L_FRIENDS), global::empty, global::header_text);
	cmenu.enemies = cmenu.relations->add_element<uiHozContainer>(5, 68 * 4, cmenu.relations->pos.width - 5, 68, 10, get_simple_string(TX_L_ENEMIES), global::empty, global::header_text);
	cmenu.hated = cmenu.relations->add_element<uiHozContainer>(5, 68 * 5, cmenu.relations->pos.width - 5, 68, 10, get_simple_string(TX_L_HATED), global::empty, global::header_text);


	cmenu.titles = cmenu.tabs->panes[1];
	cmenu.titles->margin = 5;

	cmenu.polrelations = cmenu.tabs->panes[2];
	cmenu.polrelations->margin = 5;

	cmenu.peacetreaty = std::make_shared<uiHozContainer>(cmenu.polrelations, 5, 68 * 0, cmenu.relations->pos.width - 15, 68, 10, get_simple_string(TX_L_TREATIES), global::empty, global::header_text);
	//global::cmenu.polrelations->subelements.push_back(global::cmenu.peacetreaty);

	cmenu.traits = cmenu.tabs->panes[3]->add_element<uiHozContainer>(5, 5, cmenu.tabs->panes[3]->pos.width - 15, 50, 10, get_simple_string(TX_L_TRAITS), global::empty, global::header_text);

	//cmenu.money =  cmenu.tabs->panes[3]->add_element<uiSimpleText>(5, 60, TEXT("MONEY"), global::empty, global::standard_text);
	//cmenu.tax =  cmenu.tabs->panes[3]->add_element<uiSimpleText>(5, 80, TEXT("TAX"), global::empty, global::standard_text);

	static const std::vector<std::wstring> statnames = {get_simple_string(TX_L_ANALYTIC), get_simple_string(TX_L_MARTIAL),
		get_simple_string(TX_L_SOCIAL), get_simple_string(TX_L_INTRIGUE), get_simple_string(TX_L_HONORABLE), get_simple_string(TX_L_PEACEFUL),
		get_simple_string(TX_L_ACTIVITY)};
	cmenu.attributes = cmenu.tabs->panes[3]->add_element<uiPropList>(5, 100, 300, global::empty, global::standard_text, statnames);

	cmenu.showcontrolled = nullptr;
	cmenu.showdj = nullptr;
}

void hide_chm() noexcept {
	cmenu.chmenu->setVisible(false);
}
