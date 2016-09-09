#include "globalhelpers.h"
#include "wardata.h"
#include "structs.hpp"
#include <immintrin.h>
#include <amp_math.h>
#include <numeric>
#include "living_data.h"
#include "datamanagement.hpp"
#include "schedule.h"
#include "actions.h"
#include "i18n.h"
#include "prov_control.h"
#include "uielements.hpp"
#include "structs.hpp"
#include "finances.h"
#include "relations.h"
#include "requests.h"
#include "reputation.h"
#include "action_opinion.h"


//namespace global {
//	flat_map<title_id, std::weak_ptr<newtroop>> raisedtroops;
//};

multiindex<admin_id_t, war_id_t> wars_involved_in;
single_index_t<admin_id_t, newtroop> raised_troops;
v_pool_t<war_pair, war_id> war_pool;
single_index_t<prov_id_t, seiging_info> occupation_info;

#define DEF_VALUE_FOR(x) (global::project_income(x, l))
#define DEF_MONEY_MULTIPLIER 12.0
#define TAX_MULTIPLIER 12.0


void sort_by_proximity(INOUT(std::vector<prov_id_t>) lst, char_id_t person, IN(g_lock) l) {
	const auto cap = global::get_home_province(person, l);
	if (valid_ids(cap)) {
		IN(auto) capp = get_object(cap);
		std::sort(lst.begin(), lst.end(), [&capp](const prov_id_t a, const prov_id_t b) noexcept {
			return capp.dsq(get_object(a)) < capp.dsq(get_object(b)); });
	}
}

void sort_by_proximity(INOUT(std::vector<prov_id_t>) lst, prov_id_t cap, IN(g_lock) l) {
	IN(auto) capp = get_object(cap);
	std::sort(lst.begin(), lst.end(), [&capp](const prov_id_t a, const prov_id_t b) noexcept {
		return capp.dsq(get_object(a)) < capp.dsq(get_object(b)); });
	
}


void sort_by_proximity_reverse(INOUT(std::vector<prov_id_t>) lst, char_id_t person, IN(g_lock) l) {
	const auto cap = global::get_home_province(person, l);
	if (valid_ids(cap)) {
		IN(auto) capp = get_object(cap);
		std::sort(lst.begin(), lst.end(), [&capp](const prov_id_t a, const prov_id_t b) noexcept {
			return capp.dsq(get_object(a)) >= capp.dsq(get_object(b)); });
	}
}

void sort_by_proximity_reverse(INOUT(std::vector<prov_id_t>) lst, prov_id_t cap, IN(g_lock) l) {
	IN(auto) capp = get_object(cap);
	std::sort(lst.begin(), lst.end(), [&capp](const prov_id_t a, const prov_id_t b) noexcept {
		return capp.dsq(get_object(a)) >= capp.dsq(get_object(b)); });
}

template<typename T, typename L>
void sort_by_proximity_reverse_a(INOUT(std::vector<prov_id_t, T>) lst, admin_id_t adm, IN(L) l) {
	const auto cap = get_object(adm, l).capital;
	if (valid_ids(cap)) {
		IN(auto) capp = get_object(cap);
		std::sort(lst.begin(), lst.end(), [&capp](const prov_id_t a, const prov_id_t b) noexcept {
			return capp.dsq(get_object(a)) >= capp.dsq(get_object(b)); });
	}
}

war_id_t new_war_pair(char_id_t a, wargoal attacker_goal, char_id_t d, INOUT(w_lock) l) {
	const auto apt = get_object(get_object(a).primetitle).associated_admin;
	const auto bpt = get_object(get_object(d).primetitle).associated_admin;

	if (!valid_ids(apt, bpt))
		return war_id_t();

	const war_id_t id(war_pool.emplace(l, apt, bpt, global::currentday));
	INOUT(auto) wp = get_object(id, l);

	wp.fronts.first.target = global::get_home_province(d, l).value;
	wp.fronts.second.target = global::get_home_province(a, l).value;

	adjust_for_attack(a, d, l);

	with_udata_force(a, l, [type = attacker_goal.type](INOUT(udata) d) {
		if (type == wargoal::WARGOAL_CONQUEST)
			d.p_peaceful = update_reputation(reputation::p_attack_nonagressive, reputation::p_attack_agressive, d.p_peaceful);
	});

	flat_map<char_id_t, bool> asked;

	global::holdertotitle.for_each(a, l, [&wp, &asked, &l, a, d, id, &attacker_goal](title_id_t t) noexcept {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			add_to_war(wp.attacker, asked, to.associated_admin, a, d, attacker_goal, l);
		}
	});
	asked.clear();

	global::holdertotitle.for_each(d, l, [&wp, &asked, &l, id, a, d](title_id_t t) noexcept {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			add_to_war(wp.defender, asked, to.associated_admin, d, a, make_wg_none(), l);
		}
	});
	add_goal_to_war(wp.attacker, apt, attacker_goal, l);


	enum_defensive_against(d, a, l, [wid = id, target = d, actor = a, &wp, &l, &asked](char_id_t id, pact_id_t pact_id) noexcept {
		with_udata(id, l, [wid, target, actor, &wp, &l, &asked, id, pact_id](INOUT(udata) ld) noexcept {
			INOUT(auto) pct = get_object(pact_id, l);

			if (will_honor_def_pact(id, target, actor, pct, l) || !dishonor_def_pact_reaction(id, target, actor, pact_id, l)) {
				ld.p_honorable = update_reputation(reputation::p_honor_reliable, reputation::p_honor_unreliable, ld.p_honorable);
				adjust_relation(target, id, 1, l);

				asked[id] = true;
				global::holdertotitle.for_each(id, l, [&wp, &asked, &l, wid, id, a = actor](title_id_t t) noexcept {
					IN(auto) to = get_object(t);
					if (valid_ids(to.associated_admin)) {
						add_to_war(wp.defender, asked, to.associated_admin, id, a, make_wg_none(), l);
					}
				});

				if (global::playerid == target) {
					message_popup(global::uicontainer, get_simple_string(TX_DEF_CALL), [id](IN(std::shared_ptr<uiScrollView>) sv) noexcept {
						size_t param = id.value;
						const auto blk = create_tex_block(TX_DEF_CALL_HONORED, &param, 1, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
						sv->subelements.push_back(blk);
					});
				}
			} else {
				break_pact(pct, id, l);

				if (global::playerid == target) {
					message_popup(global::uicontainer, get_simple_string(TX_DEF_CALL), [id](IN(std::shared_ptr<uiScrollView>) sv) noexcept {
						size_t param = id.value;
						const auto blk = create_tex_block(TX_DEF_CALL_DISHONORED, &param, 1, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
						sv->subelements.push_back(blk);
					});
				}
			}
		});
	});


	flat_set<admin_id_t> p;
	list_participants(wp.attacker, p, l);
	for(const auto pa : p)
		wars_involved_in.insert(pa, id, l);
	raise_all_troops(p, l);
	p.clear();

	list_participants(wp.defender, p, l);
	for (const auto pa : p)
		wars_involved_in.insert(pa, id, l);
	raise_all_troops(p, l);

	update_expectation_changed(a, count_troops_raised(a, l), l);
	update_expectation_changed(d, count_troops_raised(d, l), l);

	balance_war(wp, true, l);
	balance_war(wp, false, l);

	adjust_relation(d, a, -3, l);

	return id;
}

void raise_all_troops(IN(flat_set<admin_id_t>) p, IN(w_lock) l) {
	for (const auto a : p) {
		if (!raised_troops.contains(a, l)) {
			unsigned short levy_sum = 0;
			global::enum_control_by_admin(a, l, [&levy_sum](IN(controlrecord) r) {
				levy_sum += static_cast<short>(r.tax * get_object(r.province).tax * 1000.0);
			});
			raised_troops.emplace(a, l, global::currentday + 30, levy_sum, static_cast<unsigned short>(200));
			//to_update[head_of_state(a, l)] += troops_to_points(levy_sum, 200);
		}
	}
	//for(const auto c : to_update)
	//	update_expectation_changed(c.first, c.second, l);
}

double points_in_adm_chain(admin_id_t a, IN(g_lock) l) {
	double total_pts = 0.0;
	if (raised_troops.contains(a, l)) {
		IN(auto) trp = raised_troops.get(a, l);
		total_pts += troops_to_points(trp.numlevy, trp.numstanding);
	}
	leigetoadmin.for_each(a, l, [&total_pts, &l](admin_id_t aa) {
		total_pts += points_in_adm_chain(aa, l);
	});
	return total_pts;
}

double count_troops_raised(char_id_t id, IN(g_lock) l) {
	double total_pts = 0.0;
	global::holdertotitle.for_each(id, l, [&total_pts, &l] (title_id_t t) {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			total_pts += points_in_adm_chain(to.associated_admin, l);
		}
	});
	return total_pts;
}

