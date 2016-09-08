#include "globalhelpers.h"
#include "peace.h"
#include "wardata.h"
#include "structs.hpp"
#include "actions.h"
#include "uielements.hpp"
#include "i18n.h"
#include "requests.h"
#include "datamanagement.hpp"
#include "living_data.h"
#include "WarPane.h"
#include "prov_control.h"
#include "relations.h"

template <typename LOCK>
bool _make_offer(INOUT(peace_deal) deal, INOUT(LOCK) l) noexcept {
	IN(auto) wp = get_object(deal.assocated_war, l);
	if (will_accept_peace(is_agressor(deal.offer_from, deal.assocated_war, l) ?
		get_diplo_decider(get_object(wp.defender.primary, l), l) :
		get_diplo_decider(get_object(wp.attacker.primary, l), l), deal, l)) {
		implement_peace_offer_fac(get_diplo_decider(get_object(deal.offer_from, l), l), std::move(deal));
		return true;
	}
	deal.reset();
	return false;
}
bool peace_deal::make_offer(INOUT(r_lock) l) noexcept {
	return _make_offer(*this, l);
}
bool peace_deal::make_offer(INOUT(w_lock) l) noexcept {
	return _make_offer(*this, l);
}

double peace_deal::total_value() const noexcept {
	double sum = 0.0;
	for (const auto& o : value_distribution)
		sum += o.second;
	return sum;
}


void peace_deal::reset() noexcept {
	goals.clear();
	value_distribution.clear();
	assocated_war = war_id_t::NONE;
}


void peace_deal::make_demand(IN(g_lock) l) noexcept {
	//add action
	global::actionlist.add_new<implement_peace_offer>(head_of_state(offer_from,l), std::move(*this));
	reset();
}

