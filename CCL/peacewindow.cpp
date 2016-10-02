#include "globalhelpers.h"
#include "peacewindow.h"
#include "structs.hpp"
#include "wardata.h"
#include "ChPane.h"
#include "ProvPane.h"
#include "TPane.h"
#include "i18n.h"
#include "living_data.h"
#include "peace.h"
#include "datamanagement.hpp"
#include "WarPane.h"
#include "prov_control.h"

#define PWIN_WIDTH 500
#define PWIN_HEIGHT 400

struct {
	std::shared_ptr<uiDragRect> pwindow;
	std::shared_ptr<uiCenterdText> title;

	std::shared_ptr<uiScrollView> wargoals;
	std::shared_ptr<uiScrollView> provinces;

	std::shared_ptr<uiButton> makeoffer;
	std::shared_ptr<uiButton> reset;

	std::shared_ptr<uiCheckBox> showonmap;

	std::vector<peace_deal::deal_part> goals;
	peace_deal offer;
} pwin;

void InitPeaceWindow() noexcept {
	const sf::Color mback(200, 200, 200);

	pwin.pwindow =  global::uicontainer->add_element<uiDragRect>(20, 20, PWIN_WIDTH, PWIN_HEIGHT, global::solid_border);
	pwin.pwindow->setVisible(false);

	pwin.pwindow->add_element<uiGButton>(PWIN_WIDTH - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) {pwin.pwindow->setVisible(false); });

	pwin.title =  pwin.pwindow->add_element<uiCenterdText>(0, 5, PWIN_WIDTH, TEXT("TITLE"), global::empty, global::header_text);

	pwin.pwindow->add_element<uiCenterdText>(0, 25, PWIN_WIDTH/2, get_simple_string(TX_WAR_GOALS), global::empty, global::standard_text);
	pwin.pwindow->add_element<uiCenterdText>(PWIN_WIDTH / 2, 25, PWIN_WIDTH / 2, get_simple_string(TX_CUR_PEACE_OFFER), global::empty, global::standard_text);

	pwin.pwindow->subelements.push_back(pwin.wargoals = std::make_shared<uiScrollView>(5, 40, PWIN_WIDTH / 2 - 8, 260, pwin.pwindow));
	pwin.pwindow->subelements.push_back(pwin.provinces = std::make_shared<uiScrollView>(PWIN_WIDTH / 2 + 3, 40, PWIN_WIDTH / 2 - 8, 260, pwin.pwindow));


	pwin.makeoffer = pwin.pwindow->add_element<uiButton>(PWIN_WIDTH - 200, PWIN_HEIGHT - 25, 95, 20, get_simple_string(TX_MAKE_OFFER), global::solid_border, global::standard_text,
																								[](uiButton* b) {
		
		r_lock l;
		if (pwin.offer.offer_valid(l)) {
			pwin.pwindow->setVisible(false);
			if(pwin.offer.is_demand)
				pwin.offer.make_demand(l);
			else
				pwin.offer.make_offer(l);
		} else {
			message_popup(get_simple_string(TX_L_ERROR), [](IN(std::shared_ptr<uiElement>) sv) {
				create_tex_block(TX_DEAL_INVALID, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
			});
		}
	});
	pwin.reset = pwin.pwindow->add_element<uiButton>(PWIN_WIDTH - 100, PWIN_HEIGHT - 25, 95, 20, get_simple_string(TX_RESET), global::solid_border, global::standard_text,
																								[](uiButton* b) {
		SetupPeaceWindow(pwin.offer.assocated_war, pwin.offer.offer_from, pwin.offer.is_demand);
	});

	pwin.showonmap = pwin.pwindow->add_element<uiCheckBox>(PWIN_WIDTH - 200, PWIN_HEIGHT - 25 - 24 - 5, get_simple_string(TX_MAP_CK), global::standard_text, get_simple_string(TX_SHOW_EXPLANATION), global::tooltip_text, false, [](uiCheckBox* b) {
		if (b->toggled) {
			global::clear_highlight();
		}
		war_window_rec.needs_update = true;
		global::setFlag(FLG_WWIN_UPDATE);
	});

}

