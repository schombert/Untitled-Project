#include "globalhelpers.h"
#include "PlansWindow.h"
#include "structs.hpp"
#include "i18n.h"
#include "ai_interest.h"
#include "datamanagement.hpp"
#include "ChPane.h"

#define PWIN_WIDTH 200
#define PWIN_HEIGHT 400

struct {
	std::shared_ptr<uiDragRect> mainwindow;
	std::shared_ptr<uiGButton> close;

	std::shared_ptr<uiScrollView> contents;
} planwin;

struct plandecript {
	const TCHAR * text;
	const unsigned short planid;
	std::vector<plandecript> subplans;
	plandecript(const TCHAR* const t, const unsigned short pid) :text(t), planid(pid) {};
};

bool addPlan(const TCHAR* const t, const unsigned short pid, const unsigned short parent, std::vector<plandecript> &plans) noexcept {
	for (auto & plan : plans) {
		if (plan.planid == parent) {
			plan.subplans.emplace_back(t, pid);
			return true;
		} else if (addPlan(t, pid, parent, plan.subplans)) {
				return true;
		}
	}
	return false;
}

void inserttext(std::shared_ptr<uiScrollView> &parent, int &y, int x, const plandecript &plans) noexcept {
	parent->add_element<uiSimpleText>(x,y, plans.text, global::empty, global::standard_text);
	y += 17;
	for (const auto & plan : plans.subplans) {
		inserttext(parent, y, x + 15, plan);
	}
}

void SetupPlansWindow( char_id_t focused) noexcept {
	global::uiqueue.push([ focused] {
		planwin.mainwindow->setVisible(true);
		planwin.mainwindow->toFront(global::uicontainer);

		planwin.contents->subelements.clear();

		{
			r_lock l;

			int threats = 0;
			int targets = 0;
			int pthreats = 0;
			get_interest_totals(focused, threats, targets, pthreats, l);

			std::wstring tst = L"ta: " + std::to_wstring(targets) + L" th: " + std::to_wstring(threats);
			planwin.contents->add_element<uiSimpleText>(1, 1, tst, global::empty, global::standard_text);

			int y = 22;
			std::string build;
			for (auto rng = interested_in.equal_range(focused); rng.first != rng.second; ++rng.first) {
				auto v = rng.first->second;
				auto tg = interest_status_of(focused, v, l);
				if (tg == interest_relation_contents::TAG_THREAT)
					build = "Threat: ";
				else if (tg == interest_relation_contents::TAG_TARGET)
					build = "Target: ";
				else if (tg == interest_relation_contents::TAG_NONE)
					build = "None: ";
				else
					build = "Other: ";
				build += global::character_name(v);

				planwin.contents->add_element<uiHLink>(1, y, str_to_wstr(build), global::empty, global::standard_text, global::whandle, [v](uiHLink*) { SetupChPane(v);  });
				y += 20;
			}
			
		}

		/*
		std::vector<plandata> planlist;
		global::getPlans(focused, planlist);
		global::planwin.contents->subelements.clear();

		std::vector<plandecript> plans;
		for (size_t indx = 0; indx < planlist.size(); ++indx) {
			if (planlist[indx].returnplan == NO_PLAN) {
				plans.emplace_back(PlanBase::idsmap[planlist[indx].planid]->description(), static_cast<unsigned short>(indx));
			} else {
				addPlan(PlanBase::idsmap[planlist[indx].planid]->description(), indx, planlist[indx].returnplan, plans);
			}
		}
		int y = 0;
		for (const auto &plan : plans) {
			inserttext(global::planwin.contents, y, 0, plan, global);
		}*/
		planwin.contents->scrolloff = 0;
		planwin.contents->calcTotalHeight();
		
	});
}

void InitPlansWindow( sf::Font* const font) noexcept {
	sf::Color mback(200, 200, 200);
	sf::Color trans(0, 0, 0, 0);

	planwin.mainwindow = global::uicontainer->add_element<uiDragRect>(10, 10, PWIN_WIDTH, PWIN_HEIGHT, global::solid_border);
	planwin.mainwindow->setVisible(false);
	planwin.close = planwin.mainwindow->add_element<uiGButton>(PWIN_WIDTH-20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) {  planwin.mainwindow->setVisible(false); });
	planwin.mainwindow->subelements.emplace_back(planwin.contents = std::make_shared<uiScrollView>(0, 20, PWIN_WIDTH, PWIN_HEIGHT-20, planwin.mainwindow));
	planwin.contents->margin = 5;
}