void add_to_war(INOUT(newwar) w, INOUT(flat_map<char_id_t, bool>) asked, admin_id_t a, char_id_t on_behalf_of, char_id_t war_against, wargoal primarygoal, IN(w_lock) l) {
	IN(auto) adm = get_object(a, l);
	const auto decider = get_war_decider(adm, l);
	if (decider != on_behalf_of && asked.count(decider) == 0) {
		asked[decider] = willraisetroops(decider, on_behalf_of, war_against);
	}
	if (decider == on_behalf_of || asked[decider]) {
		w.participants.emplace_back(a, make_wg_none());

		leigetoadmin.for_each(a, l, [&asked, decider, war_against, &w, &primarygoal, &l](admin_id_t v) {
			add_to_war(w, asked, v, decider, war_against, primarygoal, l);
		});
	}
}

void add_goal_to_war(INOUT(newwar) w, admin_id_t a, wargoal g, IN(w_lock) l) {
	for (INOUT(auto) p : w.participants) {
		if (p.adm == a && p.goal.type == wargoal::WARGOAL_NONE) {
			p.goal = g;
			return;
		}
	}
	w.participants.emplace_back(a, g);
}

double aggression_cost_for_no_cb(char_id_t id, IN(g_lock) l) {
	return with_udata(id, l, 0.0, [](IN(udata) d) {
		return d.p_peaceful - update_reputation(reputation::p_attack_nonagressive, reputation::p_attack_agressive, d.p_peaceful);
	});
	
}

bool in_war_against(char_id_t a, char_id_t b, IN(g_lock) l) {
	const auto pa = get_prime_admin(a, l);
	const auto pb = get_prime_admin(b, l);
	return wars_involved_in.for_each_breakable(pa, l, [pb, pa, &l](war_id_t id) {
		IN(auto) wp = get_object(id, l);
		if (std::find_if(wp.attacker.participants.begin(), wp.attacker.participants.end(), [pb](IN(participant) p) { return p.adm == pb; }) != wp.attacker.participants.end())
			return std::find_if(wp.defender.participants.begin(), wp.defender.participants.end(), [pa](IN(participant) p) { return p.adm == pa; }) != wp.defender.participants.end();
		if (std::find_if(wp.defender.participants.begin(), wp.defender.participants.end(), [pb](IN(participant) p) { return p.adm == pb; }) != wp.defender.participants.end())
			return std::find_if(wp.attacker.participants.begin(), wp.attacker.participants.end(), [pa](IN(participant) p) { return p.adm == pa; }) != wp.attacker.participants.end();
		return false;
	});
}

template<typename A, typename L>
bool _wargoal_possible(IN(wargoal) goal, char_id_t ch_for, IN(flat_set<admin_id_t, std::less<admin_id_t>, A>) against, IN(L) l) noexcept {
	switch (goal.type) {
	case wargoal::WARGOAL_CONQUEST:
	case wargoal::WARGOAL_DEFENSIVE:
		return true;
	case wargoal::WARGOAL_DEJURE:
	{
		cvector<prov_id_t> djprovs;
		owned_by_title(goal.data.title_for, djprovs, l);
		cvector<prov_id_t> tprovs;
		for (auto a : against) {
			controlled_by_admin(a, tprovs, l);
		}
		for (const auto p : djprovs) {
			if (std::find(tprovs.cbegin(), tprovs.cend(), p) != tprovs.cend())
				return true;
		}
		return false;
	}
	}
	return false;
}

bool wargoal_possible(IN(wargoal) goal, char_id_t ch_for, IN(flat_set<admin_id_t>) against, IN(w_lock) l) noexcept {
	return _wargoal_possible(goal, ch_for, against, l);
}
bool wargoal_possible(IN(wargoal) goal, char_id_t ch_for, IN(cflat_set<admin_id_t>) against, IN(g_lock) l) noexcept {
	return _wargoal_possible(goal, ch_for, against, l);
}

void list_participants(IN(newwar) w, INOUT(flat_set<admin_id_t>) p, IN(w_lock) l) noexcept {
	for (IN(auto) par : w.participants) {
		p.insert(par.adm);
	}
}

void list_participants(IN(newwar) w, INOUT(cflat_set<admin_id_t>) p, IN(g_lock) l) noexcept {
	for (IN(auto) par : w.participants) {
		p.insert(par.adm);
	}
}

void collapse_participants(IN(newwar) w, INOUT(flat_map<admin_id_t, std::vector<wargoal>>) cmap) {
	for (IN(auto) par : w.participants) {
		cmap[par.adm].push_back(par.goal);
	}
}

void list_war_participants_ch(IN(newwar) w, INOUT(cflat_set<char_id_t>) p, IN(g_lock) l) {
	for (IN(auto) par : w.participants) {
		p.insert(head_of_state(par.adm, l));
	}
}

void list_war_participants_ch(IN(newwar) w, INOUT(flat_set<char_id_t>) p, IN(w_lock) l) {
	for (IN(auto) par : w.participants) {
		p.insert(head_of_state(par.adm, l));
	}
}