void peace_deal::implement_offer(INOUT(w_lock) l) const noexcept {
	INOUT(auto) wp = war_pool.get(assocated_war.value, l);
	INOUT(auto) tside = wp.attacker.primary == offer_from ? wp.attacker : wp.defender;
	INOUT(auto) oside = wp.attacker.primary == offer_from ? wp.defender : wp.attacker;

	flat_set<admin_id_t> against;
	const auto offer_to = oside.primary;
	const auto other_hos = head_of_state(oside.primary, l);
	const auto actor_hos = head_of_state(offer_from, l);
	list_participants(oside, against, l);

	bool interested_participant = false;
	for (IN(auto) p : wp.attacker.participants) {
		if (global::interested.in_set(head_of_state(admin_id_t(p.adm), l).value)) {
			interested_participant = true;
			break;
		}
	}
	if (!interested_participant) {
		for (IN(auto) p : wp.defender.participants) {
			if (global::interested.in_set(head_of_state(admin_id_t(p.adm), l).value)) {
				interested_participant = true;
				break;
			}
		}
	}

	if ((interested_participant) &&
		(actor_hos != global::playerid || !is_demand) &&
		(other_hos != global::playerid || is_demand)) {
		message_popup(global::uicontainer, get_simple_string(TX_PEACE_ACCOUNCEMENT), [actor_hos, other_hos, deal = this, wp, &l](const std::shared_ptr<uiScrollView>& sv) {
			size_t params[] = {actor_hos.value, other_hos.value};
			if (other_hos == global::playerid) {
				if (deal->is_demand) {
					const auto blk = create_tex_block(TX_P_ENFORCED_US, params, 2, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
					sv->subelements.push_back(blk);
					int y = blk->pos.height + 10;
					deal->to_ui(sv, 0, y, l);
				}
			} else {
				if (!deal->is_demand) {
					const auto blk = create_tex_block(TX_P_OFFER_ACCEPTED, params, 2, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
					sv->subelements.push_back(blk);
					int y = blk->pos.height + 10;
					deal->to_ui(sv, 0, y, l);
					
				} else {
					const auto blk = create_tex_block(TX_P_ENFORCED, params, 2, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
					sv->subelements.push_back(blk);
					int y = blk->pos.height + 10;
					deal->to_ui(sv, 0, y, l);
				}
			}

		});
	}

	std::vector<prov_id_t> plist;
	for (auto a : against) {
		global::get_controlled_by_admin(a, plist, l);
	}

	if (is_demand) {
		vector_erase_if(plist, [&l, aw = this->assocated_war](prov_id_t p) {return !occupation_info.contains(p, l) || occupation_info.get(p,l).in_war != aw; });
	}

	flat_set<char_id_t> territory_losers;
	flat_set<char_id_t> territory_gainers;

	auto value_distribution_copy(value_distribution);

	for (IN(auto) g : goals) {
		const auto hos = head_of_state(g.deal_for, l);
		if (g.goal.type == wargoal::WARGOAL_CONQUEST || g.goal.type == wargoal::WARGOAL_DEJURE) {
			territory_gainers.insert(hos);
		}
		value_distribution_copy[hos] -= enact_wargoal_on_a(is_demand, g.goal, value_distribution_copy[hos], g.deal_for, against, plist, territory_losers, l);
	}
	
	
	for (auto ch : territory_losers) {
		update_territory_lost(ch, l);
	}
	for (auto ch : territory_gainers) {
		update_territory_gained(ch, l);
	}
	
	end_war(assocated_war, l);


	add_peace_treaty(actor_hos, other_hos, global::currentday + 365 * 5, l);

	if (global::mapmode == MAP_MODE_WAR || global::mapmode == MAP_MODE_WARSEL) {
		global::setFlag(FLG_MAP_UPDATE);
	}
	//if (other_hos == global::playerid || actor_hos == global::playerid) {
		war_window_rec.needs_update = true;
	//}

	if (global::interested.in_set(other_hos.value) || global::interested.in_set(actor_hos.value))
		makePeaceDeclaration(get_object(offer_from,l).associated_title.value, get_object(offer_to, l).associated_title.value);

}

template<typename L>
void _peace_deal_init(INOUT(peace_deal) deal, war_id_t war_for, IN(newwar) wr, IN(newwar) other, double v, IN(L) l) noexcept {
	deal.assocated_war = war_for;
	deal.offer_from = wr.primary;

	double total_share = 0.0;
	flat_map<char_id_t, double, std::less<char_id_t>, typename allocator_t<L>::type<std::pair<char_id_t, double>>> share_accumulation;
	flat_set<char_id_t, std::less<char_id_t>, typename allocator_t<L>::type<char_id_t>> has_demand;

	for (IN(auto) p : wr.participants) {
		const auto hos = head_of_state(p.adm, l);
		share_accumulation[hos] = points_in_war(p.adm, l);

		if (p.goal.type != wargoal::WARGOAL_NONE) {
			deal.goals.emplace_back(p.adm, p.goal);
			has_demand.insert(hos);
		}
	}

	for (IN(auto) pr : share_accumulation) {
		if (has_demand.count(pr.first) != 0)
			total_share += pr.second;
	}

	for (IN(auto) pr : share_accumulation) {
		if (has_demand.count(pr.first) != 0)
			deal.value_distribution[pr.first] = v * (pr.second / total_share);
		else
			deal.value_distribution[pr.first] = 0.0;
	}
}

void peace_deal::init(war_id_t war_for, IN(newwar) wr, IN(newwar) other, double v, IN(g_lock) l) noexcept {
	_peace_deal_init(*this, war_for, wr, other, v, l);
}
void peace_deal::init(war_id_t war_for, IN(newwar) wr, IN(newwar) other, double v, IN(w_lock) l) noexcept {
	_peace_deal_init(*this, war_for, wr, other, v, l);
}

void peace_deal::init_w_goals(war_id_t war_for, IN(std::vector<deal_part>) goals_in, IN(newwar) wr, double v, IN(g_lock) l) noexcept {
	assocated_war = war_for;
	offer_from = wr.primary.value;

	double total_share = 0.0;
	flat_map<char_id_t, double> share_accumulation;
	flat_set<char_id_t> has_demand;

	for (IN(auto) p : wr.participants) {
		const auto hos = head_of_state(admin_id_t(p.adm), l);
		share_accumulation[hos] = points_in_war(p.adm, l);
	}

	goals.insert(goals.end(), goals_in.begin(), goals_in.end());

	for (IN(auto) pr : share_accumulation) {
		if (has_demand.count(pr.first) != 0)
			total_share += pr.second;
	}

	for (IN(auto) pr : share_accumulation) {
		if (has_demand.count(pr.first) != 0)
			value_distribution[pr.first] = v * (pr.second / total_share);
		else
			value_distribution[pr.first] = 0.0;
	}
}

template<typename L>
bool _peace_deal_offer_valid(IN(peace_deal) deal, IN(L) l) noexcept {
	double totalvalue = 0.0;
	for (IN(auto) p : deal.value_distribution) {
		totalvalue += p.second;
	}

	IN(auto) wp = get_object(deal.assocated_war, l);
	IN(auto) aside = wp.attacker.primary == deal.offer_from ? wp.attacker : wp.defender;
	IN(auto) oside = wp.attacker.primary == deal.offer_from ? wp.defender : wp.attacker;

	std::vector<prov_id_t, typename allocator_t<L>::type<prov_id_t>> acontrolled;
	std::vector<prov_id_t, typename allocator_t<L>::type<prov_id_t>> bcontrolled;

	flat_set<admin_id_t, std::less<admin_id_t>, typename allocator_t<L>::boost_type<admin_id_t>> all_a;
	flat_set<admin_id_t, std::less<admin_id_t>, typename allocator_t<L>::boost_type<admin_id_t>> all_o;
	list_participants(aside, all_a, l);
	list_participants(oside, all_o, l);

	for (const auto a : all_a)
		controlled_by_admin(a, acontrolled, l);
	for (const auto a : all_o)
		controlled_by_admin(a, bcontrolled, l);

	const double warbase = war_base_tax_value(deal.assocated_war, acontrolled, bcontrolled, l);

	if (deal.is_demand) {
		return deal.can_demand(aside, oside, all_a, all_o, acontrolled, bcontrolled, l) && totalvalue <= warbase;
	} else {
		return totalvalue <= warbase + war_prediction_value(deal.assocated_war, wp.attacker.primary == deal.offer_from, acontrolled, bcontrolled, l);
	}
}

bool peace_deal::offer_valid(IN(g_lock) l) noexcept {
	return _peace_deal_offer_valid(*this, l);
}
bool peace_deal::offer_valid(IN(w_lock) l) noexcept {
	return _peace_deal_offer_valid(*this, l);
}

template<typename T, typename U, typename L>
bool _peace_deal_can_demand(IN(peace_deal) deal, IN(newwar) wr, IN(newwar) other, IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) apart, IN(flat_set<admin_id_t, std::less<admin_id_t>, T>) bpart, IN(std::vector<prov_id_t, U>) nom_a_controlled, IN(std::vector<prov_id_t, U>) nom_b_controlled, IN(L) l) noexcept {
	if (!can_enforce(other, apart,  bpart,  nom_b_controlled, l)) return false;
	bool candemand = true;
	for (IN(auto) g : deal.goals)
		candemand = candemand || can_enforce_goal(g.deal_for, g.goal, wr, other, nom_a_controlled, nom_b_controlled, l);
	return candemand;
}

bool peace_deal::can_demand(IN(newwar) wr, IN(newwar) other, IN(flat_set<admin_id_t>) apart, IN(flat_set<admin_id_t>) bpart, IN(std::vector<prov_id_t>) nom_a_controlled, IN(std::vector<prov_id_t>) nom_b_controlled, IN(w_lock) l) const noexcept {
	return _peace_deal_can_demand(*this, wr, other, apart, bpart, nom_a_controlled, nom_b_controlled, l);
}

bool peace_deal::can_demand(IN(newwar) wr, IN(newwar) other, IN(cflat_set<admin_id_t>) apart, IN(cflat_set<admin_id_t>) bpart, IN(cvector<prov_id_t>) nom_a_controlled, IN(cvector<prov_id_t>) nom_b_controlled, IN(g_lock) l) const noexcept {
	return _peace_deal_can_demand(*this, wr, other, apart, bpart, nom_a_controlled, nom_b_controlled, l);
}


void peace_deal::remove_goal(size_t n, IN(g_lock) l) noexcept {
	goals[n] = std::move(goals.back());
	goals.pop_back();
}

template<typename L>
void _peace_deal_generate_offer_to_value(INOUT(peace_deal) deal, IN(newwar) wr, IN(newwar) other, IN(L) l) noexcept {
	deal.is_demand = false;
	flat_set<admin_id_t, std::less<admin_id_t>, typename allocator_t<L>::boost_type<admin_id_t>> tlist;
	list_participants(other, tlist, l);

	flat_map<char_id_t, double, std::less<char_id_t>, typename allocator_t<L>::boost_type<std::pair<char_id_t, double>>> value_accumulation;
	double accvalue = 0.0;
	for (size_t i = deal.goals.size() - 1; i != SIZE_MAX; --i) {
		const auto hos = head_of_state(deal.goals[i].deal_for, l);
		value_accumulation[hos] += wargoal_value(deal.goals[i].goal, deal.goals[i].deal_for, tlist, l);
		if (value_accumulation[hos] > deal.value_distribution[hos]) {
			if (wargoal_allows_partial(deal.goals[i].goal)) {
				value_accumulation[hos] = deal.value_distribution[hos];
			} else {
				deal.goals[i] = deal.goals.back();
				deal.goals.pop_back();
			}
		}
	}
}

template<typename L>
void _peace_deal_generate_demand_to_value(INOUT(peace_deal) deal, IN(newwar) wr, IN(newwar) other, IN(L) l) noexcept {
	deal.is_demand = true;
	flat_set<admin_id_t, std::less<admin_id_t>, typename allocator_t<L>::boost_type<admin_id_t>> tlist;
	list_participants(other, tlist, l);

	flat_map<char_id_t, double, std::less<char_id_t>, typename allocator_t<L>::boost_type<std::pair<char_id_t, double>>> value_accumulation;
	double accvalue = 0.0;
	for (size_t i = deal.goals.size() - 1; i != SIZE_MAX; --i) {
		const auto hos = head_of_state(deal.goals[i].deal_for, l);
		value_accumulation[hos] += wargoal_value(deal.goals[i].goal, deal.goals[i].deal_for, tlist, l);
		if (value_accumulation[hos] > deal.value_distribution[hos]) {
			if (wargoal_allows_partial(deal.goals[i].goal)) {
				value_accumulation[hos] = deal.value_distribution[hos];
			} else {
				deal.goals[i] = deal.goals.back();
				deal.goals.pop_back();
			}
		}
	}
}

void peace_deal::generate_offer_to_value(IN(newwar) wr, IN(newwar) other, IN(g_lock) l) noexcept {
	_peace_deal_generate_offer_to_value(*this, wr, other, l);
}
void peace_deal::generate_demand_to_value(IN(newwar) wr, IN(newwar) other, IN(g_lock) l) noexcept {
	_peace_deal_generate_demand_to_value(*this, wr, other, l);
}
void peace_deal::generate_offer_to_value(IN(newwar) wr, IN(newwar) other, IN(w_lock) l) noexcept {
	_peace_deal_generate_offer_to_value(*this, wr, other, l);
}
void peace_deal::generate_demand_to_value(IN(newwar) wr, IN(newwar) other, IN(w_lock) l) noexcept {
	_peace_deal_generate_demand_to_value(*this, wr, other, l);
}

void peace_deal::to_ui(IN(std::shared_ptr<uiElement>) parent, int x, INOUT(int) y, IN(g_lock) l) const noexcept {
	//display_goal(IN(std::shared_ptr<uiElement>) parent, int x, int &y, IN(wargoal) goal, char_id actor, char_id target, double totalval, bool enforcing);

	IN(auto) wrp = war_pool.get(assocated_war.value, l);

	IN(auto) wr = wrp.attacker.primary == offer_from ? wrp.attacker : wrp.defender;
	IN(auto) otherw = wrp.attacker.primary == offer_from ? wrp.defender : wrp.attacker;

	cflat_set<admin_id_t> against;
	list_participants(otherw, against, l);

	cvector<prov_id_t> plist;

	for (auto a : against) {
		global::get_controlled_by_admin(a, plist, l);
	}

	flat_map<char_id_t, double> val_copy(value_distribution);

	for (IN(auto) g : goals) {
		const auto hos = head_of_state(g.deal_for, l);
		if (val_copy[hos] > 0.0) {
			if (!wargoal_has_prov_list(g.goal)) {
				val_copy[hos] -= display_goal(parent, x, y, g.goal, hos, val_copy[hos], l, is_demand);
			} else {
				display_goal(parent, x, y, g.goal, hos, val_copy[hos], l, is_demand);
				cvector<prov_id_t> provs;

				//list_goal_provinces_a(provs, IN(wargoal) goal, admin_id adm_for, IN(flat_set<admin_id>) against, double totalval, bool enforcing, INOUT(std::vector<prov_id>) ppool, IN(g_lock) l);
				val_copy[hos] -= list_goal_provinces_a(provs, g.goal, g.deal_for, against, val_copy[hos], is_demand, plist, l);
				size_t params[] = {0ui64, hos.value};
				for (const auto p : provs) {
					params[0] = p.value;
					get_linear_ui(TX_P_TRANSFER, params, 2, parent, x, y, global::empty, global::standard_text);
					y += global::standard_text.csize + 2;
				}
			}
		}
	}
}
