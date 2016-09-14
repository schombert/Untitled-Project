#pragma once
#include "globalhelpers.h"
#include "political_action.h"
#include "political_action_template.h"


POL_ACTION_CLASS(pa_war_declaration, war_action) {
public:
	const wargoal wg;
	const char_id_t target;

	pa_war_declaration(admin_id_t adm, char_id_t tar, wargoal goal) : war_action(adm), wg(goal), target(tar) {}


	virtual void display_description(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(g_lock) l) const override;
	virtual bool is_possible(IN(g_lock) l) const override;

	void do_action(INOUT(w_lock) l) const;
	double evaluate_action(char_id_t by, IN(g_lock) l) const;

	template<typename T, typename L>
	void influences_involved(INOUT(std::vector<influence_against, T>) v, IN(L) l) const {
		const static influence_against saved_type(influence_against::iad(target), influence_against::WAR_WITH);
		v.push_back(saved_type);
	}
};