double enact_wargoal_on_a(bool enforced, IN(wargoal) goal, double totalval, admin_id_t adm_for, IN(flat_set<admin_id_t>) against, INOUT(std::vector<prov_id_t>) ppool, INOUT(flat_set<char_id_t>) territory_losers, IN(w_lock) l) {
	switch (goal.type) {
	case wargoal::WARGOAL_NONE:
		return 0.0;
	case wargoal::WARGOAL_DEFENSIVE:
	{
		double amount = totalval;
		// double amount = std::min(DEF_VALUE_FOR(actor), totalval);
		// global::DeltaMoney(target, -amount * DEF_MONEY_MULTIPLIER, l);
		// global::DeltaMoney(actor, amount * DEF_MONEY_MULTIPLIER, l);
		// global::flag_ch_update(actor);
		// global::flag_ch_update(target);
		return amount;
	}
	case wargoal::WARGOAL_DEJURE: // fall through
	case wargoal::WARGOAL_CONQUEST:
	{
		std::vector<prov_id_t> provs;
		const double consumed = list_goal_provinces_a(provs, goal, adm_for, against, totalval, enforced, ppool, l);

		//do transfer
		//std::vector<char_id> ptupdate;
		//ptupdate.reserve(16);

		//title_id targetpt = global::GetPrimtitle(actor);
		//title_id sourcept = global::GetPrimtitle(target);

		//title_id leige;
		//int type;
		//global::GetTQdata(targetpt, type, leige);
		//const title_id ltitle = (type == 4) ? leige : targetpt;

		
		for (auto prov : provs)
			transfer_prov_a(prov, adm_for, territory_losers, l);
		

		//if (targetpt == 0)
		//	ptupdate.push_back(target);
		//for (const auto ch : ptupdate)
		//	global::UpdatePrimT(ch, l);

		//global::update_capital(, l);
		//global::UpdateCapital(sourcept, fake_lock());

		global::setFlag(FLG_BORDER_UPDATE);
		if (global::mapmode == MAP_MODE_POL || global::mapmode == MAP_MODE_VAS) {
			global::setFlag(FLG_MAP_UPDATE);
		}

		return consumed;
	}
	}
	return 0.0;
}

template<typename T, typename L>
double _wargoal_value(IN(wargoal) goal, admin_id_t adm_for, IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) against, IN(L) l) {
	switch (goal.type) {
	case wargoal::WARGOAL_NONE: return 0.0; // DEF_VALUE_FOR(actor);
	case wargoal::WARGOAL_DEFENSIVE: return 0.0; // DEF_VALUE_FOR(actor);
	case wargoal::WARGOAL_CONQUEST:
	{
		std::vector<prov_id_t, typename allocator_t<L>::type<prov_id_t>> provs;
		for (auto a : against) {
			controlled_by_admin(a, provs, l);
		}
		double totalv = 0.0;
		for (size_t i = provs.size() - 1; i != SIZE_MAX; --i) {
			totalv += get_object(provs[i]).tax * TAX_MULTIPLIER;
		}
		return totalv;
	}
	case wargoal::WARGOAL_DEJURE:
	{
		std::vector<prov_id_t, typename allocator_t<L>::type<prov_id_t>> provs;
		for (auto a : against) {
			controlled_by_admin(a, provs, l);
		}
		double totalv = 0.0;
		for (auto  p : provs) {
			if(is_dj(p, goal.data.title_for, l))
				totalv += get_object(p).tax * TAX_MULTIPLIER;
		}
		return totalv;
	}
	}
	return 0.0;
}

double wargoal_value(IN(wargoal) goal, admin_id_t adm_for, IN(flat_set<admin_id_t>) against, IN(w_lock) l) {
	return _wargoal_value(goal, adm_for, against, l);
}
double wargoal_value(IN(wargoal) goal, admin_id_t adm_for, IN(cflat_set<admin_id_t>) against, IN(g_lock) l) {
	return _wargoal_value(goal, adm_for, against, l);
}

template <typename T, typename L>
double warscore_for_goals(IN(newwar) wr, IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) against, IN(L) l) {
	double sum = 0.0;
	for (IN(auto) p : wr.participants) {
		sum += wargoal_value(p.goal, p.adm, against, l);
	}
	return sum;
}

bool wargoal_allows_partial(IN(wargoal) goal) {
	switch (goal.type) {
	case wargoal::WARGOAL_NONE: return true;
	case wargoal::WARGOAL_DEFENSIVE: return true;
	case wargoal::WARGOAL_CONQUEST: return true;
	case wargoal::WARGOAL_DEJURE: return true;
	}
	return false;
}

bool wargoal_has_prov_list(IN(wargoal) goal) {
	switch (goal.type) {
	case wargoal::WARGOAL_NONE: return false;
	case wargoal::WARGOAL_DEFENSIVE: return false;
	case wargoal::WARGOAL_CONQUEST: return true;
	case wargoal::WARGOAL_DEJURE: return true;
	}
	return false;
}

template<typename T, typename L>
bool _can_enforce_goal(admin_id_t goal_for, IN(wargoal) goal, IN(newwar) wr, IN(newwar) otherside, IN(std::vector<prov_id_t, T>) noma, IN(std::vector<prov_id_t, T>) nomb, IN(L) l) {
	switch (goal.type) {
	case wargoal::WARGOAL_NONE: return true;
	case wargoal::WARGOAL_DEFENSIVE: return true;
	case wargoal::WARGOAL_CONQUEST: return true;
	case wargoal::WARGOAL_DEJURE: return true;
	}
	return false;
}

bool can_enforce_goal(admin_id_t goal_for, IN(wargoal) goal, IN(newwar) wr, IN(newwar) otherside, IN(std::vector<prov_id_t>) noma, IN(std::vector<prov_id_t>) nomb, IN(w_lock) l) {
	return _can_enforce_goal(goal_for, goal, wr, otherside, noma, nomb, l);
}
bool can_enforce_goal(admin_id_t goal_for, IN(wargoal) goal, IN(newwar) wr, IN(newwar) otherside, IN(cvector<prov_id_t>) noma, IN(cvector<prov_id_t>) nomb, IN(g_lock) l) {
	return _can_enforce_goal(goal_for, goal, wr, otherside, noma, nomb, l);
}

template<typename T, typename U, typename L>
double _list_goal_provinces_a(INOUT(std::vector<prov_id_t, T>) lst, IN(wargoal) goal, admin_id_t adm_for, IN(flat_set<admin_id_t, std::less<admin_id_t>, U>) against, double totalval, bool enforcing, INOUT(std::vector<prov_id_t, T>) ppool, IN(L) l) {
	sort_by_proximity_reverse_a(ppool, adm_for, l);
	double accvalue = 0.0;
	for (size_t i = ppool.size()-1; i != SIZE_MAX && accvalue <= totalval; --i) {
		if (goal.type == wargoal::WARGOAL_CONQUEST || is_dj(ppool[i], goal.data.title_for, l)) {
			double pval = get_object(ppool[i]).tax * TAX_MULTIPLIER;
			if (accvalue + pval <= totalval) {
				lst.push_back(ppool[i]);
				accvalue += pval;
				ppool[i] = ppool.back();
				ppool.pop_back();
			}
		} 
	}
	return accvalue;
}

double list_goal_provinces_a(INOUT(std::vector<prov_id_t>) lst, IN(wargoal) goal, admin_id_t adm_for, IN(flat_set<admin_id_t>) against, double totalval, bool enforcing, INOUT(std::vector<prov_id_t>) ppool, IN(w_lock) l) {
	return _list_goal_provinces_a(lst, goal, adm_for, against, totalval, enforcing, ppool, l);
}
double list_goal_provinces_a(INOUT(cvector<prov_id_t>) lst, IN(wargoal) goal, admin_id_t adm_for, IN(cflat_set<admin_id_t>) against, double totalval, bool enforcing, INOUT(cvector<prov_id_t>) ppool, IN(g_lock) l) {
	return _list_goal_provinces_a(lst, goal, adm_for, against, totalval, enforcing, ppool, l);
}

