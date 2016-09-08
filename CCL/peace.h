#pragma once
#include "globalhelpers.h"
#include "wardata.h"
#include "requests.h"
#include "laws.h"

class implement_peace_offer;
class peace_deal;
void implement_peace_offer_fac(char_id_t, peace_deal&&) noexcept;

class peace_deal {
public:
	struct deal_part {
		wargoal goal;
		admin_id_t deal_for;

		deal_part(admin_id_t f, wargoal g) noexcept : deal_for(f), goal(g) {};
	};
	
	war_id_t assocated_war;
	admin_id_t offer_from;
	bool is_demand;

	flat_map<char_id_t, double> value_distribution;
	std::vector<deal_part> goals;

	void remove_goal(size_t n, IN(g_lock) l) noexcept;
	double total_value() const noexcept;

	peace_deal() noexcept {};

	void reset() noexcept;

	bool can_demand(IN(newwar) wr, IN(newwar) other, IN(flat_set<admin_id_t>) apart, IN(flat_set<admin_id_t>) bpart, IN(std::vector<prov_id_t>) nom_a_controlled, IN(std::vector<prov_id_t>) nom_b_controlled, IN(w_lock) l) const noexcept;
	bool can_demand(IN(newwar) wr, IN(newwar) other, IN(cflat_set<admin_id_t>) apart, IN(cflat_set<admin_id_t>) bpart, IN(cvector<prov_id_t>) nom_a_controlled, IN(cvector<prov_id_t>) nom_b_controlled, IN(g_lock) l) const noexcept;


	bool offer_valid(IN(g_lock) l) noexcept;
	bool offer_valid(IN(w_lock) l) noexcept;

	bool make_offer(INOUT(r_lock) l) noexcept;
	bool make_offer(INOUT(w_lock) l) noexcept;
	void make_demand(IN(g_lock) l) noexcept;
	void implement_offer(INOUT(w_lock) l) const noexcept;


	void init(war_id_t war_for, IN(newwar) wr, IN(newwar) other, double v, IN(w_lock) l) noexcept;
	void init(war_id_t war_for, IN(newwar) wr, IN(newwar) other, double v, IN(g_lock) l) noexcept;
	void init_w_goals(war_id_t war_for, IN(std::vector<deal_part>) goals_in, IN(newwar) wr, double v, IN(g_lock) l) noexcept;
	void generate_offer_to_value(IN(newwar) wr, IN(newwar) other, IN(g_lock) l) noexcept;
	void generate_demand_to_value(IN(newwar) wr, IN(newwar) other, IN(g_lock) l) noexcept;
	void generate_offer_to_value(IN(newwar) wr, IN(newwar) other, IN(w_lock) l) noexcept;
	void generate_demand_to_value(IN(newwar) wr, IN(newwar) other, IN(w_lock) l) noexcept;

	void to_ui(IN(std::shared_ptr<uiElement>) parent, int x, INOUT(int) y, IN(g_lock) l) const noexcept;
};