void setup_immediate(war_id_t war_for, admin_id_t offer_from, bool is_demand, bool use_g_list, IN(g_lock) l) noexcept {
	IN(auto) wp = war_pool.get(war_for.value, l);
	if (wp.date_started == 0 || (wp.attacker.primary != offer_from && wp.defender.primary != offer_from))
		return;

	pwin.offer.reset();

	IN(auto) aside = wp.attacker.primary == offer_from ? wp.attacker : wp.defender;
	IN(auto) oside = wp.attacker.primary == offer_from ? wp.defender : wp.attacker;

	if (!use_g_list) {
		pwin.goals.clear();

		for (IN(auto) p : aside.participants) {
			if (p.goal.type != wargoal::WARGOAL_NONE) {
				pwin.goals.emplace_back(admin_id_t(p.adm), p.goal);
			}
		}
	}

	cvector<prov_id_t> acontrolled;
	cvector<prov_id_t> bcontrolled;

	cflat_set<admin_id_t> all_a;
	cflat_set<admin_id_t> all_o;
	list_participants(aside, all_a, l);
	list_participants(oside, all_o, l);

	for (const auto a : all_a)
		controlled_by_admin(a, acontrolled, l);
	for (const auto a : all_o)
		controlled_by_admin(a, bcontrolled, l);

	const double warbase = war_base_tax_value(war_for, acontrolled, bcontrolled, l);

	if (is_demand) {
		if (use_g_list)
			pwin.offer.init_w_goals(war_for, pwin.goals, aside, warbase, l);
		else
			pwin.offer.init(war_for, aside, oside, warbase, l);
		pwin.offer.generate_demand_to_value(aside, oside, l);
		pwin.makeoffer->text.setString(get_simple_string(TX_MAKE_DEMAND));
	} else {
		if (use_g_list)
			pwin.offer.init_w_goals(war_for, pwin.goals, aside, warbase + war_prediction_value(war_for, wp.attacker.primary == offer_from, acontrolled, bcontrolled, l), l);
		else
			pwin.offer.init(war_for, aside, oside, warbase + war_prediction_value(war_for, wp.attacker.primary == offer_from, acontrolled, bcontrolled, l), l);
		pwin.offer.generate_offer_to_value(aside, oside, l);
		pwin.makeoffer->text.setString(get_simple_string(TX_MAKE_OFFER));
	}



	size_t param = head_of_state(oside.primary, l).value;
	pwin.title->text.setString(get_p_string(TX_PEACE_OFFER_TO, &param, 1));
}

void SetupPeaceWindow(war_id_t war_for, admin_id_t offer_from, bool is_demand) noexcept {
	global::uiqueue.push([war_for, offer_from, is_demand] {
		setup_immediate(war_for, offer_from, is_demand, false, r_lock());

		pwin.pwindow->setVisible(true);
		pwin.pwindow->toFront(global::uicontainer);
		global::setFlag(FLG_WWIN_UPDATE);
	});
}

void inline_update_peace_window() noexcept {
	if (!pwin.pwindow->gVisible())
		return;


	if (!valid_ids(pwin.offer.assocated_war))
		return;

	r_lock l;

	IN(auto) wp = war_pool.get(pwin.offer.assocated_war.value, l);
	if (wp.date_started == 0 || (pwin.offer.offer_from != wp.attacker.primary && pwin.offer.offer_from != wp.defender.primary))
		return;


	pwin.wargoals->subelements.clear();
	pwin.provinces->subelements.clear();

	if (!pwin.showonmap->toggled) {
		global::clear_highlight();
	}

	int y = 2;
	int gindex = 0;

	for (IN(auto) g : pwin.goals) {
		if (g.goal.type != wargoal::WARGOAL_NONE && head_of_state(g.deal_for, l) == global::playerid) {
			pwin.wargoals->add_element<uiButton>(0, y, 18, 18, TEXT("X"), global::solid_border, text_format{14, global::font, sf::Color::Red},
				[gindex](uiButton* b) {
				pwin.goals[gindex] = pwin.goals.back();
				pwin.goals.pop_back();

				setup_immediate(pwin.offer.assocated_war, pwin.offer.offer_from, pwin.offer.is_demand, true, r_lock());
				global::setFlag(FLG_WWIN_UPDATE);
			});

			display_goal_name(pwin.wargoals, 20, y, g.goal, l);
		}
		++gindex;
	}

	y = 2;
	pwin.offer.to_ui(pwin.provinces, 2, y, l);

	pwin.wargoals->calcTotalHeight();
	pwin.provinces->calcTotalHeight();
}

bool peace_window_open() noexcept {
	return pwin.pwindow->gVisible();
}