void display_goal_name(IN(std::shared_ptr<uiElement>) parent, int x, int &y, IN(wargoal) goal, IN(g_lock) l) {
	switch (goal.type) {
	case wargoal::WARGOAL_NONE:
	{
		return;
	}
	case wargoal::WARGOAL_DEFENSIVE:
	{
		get_linear_ui(TX_L_GOAL_DEF, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 2;
		return;
	}
	case wargoal::WARGOAL_CONQUEST:
	{
		get_linear_ui(TX_L_GOAL_CONQ, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 2;
		return;
	}
	case wargoal::WARGOAL_DEJURE:
	{
		size_t param = goal.data.title_for.value;
		get_linear_ui(TX_L_GOAL_DEJ, &param, 1, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 2;
		return;
	}
	}
	return;
}

double display_goal(IN(std::shared_ptr<uiElement>) parent, int x, int &y, IN(wargoal) goal, char_id_t actor, double totalval, IN(g_lock) l, bool enforcing) {
	switch (goal.type) {
	case wargoal::WARGOAL_NONE:
	{
		return 0.0;
	}
	case wargoal::WARGOAL_DEFENSIVE:
	{
		double amount = std::min(DEF_VALUE_FOR(actor), totalval) * DEF_MONEY_MULTIPLIER;
		size_t params[] = {dumb_cast<size_t>(static_cast<__int64>(amount)), actor.value};
		get_linear_ui(TX_GOAL_DEF, params, 2, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 2;
		return amount;
	}
	case wargoal::WARGOAL_CONQUEST:
	{
		size_t param = actor.value;
		get_linear_ui(TX_GOAL_CONQ, &param, 1, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 2;
		return 0.0;
	}
	case wargoal::WARGOAL_DEJURE:
	{
		size_t params[] = {goal.data.title_for.value, actor.value};
		get_linear_ui(TX_GOAL_DEJ, params, 2, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 2;
		return 0.0;
	}
	}
	return 0.0;
}

void adjust_alloc_to(size_t front_index, INOUT(war_pair) w, bool attacker, float target, IN(w_lock) l) {
	/*
	float freealloc = 1.0f;

	INOUT(auto) f = attacker ? w.fronts[front_index].first : w.fronts[front_index].second;

	bool prevlocked = f.locked;
	f.locked = true;

	float freeablealloc = 0.0f;

	if (attacker) {
		for (IN(auto) fpr : w.fronts) {
			freealloc -= fpr.first.allocation;
		}

		if (target - f.allocation < freealloc) {
			f.allocation = target;
			return;
		}

		for (IN(auto) fpr : w.fronts) {
			if (!fpr.first.locked)
				freeablealloc += fpr.first.allocation;
		}

		if (target < freeablealloc + f.allocation + freealloc) {
			const float multiplier = 1.0f - (((target - f.allocation) - freealloc) / freeablealloc);
			for (INOUT(auto) fpr : w.fronts) {
				if (!fpr.first.locked)
					fpr.first.allocation *= multiplier;
			}
			f.allocation = target;
			f.locked = prevlocked;
			return;
		}
		for (INOUT(auto) fpr : w.fronts) {
			if (!fpr.first.locked)
				fpr.first.allocation = 0.0f;
		}
	} else {
		for (IN(auto) fpr : w.fronts) {
			freealloc -= fpr.second.allocation;
		}

		if (target - f.allocation < freealloc) {
			f.allocation = target;
			return;
		}

		for (IN(auto) fpr : w.fronts) {
			if (!fpr.second.locked)
				freeablealloc += fpr.second.allocation;
		}

		if (target < freeablealloc + f.allocation + freealloc) {
			const float multiplier = 1.0f - (((target - f.allocation) - freealloc) / freeablealloc);
			for (INOUT(auto) fpr : w.fronts) {
				if (!fpr.second.locked)
					fpr.second.allocation *= multiplier;
			}
			f.allocation = target;
			f.locked = prevlocked;
			return;
		}
		for (INOUT(auto) fpr : w.fronts) {
			if (!fpr.second.locked)
				fpr.second.allocation = 0.0f;
		}
	}
	f.allocation += freealloc + freeablealloc;
	f.locked = prevlocked; */
}

bool atwar(char_id_t ch, IN(g_lock) l) noexcept {
	return global::holdertotitle.for_each_breakable(ch, l, [&l](title_id_t t) {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			return wars_involved_in.count(to.associated_admin, l) != 0;
		}
		return false;
	});
}


double points_in_war(admin_id_t adm, IN(g_lock) l) noexcept {
	if (raised_troops.contains(adm, l)) {
		IN(auto) t = raised_troops.get(adm, l);
		return troops_to_points(t.numlevy, t.numstanding);
	}
	return 0.0;
}

double totalpointsinwar(IN(cflat_set<admin_id_t>) participants, IN(g_lock) l) noexcept {
	double sum = 0.0;
	for (const auto a : participants) {
		IN(auto) t = raised_troops.get(a, l);
		sum += t.numlevy * 1.0;
		sum += t.numstanding * 2.0;
	}
	return sum;
}

double totalpointsinwar(IN(flat_set<admin_id_t>) participants, IN(w_lock) l) noexcept {
	double sum = 0.0;
	for (const auto a : participants) {
		IN(auto) t = raised_troops.get(a, l);
		sum += t.numlevy * 1.0;
		sum += t.numstanding * 2.0;
	}
	return sum;
}


unsigned int totaltroops(IN(flat_set<admin_id_t>) participants, IN(w_lock) l) noexcept {
	unsigned int sum = 0;
	for (const auto a : participants) {
		IN(auto) t = raised_troops.get(a, l);
		sum += t.numlevy;
		sum += t.numstanding;
	}
	return sum;
}
unsigned int totaltroops(IN(cflat_set<admin_id_t>) participants, IN(g_lock) l) noexcept {
	unsigned int sum = 0;
	for (const auto a : participants) {
		IN(auto) t = raised_troops.get(a, l);
		sum += t.numlevy;
		sum += t.numstanding;
	}
	return sum;
}

template<typename L>
unsigned int _totaltroops(war_id_t war_for, bool agressor, IN(L) l) noexcept {
	IN(auto) wp = get_object(war_for, l);

	flat_set<admin_id_t, std::less<admin_id_t>, typename allocator_t<L>::boost_type<admin_id_t>> participants;
	list_participants(agressor ? wp.attacker : wp.defender, participants, l);

	unsigned int sum = 0;
	for (const auto a : participants) {
		IN(auto) t = raised_troops.get(a, l);
		sum += t.numlevy;
		sum += t.numstanding;
	}
	return sum;
}

unsigned int totaltroops(war_id_t war_for, bool agressor, IN(g_lock) l) noexcept {
	return _totaltroops(war_for, agressor, l);
}
unsigned int totaltroops(war_id_t war_for, bool agressor, IN(w_lock) l) noexcept {
	return _totaltroops(war_for, agressor, l);
}

void reduce_troops(IN(flat_set<admin_id_t>) participants, double percent_loss, IN(w_lock) l) noexcept {
	const double mul = 1.0 - (percent_loss < 1.0 ? percent_loss : 1.0);
	for (const auto a : participants) {
		INOUT(auto) t = raised_troops.get(a, l);
		t.numlevy = static_cast<decltype(t.numlevy)>(mul * t.numlevy);
		t.numstanding = static_cast<decltype(t.numstanding)>(mul * t.numstanding);
	}
}

void stand_down_troop(admin_id_t adm, IN(w_lock) l) {
	if(raised_troops.contains(adm , l))
		raised_troops.erase(adm, l);
}

void end_war(war_id_t wid, IN(w_lock) l) {
	INOUT(war_pair) w = get_object(wid, l);

	for (IN(auto) p : w.attacker.participants) {
		wars_involved_in.erase(p.adm, wid, l);
		if (wars_involved_in.count(p.adm, l) == 0) {
			stand_down_troop(p.adm, l);
		}
	}
	for (IN(auto) p : w.defender.participants) {
		wars_involved_in.erase(p.adm, wid, l);
		if (wars_involved_in.count(p.adm, l) == 0) {
			stand_down_troop(p.adm, l);
		}
	}

	occupation_info.erase_if(l, [wid](IN(std::pair<prov_id_t, seiging_info>) pr) { return pr.second.in_war == wid; });

	//clear battles and seiges
	auto cr = global::schedule.begin();
	const auto ubound = global::schedule.upper_bound(global::currentday + 30);
	for (; cr != ubound; ++cr) {
		if (cr->second->type == S_TYPE_SEIGE) {
			IN_P(auto) se = static_cast<s_seige*>(cr->second.get());
			if (se->war_for == wid) {
				se->type = S_TYPE_ERASED;
			}
		} else if (cr->second->type == S_TYPE_BATTLE) {
			IN_P(auto) se = static_cast<s_battle*>(cr->second.get());
			if (se->war_for == wid) {
				se->type = S_TYPE_ERASED;
			}
		}
	}

	war_pool.free(wid.value, l);
}

bool touchesregions( prov_id prov, INOUT(std::vector<prov_id>) lst) {
	for (const auto prv : lst) {
		if (global::adjacent(prv, prov))
			return true;
	}
	return false;
}


inline void listedges(INOUT(flat_set<edge>) edges,  IN(std::vector<prov_id>) sidea, IN(std::vector<prov_id>) sideb) {
	for (const auto prv : sidea) {
		for (const auto prvb : sideb) {
			if(global::adjacent(prv,prvb))
				edges.emplace(prv, prvb);
		}
	}
}

bool touchesnotinregions( int prov, IN(std::vector<prov_id>) sourceregions) {
	for (auto pr = global::province_connections.get_row(prov); pr.first != pr.second; ++pr.first) {
		if (std::find(sourceregions.cbegin(), sourceregions.cend(), *pr.first) == sourceregions.cend()) {
			return true;
		}
	}
	return false;
}

template<typename T>
double totaltroopcost(IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) p, IN(g_lock) l) {
	double troopscost = 0.0;
	for (const auto a : p) {
		IN(auto) t = raised_troops.get(a, l);
		troopscost += troopCost(t.numlevy, t.numstanding);
	}
	return troopscost;
}

template<typename T, typename U, typename L>
double month_prediction(war_id_t wid, bool attacker, unsigned int month, IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) all_attackers, IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) all_defenders,
	IN(std::vector<prov_id_t, U>) acontrolled, IN(std::vector<prov_id_t, U>) bcontrolled,  IN(L) l) {
	double numaprovs = 0.0;
	double numbprovs = 0.0;
	double totalatax = 0.0;
	double totalbtax = 0.0;

	for (const auto p : acontrolled) {
		if(!occupation_info.contains(p, l)) {
			totalatax += get_object(p).tax * 12.0;
			++numaprovs;
		} else if (occupation_info.get(p, l).in_war == wid) {
			totalbtax += get_object(p).tax * 12.0;
			++numbprovs;
		}
	}

	for (const auto p : bcontrolled) {
		if (!occupation_info.contains(p, l)) {
			totalbtax += get_object(p).tax * 12.0;
			++numbprovs;
		} else if (occupation_info.get(p, l).in_war == wid) {
			totalatax += get_object(p).tax * 12.0;
			++numaprovs;
		}
	}
	
	IN(auto) wp = get_object(wid, l);
	
	const auto main_attacker = attacker ? head_of_state(wp.attacker.primary, l) : head_of_state(wp.defender.primary, l);
	const auto main_defender = !attacker ? head_of_state(wp.attacker.primary, l) : head_of_state(wp.defender.primary, l);

	double monthlyacost = global::project_income(main_attacker, l) - totaltroopcost(all_attackers, l); //monthly cost of war
	double monthlybcost = global::project_income(main_defender, l) - totaltroopcost(all_defenders, l); //monthly cost of war

	double captureprojectiona = 0.0;
	double captureprojectionb = 0.0;

	double aallocsum = 1.0;// 0.0;
	double ballocsum = 1.0; // 0.0;

//	for (IN(auto) fr : nw.fronts) {
//		aallocsum += fr.first.allocation;
//		ballocsum += fr.second.allocation;
//	}

	const double ptsa = totalpointsinwar(all_attackers, l) * aallocsum;
	const double ptsb = totalpointsinwar(all_defenders, l) * ballocsum;
	
	const double frnta = pointstoprob(ptsa);
	const double frntb = pointstoprob(ptsb);
	//frnta *= frnta;
	//frntb *= frntb;
	const double diff = (frnta + frntb) > 0.0 ? ((frnta + frntb) * 0.5 - frntb) / (frnta + frntb) : 0.0;


	for (size_t indx = 0; indx < month; ++indx) {
		if (monthlyacost * (indx + 1) < global::get_wealth(main_attacker, l) && monthlybcost * (indx + 1) < global::get_wealth(main_defender, l)) {
			if (diff > 0.0) {
				if (numbprovs > 0.0) {
					captureprojectiona += totalbtax / numbprovs * diff;
					totalatax += totalbtax / numbprovs * diff;
					numbprovs = std::max(0.0, numbprovs - diff);
					numaprovs += diff;
					captureprojectiona = std::min(captureprojectiona, totalbtax);
				}
			} else if(numaprovs > 0.0) {
				captureprojectionb += totalatax / numaprovs * -diff;
				totalbtax += totalatax / numaprovs * -diff;
				numaprovs = std::max(0.0, numaprovs + diff);
				numbprovs += -diff;
				captureprojectionb = std::min(captureprojectionb, totalatax);
			}
		} else if (monthlyacost * (indx + 1) < global::get_wealth(main_attacker, l)) {
			if (numbprovs > 0.0) {
				captureprojectiona += totalbtax / numbprovs;
				totalatax += totalbtax / numbprovs;
				numbprovs = std::max(0.0, numbprovs - 1.0);
				++numaprovs;
				captureprojectiona = std::min(captureprojectiona, totalbtax);
			}
		} else if (monthlybcost * (indx + 1) < global::get_wealth(main_defender, l)) {
			if (numaprovs > 0.0) {
				captureprojectionb += totalatax / numaprovs;
				totalbtax += totalatax / numaprovs;
				numaprovs = std::max(0.0, numaprovs - 1.0);
				++numbprovs;
				captureprojectionb = std::min(captureprojectionb, totalatax);
			}
		}
	}

	return captureprojectiona - captureprojectionb;
}

template<typename A, typename B, typename L>
bool _can_enforce(IN(newwar) other, IN(flat_set<admin_id_t, std::less<admin_id_t>, A>) apart, IN(flat_set<admin_id_t, std::less<admin_id_t>, A>) bpart, IN(std::vector<prov_id_t, B>) nom_b_controlled, IN(L) l) noexcept {
	//unpaid?
	if (other.paid == false) {
		return true;
	}
	//defense too small
	if (totaltroops(bpart, l) * 16 < totaltroops(apart, l)) {
		return true;
	}

	//unoccupied land?
	for (const auto p : nom_b_controlled) {
		if (!occupation_info.contains(p, l)) {
			return false;
		}
	}
	return true;
}

bool can_enforce(IN(newwar) other, IN(cflat_set<admin_id_t>) apart, IN(cflat_set<admin_id_t>) bpart, IN(cvector<prov_id_t>) nom_b_controlled, IN(g_lock) l) noexcept {
	return _can_enforce(other, apart, bpart, nom_b_controlled, l);
}
bool can_enforce(IN(newwar) other, IN(flat_set<admin_id_t>) apart, IN(flat_set<admin_id_t>) bpart, IN(std::vector<prov_id_t>) nom_b_controlled, IN(w_lock) l) noexcept {
	return _can_enforce(other, apart, bpart, nom_b_controlled, l);
}

template<typename T, typename L>
double _war_prediction_value(war_id_t wid, bool attacker, IN(std::vector<prov_id_t, T>) selfcontrolled, IN(std::vector<prov_id_t, T>) tcontrolled, IN(L) l) {
	flat_set<admin_id_t, std::less<admin_id_t>, typename allocator_t<L>::boost_type<admin_id_t>> all_attackers;
	flat_set<admin_id_t, std::less<admin_id_t>, typename allocator_t<L>::boost_type<admin_id_t>> all_defenders;

	IN(auto) wp = get_object(wid, l);
	IN(auto) sidea = attacker ? wp.attacker : wp.defender;
	IN(auto) sideb = !attacker ? wp.attacker : wp.defender;

	list_participants(sidea, all_attackers, l);
	list_participants(sideb, all_defenders, l);

	auto pred = month_prediction(wid, attacker, 12, all_attackers, all_defenders, selfcontrolled, tcontrolled, l);

	if (pred < 0.0) {
		if (can_enforce(sidea, all_attackers, all_defenders, tcontrolled, l)) {
			pred = 0.0;
		}
	} else if (pred > 0.0) {
		if (can_enforce(sideb, all_defenders, all_attackers, selfcontrolled, l)) {
			pred = 0.0;
		}
	}
	return pred;
}

double war_prediction_value(war_id_t wid, bool attacker, IN(cvector<prov_id_t>) selfcontrolled, IN(cvector<prov_id_t>) tcontrolled, IN(g_lock) l) {
	return _war_prediction_value(wid, attacker, selfcontrolled, tcontrolled, l);
}
double war_prediction_value(war_id_t wid, bool attacker, IN(std::vector<prov_id_t>) selfcontrolled, IN(std::vector<prov_id_t>) tcontrolled, IN(w_lock) l) {
	return _war_prediction_value(wid, attacker, selfcontrolled, tcontrolled, l);
}

double war_estimation(char_id_t from, char_id_t to, IN(g_lock) l) {
	cvector<prov_id_t> provs;

	global::get_nom_controlled(to, provs, l);
	double total_to_tax = 0.0;
	for (const auto p : provs) {
		total_to_tax += get_object(p).tax;
	}
	provs.clear();

	global::get_nom_controlled(from, provs, l);
	double total_from_tax = 0.0;
	for (const auto p : provs) {
		total_from_tax += get_object(p).tax;
	}

	const auto from_mu = mu_estimate(from, l);
	const auto to_mu = mu_estimate(to, l) + def_mu_estimate(to, l) + def_against_mu_estimate(to, from, l);

	const double prob_success = static_cast<double>(from_mu / (from_mu + to_mu));
	return std::min(1.0, (total_to_tax / total_from_tax) * prob_success)  - (1.0 - prob_success);
}

template<typename T, typename L>
double _war_base_tax_value(war_id_t wid, IN(std::vector<prov_id_t, T>) selfcontrolled, IN(std::vector<prov_id_t, T>) tcontrolled, IN(L) l) {
	double warscore = 0.0;
	for (const auto p : tcontrolled) {
		if (occupation_info.contains(p, l) && occupation_info.get(p, l).in_war == wid) {
			warscore += TAX_MULTIPLIER * get_object(p).tax;
		}
	}
	for (const auto p : selfcontrolled) {
		if (occupation_info.contains(p, l) && occupation_info.get(p, l).in_war == wid) {
			warscore -= TAX_MULTIPLIER * get_object(p).tax;
		}
	}
	return warscore;
}

double war_base_tax_value(war_id_t wid, IN(cvector<prov_id_t>) selfcontrolled, IN(cvector<prov_id_t>) tcontrolled, IN(g_lock) l) {
	return _war_base_tax_value(wid, selfcontrolled, tcontrolled, l);
}

double war_base_tax_value(war_id_t wid, IN(std::vector<prov_id_t>) selfcontrolled, IN(std::vector<prov_id_t>) tcontrolled, IN(w_lock) l) {
	return _war_base_tax_value(wid, selfcontrolled, tcontrolled, l);
}

double slow_war_base_value(war_id_t wid, IN(g_lock) l) {
	IN(auto) wp = get_object(wid, l);

	IN(auto) aside = wp.attacker;
	IN(auto) oside = wp.defender;

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

	return war_base_tax_value(wid, acontrolled, bcontrolled, l);
}

double slow_war_prediction(war_id_t wid, IN(g_lock) l) {
	IN(auto) wp = get_object(wid, l);

	IN(auto) aside = wp.attacker;
	IN(auto) oside = wp.defender;

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

	return war_prediction_value(wid, true, acontrolled, bcontrolled, l);
}

#define LEVY_COST 0.001
#define STANDING_COST 0.002


double total_troop_cost(IN(flat_set<admin_id_t>) participants, unsigned int currentday, IN(g_lock) l) noexcept {
	double sum = 0.0;
	for (const auto a : participants) {
		IN(auto) t = raised_troops.get(a, l);

		if (t.payafter + 30 < currentday)
			sum += troopCost(t.numlevy, t.numstanding);
		else if (t.payafter < currentday)
			sum += (currentday - t.payafter) * troopCost(t.numlevy, t.numstanding) / 30.0;
	}
	return sum;
}

template<typename T, typename L>
void _provsbygoal(INOUT(std::vector<prov_id_t, T>) provs, char_id_t c, char_id_t target, IN(wargoal) g,  IN(L) l) noexcept {
	if (g.type == wargoal::WARGOAL_CONQUEST) {
		global::get_nom_controlled(target, provs, l);
	} else if (g.type == wargoal::WARGOAL_DEJURE) {
		std::vector<prov_id_t, T> tmp;
		global::get_nom_controlled(target, tmp, l);
		std::vector<prov_id_t, T> dj;
		global::get_dj_controlled(c, dj, l);
		for (const auto p : tmp) {
			if (std::find(dj.cbegin(), dj.cend(), p) != dj.cend())
				provs.emplace_back(p);
		}
	}
}

void provsbygoal(INOUT(cvector<prov_id_t>) provs, char_id_t c, char_id_t target, IN(wargoal) g, IN(g_lock) l) noexcept {
	_provsbygoal(provs, c, target, g, l);
}
void provsbygoal(INOUT(std::vector<prov_id_t>) provs, char_id_t c, char_id_t target, IN(wargoal) g, IN(w_lock) l) noexcept {
	_provsbygoal(provs, c, target, g, l);
}

void pay_for_war(INOUT(newwar) wr, unsigned int currentday,  IN(w_lock) l) noexcept {
	// double paybyowner = 0.0;
	// flat_set<const newtroop*> paid;

	flat_set<admin_id_t> p;
	list_participants(wr, p, l);
	double mcost = total_troop_cost(p, currentday, l);

	if (global::get_wealth(head_of_state(wr.primary, l), l) < mcost) {
		wr.paid = false;
	} else {
		wr.paid = true;
		add_expense(head_of_state(wr.primary, l), EXPENSE_WAR, 1, mcost, l);
	}
}

double troopCost(size_t l, size_t s) noexcept {
	return  l * LEVY_COST + s * STANDING_COST;
}

float troop_cost_est(float points) noexcept {
	return  (9.0f * points / 11.0f) * static_cast<float>(LEVY_COST) + (points / 11.0f) * static_cast<float>(STANDING_COST);
}

void addseiges(war_id_t wid, admin_id_t primary, double sprob, IN(std::vector<prov_id_t>) othersideprv, IN(std::vector<prov_id_t>) ownprv, IN(w_lock) l) {
	//rank provinces
	std::vector<prov_id_t> provs;
	for (const auto e : othersideprv) {
		if (!occupation_info.contains(e, l)) {
			provs.emplace_back(e);
		}
	}

	std::vector<prov_id_t> ownseiged;
	for (const auto e : ownprv) {
		if (occupation_info.contains(e, l)) {
			ownseiged.emplace_back(e);
		}
	}

	const auto cap = get_object(primary, l).capital;
	sort_by_proximity_reverse(provs, cap, l);
	sort_by_proximity_reverse(ownseiged, cap, l);

	while (global::randdouble() <= sprob && provs.size() != 0) {
		if (ownseiged.size() > 0) {
			INOUT(auto) info = occupation_info.get(ownseiged.back(), l);
			if (info.since == 0) {
				info.since = global::currentday;
				info.lifting = true;
			} else {
				occupation_info.erase(ownseiged.back(), l);
			}
			ownseiged.pop_back();
		} else {
			occupation_info.emplace(provs.back(), l, wid, global::currentday, false);
		}

		if (global::mapmode == 3 || global::mapmode == 4) {
			global::setFlag(FLG_MAP_UPDATE);
		}
		sprob *= .5;
	}
}

double inline seigechance(unsigned int duration) noexcept {
	const double constval = duration / 30.0 - 4.0;
	//concurrency::fast_math::pow(1.0f, 2.0f);
	return 0.4 + (constval / (2.0 * (1.0 + abs(constval))));
}


bool prov_belongs_to_attacker(war_id_t wid, prov_id_t prov, IN(g_lock) l) {
	small_vector<admin_id_t, 5> alist;
	global::enum_control_by_prov(prov, l, [&alist](IN(controlrecord) r) {
		alist.push_back(r.ad_controller);
	});
	IN(auto) attacker = get_object(wid, l).attacker;
	const auto lbegin = alist.begin();
	const auto lend = alist.end();

	for (IN(auto) pa : attacker.participants) {
		if (std::find(lbegin, lend, pa.adm) != lend)
			return true;
	}
	return false;
}

void update_seiges(IN(w_lock) l) {
	occupation_info.for_all(l, [&l](prov_id_t prov, INOUT(seiging_info) i) {
		if (i.since != 0) {
			IN(auto) wp = get_object(i.in_war, l);
			if (global::randdouble() < seigechance(global::currentday - i.since)) {
				//bool for_attacker = prov_belongs_to_attacker(i.in_war, prov, l);
				// if ((for_attacker && wp.attacker.paid) || (!for_attacker && wp.defender.paid)) {
					const unsigned int sday = global::currentday + abs(global::randint()) % 28;
					global::schedule.emplace(sday, new s_seige(i.in_war, prov, sday - i.since));
				// }
			}
		}
	});
}

/*void update_seiges(war_id_t war_for, bool for_attacker, INOUT(std::vector<std::pair<prov_id_t, unsigned int>>) seiging) {
	const size_t sz = seiging.size();
	for (size_t indx = 0; indx != sz; ++indx) {
		if (global::randdouble() < seigechance(global::currentday - seiging[indx].second)) {
			const unsigned int sday = global::currentday + abs(global::randint()) % 28;
			global::schedule.emplace(sday, new s_seige(war_for, for_attacker, seiging[indx].first, sday - seiging[indx].second));
		}
	}
} */

bool makecoastal(IN(std::vector<prov_id>) rg) {
	bool coastal = false;
	for (const auto p : rg) {
		if ((detail::provinces[p].pflags & PROV_FLAG_COASTAL) != 0) {
			return true;
		}
	}
	return false;
}


double pointstoprob(const double in) noexcept {
	return sqrt(in) * in;
}

template<typename T, typename U, typename L>
bool can_demand_all(IN(newwar) wa, IN(newwar) wb, IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) aprt, IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) bprt, IN(std::vector<prov_id_t, U>) noma, IN(std::vector<prov_id_t, U>) nomb, IN(L) l) {
	if (!can_enforce(wb, aprt, bprt, nomb, l)) return false;

	for (IN(auto) gp : wa.participants) {
		if (!can_enforce_goal(gp.adm, gp.goal, wa, wb, noma, nomb, l))
			return false;
	}
	return true;
}

bool unoccupied_provinces(IN(std::vector<prov_id_t>) pset, IN(g_lock) l) {
	for (auto p : pset) {
		if (!occupation_info.contains(p, l)) {
			return true;
		} else {
			IN(auto) inf = occupation_info.get(p, l);
			if (inf.since != 0 && !inf.lifting)
				return true;
		}
	}
	return false;
}


void updatewar(INOUT(war_pair) wp, INOUT(w_lock) wlk) {

	std::vector<prov_id_t> acontrolled;
	std::vector<prov_id_t> bcontrolled;

	flat_set<admin_id_t> all_attackers;
	flat_set<admin_id_t> all_defenders;
	list_participants(wp.attacker, all_attackers, wlk);
	list_participants(wp.defender, all_defenders, wlk);

	for (const auto a : all_attackers)
		controlled_by_admin(a, acontrolled, wlk);
	for (const auto a : all_defenders)
		controlled_by_admin(a, bcontrolled, wlk);

	/*for (auto it = wp.attacker.controlled.cbegin(); it != wp.attacker.controlled.cend(); ) {
		if (std::find(bcontrolled.cbegin(), bcontrolled.cend(), *it) == bcontrolled.cend()) {
			it = wp.attacker.controlled.erase(it);
		} else {
			++it;
		}
	}

	for (auto it = wp.defender.controlled.cbegin(); it != wp.defender.controlled.cend(); ) {
		if (std::find(acontrolled.cbegin(), acontrolled.cend(), *it) == acontrolled.cend()) {
			it = wp.defender.controlled.erase(it);
		} else {
			++it;
		}
	}*/

	const auto this_id = get_id(wp, wlk);
	const double warbase = war_base_tax_value(this_id, acontrolled, bcontrolled, wlk);

	
	if (acontrolled.size() == 0) {
		peace_deal deal;
		deal.init(this_id, wp.defender, wp.attacker, -warbase, wlk);
		deal.generate_demand_to_value(wp.defender, wp.attacker, wlk);
		deal.make_demand(wlk);
		return;
	} else if (bcontrolled.size() == 0) {
		peace_deal deal;
		deal.init(this_id, wp.attacker, wp.defender, warbase, wlk);
		deal.generate_demand_to_value(wp.attacker, wp.defender, wlk);
		deal.make_demand(wlk);
		return;
	}

	//ai -- decide

	const auto a_points = totalpointsinwar(all_attackers, wlk);
	const double a_troops = static_cast<double>(totaltroops(all_attackers, wlk));

	const auto b_points = totalpointsinwar(all_defenders, wlk);
	const double b_troops = static_cast<double>(totaltroops(all_defenders, wlk));

	const auto a_primary = head_of_state(admin_id_t(wp.attacker.primary), wlk);
	const auto b_primary = head_of_state(admin_id_t(wp.defender.primary), wlk);

	const double warprediction = war_prediction_value(this_id, true, acontrolled, bcontrolled, wlk);
	const bool ahasmore = a_points > b_points;

	if (a_primary != global::playerid && warbase >= 0) {
		const auto target = warscore_for_goals(wp.attacker, all_defenders, wlk);
		if ((can_demand_all(wp.attacker, wp.defender, all_attackers, all_defenders, acontrolled, bcontrolled, wlk) && target <= warbase) || !unoccupied_provinces(bcontrolled, wlk)) {
			peace_deal deal;
			deal.init(this_id, wp.attacker, wp.defender, warbase, wlk);
			deal.generate_demand_to_value(wp.attacker, wp.defender, wlk);
			deal.make_demand(wlk);
			return;
		} else if ((!ahasmore || !wp.attacker.paid || target <= warbase + warprediction) && warbase + warprediction > 0) {
			peace_deal deal;
			deal.init(this_id, wp.attacker, wp.defender, warbase + warprediction, wlk);
			deal.generate_offer_to_value(wp.attacker, wp.defender, wlk);
			if (deal.make_offer(wlk))
				return;
		}
	}
	if (b_primary != global::playerid && warbase <= 0) {
		const auto target = warscore_for_goals(wp.defender, all_attackers, wlk);
		if ((can_demand_all(wp.defender, wp.attacker, all_defenders, all_attackers, bcontrolled, acontrolled, wlk) && target <= -warbase) || !unoccupied_provinces(acontrolled,wlk)) {
			peace_deal deal;
			deal.init(this_id, wp.defender, wp.attacker, -warbase, wlk);
			deal.generate_demand_to_value(wp.defender, wp.attacker, wlk);
			deal.make_demand(wlk);
			return;
		} else if ((ahasmore || !wp.defender.paid || target <= -(warbase + warprediction)) && warbase + warprediction < 0) {
			peace_deal deal;
			deal.init(this_id, wp.defender, wp.attacker, -(warbase + warprediction), wlk);
			deal.generate_offer_to_value(wp.defender, wp.attacker, wlk);
			if (deal.make_offer(wlk))
				return;
		}
	}


	INOUT(auto) target = wp.fronts;

	{

		if (!target.first.recovering && target.second.recovering) { //unopposed

			//war_id_t wid, admin_id_t primary, double sprob, IN(std::vector<prov_id_t>) othersideprv, IN(std::vector<prov_id_t>) ownprv, IN(w_lock) l

			if (wp.attacker.paid)
				addseiges(this_id, wp.attacker.primary, 1.0, bcontrolled, acontrolled, wlk);
			// update_seiges(this_id, true, wp.attacker.seiging);
			target.second.recovering = false;
		} else if (target.first.recovering && !target.second.recovering) { //unopposed
			if (wp.defender.paid)
				addseiges(this_id, wp.defender.primary, 1.0, acontrolled, bcontrolled, wlk);
			// update_seiges(this_id, false, wp.defender.seiging);
			target.first.recovering = false;
		} else if (!target.first.recovering && !target.second.recovering) { // chance of battle
			//update existing seiges
			//const double ptsa = pointsinwar(wc.primary, wc.troops, *target);
			//const double ptsb = pointsinwar(otherprim, wo.parent->troops, *(target->otherhalf));
			const double frnta = pointstoprob(target.first.allocation * a_points);
			const double frntb = pointstoprob(target.second.allocation * b_points);
			//frnta *= frnta;
			//frntb *= frntb;
			const double diff = (frnta + frntb) > 0.0 ? ((frnta + frntb) * global::randdouble() - frntb) / (frnta + frntb) : 0.0;
			if (diff > 0.0) {
				if (wp.attacker.paid)
					addseiges(this_id, wp.attacker.primary, diff, bcontrolled, acontrolled, wlk);
			} else if (diff < 0.0) {
				if (wp.defender.paid)
					addseiges(this_id, wp.defender.primary, -diff, acontrolled, bcontrolled, wlk);
			}

			// update_seiges(this_id, true, wp.attacker.seiging);
			// update_seiges(this_id, false, wp.defender.seiging);

			const static double battlechance = 0.3;

			if (global::randdouble() < battlechance) {
				const double fraction = diff*diff;

				const double loss = std::min(b_troops, ((diff > 0.0) ? 1.0 : (1.0 + diff)) * fraction * 0.5 * target.first.allocation * a_points);
				const double lossb = std::min(a_troops, ((diff < 0.0) ? 1.0 : (1.0 - diff)) * fraction * 0.5 * target.second.allocation * b_points);

				const unsigned int battleday = global::currentday + abs(global::randint()) % 28;
				global::schedule.emplace(battleday, new s_battle(this_id, loss, lossb, diff > 0.0));
				if (diff > 0.0)
					target.second.recovering = true;
				else
					target.first.recovering = true;
			}
		} else {
			target.first.recovering = false;
			target.second.recovering = false;
		}


	}
}

void balance_war(INOUT(war_pair) wp, bool attacker, IN(w_lock) l) {
	wp.fronts.first.allocation = 1.0f;
	wp.fronts.second.allocation = 1.0f;

	/*float fraction = 1.0f / static_cast<float>(wp.fronts.size());
	if (attacker) {
		for (INOUT(auto) fp : wp.fronts) {
			fp.first.allocation = fraction;
		}
	} else {
		for (INOUT(auto) fp : wp.fronts) {
			fp.second.allocation = fraction;
		}
	}*/
}

bool is_at_war(char_id_t id, IN(g_lock) l) noexcept {
	return global::holdertotitle.for_each_breakable(id, l, [&l](title_id_t t) {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			return wars_involved_in.count(to.associated_admin, l) != 0;
		}
		return false;
	});
}

void pack_war_participants(war_id_t war_for, bool aggressor, INOUT(std::vector<std::pair<admin_id_t, std::vector<wargoal>>>) vec, IN(g_lock) l) noexcept {
	IN(auto) wp = get_object(war_for, l);
	IN(auto) w = aggressor ? wp.attacker : wp.defender;
	for (IN(auto) pp : w.participants) {
		auto it = vec.begin();
		for (; it != vec.end(); ++it) {
			if (it->first == pp.adm) {
				if(pp.goal.type != wargoal::WARGOAL_NONE)
					it->second.push_back(pp.goal);
				break;
			}
		}
		if (it == vec.end()) {
			vec.emplace_back(pp.adm, std::vector<wargoal>());
			if (pp.goal.type != wargoal::WARGOAL_NONE)
				vec.back().second.push_back(pp.goal);
		}
	}
}

bool is_agressor(admin_id_t adm, war_id_t war_in, IN(g_lock) l) noexcept {
	IN(auto) wp = get_object(war_in, l);
	for (IN(auto) pp : wp.attacker.participants) {
		if (adm == pp.adm)
			return true;
	}
	return false;
}

bool can_attack(char_id_t source, char_id_t target, IN(g_lock) l) noexcept {
	const auto tprime = get_prime_leige(target, l);
	if (valid_ids(tprime) & (tprime != get_prime_leige(source, l)))
		return false;
	return true;
}