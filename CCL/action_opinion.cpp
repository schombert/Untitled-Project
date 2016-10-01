#include "globalhelpers.h"
#include "action_opinion.h"
#include "relations.h"
#include "structs.hpp"
#include "datamanagement.hpp"
#include "i18n.h"
#include "pacts.h"
#include "laws.h"
#include "finances.h"
#include "events.h"
#include "living_data.h"

template<typename T, typename ... REST>
class trait_list : public trait_list<REST ...> {
private:
	const T value;
public:
	constexpr trait_list(T&& t, REST&& ... rest) : trait_list<REST ...>(std::forward<REST>(rest) ...), value(t) {}
	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		return value.apply_trait(id, dat, l) + trait_list<REST ...>::apply_trait(id, dat, l);
	}
};

template<typename T>
class trait_list<T> {
private:
	const T value;
public:
	constexpr trait_list(T&& t) : value(t) {}
	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		return value.apply_trait(id, dat, l);
	}
};

template<typename ... PARAMS>
auto make_trait_list(PARAMS&& ... params) {
	return trait_list<PARAMS ...>(std::forward<PARAMS>(params) ...);
}

template<int trait, int pos, int neutral, int neg>
class trait_function {
public:
	constexpr trait_function() noexcept {};
	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		const auto tv = dat.attrib.trait_value<trait>();
		if (tv == 1)
			return static_cast<double>(pos) / 100.0;
		else if (tv == 2)
			return static_cast<double>(neg) / 100.0;
		else
			return static_cast<double>(neutral) / 100.0;
	}
};

template<int affect>
class is_vassal_tf {
public:
	const char_id_t leige;
	constexpr is_vassal_tf(char_id_t l) noexcept : leige(l) {};
	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		if (global::holdertotitle.for_each_breakable(id, l, [leige = this->leige, &l](title_id_t t) {
			IN(auto) to = get_object(t);
			if (valid_ids(to.associated_admin)) {
				const auto lg = get_object(to.associated_admin, l).leige;
				return valid_ids(lg) && get_object(get_object(lg, l).associated_title).holder == leige;
			}
			return false;
		})) {
			return static_cast<double>(affect) / 100.0;
		}
		return 0.0;
	}
};

template<int affect>
class fixed_benefit_tf {
public:
	const char_id_t affect_on;
	fixed_benefit_tf(char_id_t c) noexcept : affect_on(c) {};
	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		if(affect_on != id)
			return 0.75 * opinion(id, dat, affect_on, l) * static_cast<double>(affect) / 100.0;
		return static_cast<double>(affect) / 100.0;
	}
};


class change_in_honor_tf {
public:
	const double amount;
	change_in_honor_tf(double a) noexcept : amount(a) {};
	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		return value_of_honor(amount, dat);
	}
};

class change_in_aggression_tf {
public:
	const double amount;
	change_in_aggression_tf(double a) noexcept : amount(a) {};
	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		return value_of_peacefulness(amount, dat);
	}
};

class variable_benefit_tf {
public:
	const char_id_t affect_on;
	const double affect_size;
	variable_benefit_tf(char_id_t c, double s) noexcept : affect_on(c), affect_size(s) {};
	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		if (affect_on != id)
			return 0.75 * opinion(id, dat, affect_on, l) * affect_size;
		return affect_size;
	}
};

template<typename T>
class trait_affect_class {
public:
	T mtrait_list;

	template<typename ... PARAMS>
	trait_affect_class(std::true_type, PARAMS&& ... params) : mtrait_list(std::forward<PARAMS>(params) ...) {};

	double apply_trait(char_id_t id, IN(udata) dat, IN(g_lock) l) const noexcept {
		double total = 0.0;
		enum_not_indifferent(id, l, [th = this, id, &dat, &total, &l](char_id_t other) {
			total += opinion(id, dat, other, l) * with_udata_force(other, l, [th, &l, other](IN(udata) d) { return th->mtrait_list.apply_trait(other, d, l); });
		});
		return 0.75 * total;
	}
};

template<typename ... PARAMS>
auto make_trait_class(PARAMS&& ... params) {
	return trait_affect_class<trait_list<PARAMS ...>>(std::true_type(), std::forward<PARAMS>(params) ...);
}

template<typename T>
double opinion_of_action(char_id_t source, IN(T) tf, IN(g_lock) l) noexcept {
	return with_udata(source, l, 0.0, [&l, &source, &tf](IN(udata) d) noexcept {
		return tf.apply_trait(source, d, l);
	});
}

template<typename T, typename A>
void opinion_of_action_v_auto(INOUT(std::vector<std::pair<char_id, double>, A>) source, IN(T) tf, IN(g_lock) l) noexcept {
	for (INOUT(auto) p : source) {
		p.second = opinion_of_action<n>(p.first, tf, l);
	}
}

template<typename T, typename A>
void opinion_of_action_v(char_id_t actor, INOUT(std::vector<std::pair<char_id_t, double>, A>) source, IN(T) tf, IN(std::function<int(int, int, int, std::shared_ptr<uiElement>)>) description, INOUT(w_lock) l) noexcept {
	for (INOUT(auto) p : source) {
		if (p.first != global::playerid) {
			p.second = opinion_of_action(p.first, tf, l);
		} else {
			l.unlock();
			const double ai_opinion = opinion_of_action(global::playerid, tf, l);
			int res = make_trinary_popup(global::uicontainer, get_simple_string(TX_C_OPINION), [&description, actor](IN(std::shared_ptr<uiScrollView>)sv) {
				size_t param = actor.value;
				int cy = 5;

				get_linear_ui(TX_C_INTRO, &param, 1, sv, 5, cy, global::empty, global::standard_text);
				cy += global::standard_text.csize + 5;

				cy += 5 + description(5, cy, sv->pos.width - 10, sv);

				get_linear_ui(TX_C_FINAL, &param, 1, sv, 5, cy, global::empty, global::standard_text);
				cy += global::standard_text.csize + 5;
			}, ai_opinion <= OPINION_THRESHOLD ? 1 : 0, (ai_opinion > OPINION_THRESHOLD || ai_opinion < -OPINION_THRESHOLD) ? 1 : 0, ai_opinion >= -OPINION_THRESHOLD ? 1 : 0, approve_abstain_oppose_array);
			l.lock();
			p.second = 2.0 * OPINION_THRESHOLD * static_cast<double>(res);
		}
	}
}

void adjust_for_action_taken(char_id_t actor, IN(std::vector<std::pair<char_id_t, double>>) source, IN(w_lock) l) noexcept {
	for (IN(auto) p : source) {
		if (p.second > OPINION_THRESHOLD) {
			adjust_relation(p.first, actor, 1, l);
		} else if (p.second < -OPINION_THRESHOLD) {
			adjust_relation(p.first, actor, -1, l);
		}
	}
}

void build_concil_vec(INOUT(std::vector<std::pair<char_id_t, double>>) v, char_id_t actor, IN(g_lock) l) noexcept {
	const auto pa = get_prime_admin(actor, l);
	if (valid_ids(pa)) {
		global::enum_council(pa, l, [&v](char_id_t id) {v.push_back(std::make_pair(id, 0.0)); });
	}
}

void build_concil_vec_a(INOUT(std::vector<std::pair<char_id_t, double>>) v, admin_id_t a, IN(g_lock) l) noexcept {
	global::enum_council(a, l, [&v](char_id_t id) {v.push_back(std::make_pair(id, 0.0)); });
}

bool confirm_action(IN(std::vector<std::pair<char_id_t, double>>) v, IN(std::function<int(int, int, int, std::shared_ptr<uiElement>)>) description, INOUT(w_lock) l) noexcept {
	if (v.size() == 0)
		return true;

	const int cbias = std::accumulate(v.begin(), v.end(), 0, [](int iv, IN(std::pair<char_id_t, double>) p) {
		if (p.second > OPINION_THRESHOLD)
			return iv + 1;
		else if (p.second < -OPINION_THRESHOLD)
			return iv - 1;
		return iv;
	});
	l.unlock();
	const auto res = make_yes_no_popup(global::uicontainer, get_simple_string(TX_C_OPINION), [&v, &description](IN(std::shared_ptr<uiScrollView>) sv) {
		int cy = 5 + description(5, 5, sv->pos.width -10, sv) + 5;

		get_linear_ui(TX_C_OPPOSED, sv, 5, cy, global::empty, global::standard_text);
		cy += global::standard_text.csize + 5;

		bool found = false;
		for (IN(auto) p : v) {
			if (p.second < -OPINION_THRESHOLD) {
				sv->add_element<ui_ch_hlink>(10, cy, p.first.value);
				cy += global::standard_text.csize + 5;
				found = true;
			}
		}
		if (!found) {
			get_linear_ui(TX_NONE, sv, 5, cy, global::empty, global::standard_text);
			cy += global::standard_text.csize + 5;
		}
		
		get_linear_ui(TX_C_INFAVOR, sv, 5, cy, global::empty, global::standard_text);
		cy += global::standard_text.csize + 5;

		found = false;
		for (IN(auto) p : v) {
			if (p.second > OPINION_THRESHOLD) {
				sv->add_element<ui_ch_hlink>(10, cy, p.first.value);
				cy += global::standard_text.csize + 5;
				found = true;
			}
		}

		if (!found) {
			get_linear_ui(TX_NONE, sv, 5, cy, global::empty, global::standard_text);
			cy += global::standard_text.csize + 5;
		}

	}, cbias < 0 ? -cbias / 2 : 0, cbias > 0 ? cbias / 2 : 0, proceed_cancel_array);
	l.lock();
	return res;
}

constexpr double fixed_cost_of_extra_war = 0.40;

double defensive_envoy_opinion(char_id_t from, char_id_t hos, IN(g_lock) l) noexcept {
	return opinion_of_action(from,
		make_trait_list(trait_function<TRAIT_PEACEFUL, 20, 0, -10>(),
			trait_function<TRAIT_CAUTIOUS, 15 , 0, -30>(),
			variable_benefit_tf(hos, static_cast<double>(max_threat_percentage(hos, l)) - fixed_cost_of_extra_war)),
		l) - OPINION_THRESHOLD;
}

auto defensive_pact_tf(char_id_t hos, char_id_t with, IN(pact_data) pact, IN(g_lock) l) noexcept {
	return make_trait_list(trait_function<TRAIT_PEACEFUL, 20, 0, -10>(),
		trait_function<TRAIT_CAUTIOUS, 15, 0, -30>(),
		variable_benefit_tf(hos, static_cast<double>(max_threat_percentage(hos, l) + tribute_adjuistment(pact, hos, l)) - fixed_cost_of_extra_war),
		variable_benefit_tf(with, static_cast<double>(max_threat_percentage(with, l) + tribute_adjuistment(pact, with, l)) - fixed_cost_of_extra_war));
}


double defensive_against_envoy_opinion(char_id_t from, char_id_t hos, char_id_t against, IN(g_lock) l) noexcept {
	const auto hos_mu = mu_estimate(hos, l);
	const auto against_mu = mu_estimate(against, l);
	return opinion_of_action(from,
		make_trait_list(trait_function<TRAIT_PEACEFUL, 20, 0, -10>(),
			trait_function<TRAIT_CAUTIOUS, 15, 0, -30>(),
			variable_benefit_tf(against, -1.0 * static_cast<double>(hos_mu / (against_mu + hos_mu)) ),
			variable_benefit_tf(hos, static_cast<double>(against_mu / (against_mu + hos_mu)) - fixed_cost_of_extra_war)),
		l) - OPINION_THRESHOLD;
}

auto defensive_against_pact_tf(char_id_t hos, char_id_t with, IN(pact_data) pact, IN(g_lock) l) noexcept {
	const auto hos_mu = mu_estimate(hos, l);
	const auto with_mu = mu_estimate(with, l);
	const auto against_mu = mu_estimate(pact.against, l);
	return make_trait_list(trait_function<TRAIT_PEACEFUL, 20, 0, -10>(),
		trait_function<TRAIT_CAUTIOUS, 15, 0, -30>(),
		variable_benefit_tf(pact.against, -1.0 * static_cast<double>((hos_mu + with_mu) / (against_mu + hos_mu + with_mu))),
		variable_benefit_tf(hos, static_cast<double>(tribute_adjuistment(pact, hos, l) + against_mu / (against_mu + hos_mu)) - fixed_cost_of_extra_war),
		variable_benefit_tf(with, static_cast<double>(tribute_adjuistment(pact, with, l) + against_mu / (against_mu + with_mu)) - fixed_cost_of_extra_war));
}

double non_aggression_envoy_opinion(char_id_t from, char_id_t hos, char_id_t with, IN(g_lock) l) noexcept {
	const auto hos_mu = mu_estimate(hos, l);
	const auto with_mu = mu_estimate(with, l);
	return opinion_of_action(from,
		make_trait_list(trait_function<TRAIT_PEACEFUL, 20, 0, -20>(),
			trait_function<TRAIT_CAUTIOUS, 10, 0, -5>(),
			fixed_benefit_tf<40>(with), // = fixed cost of war
			variable_benefit_tf(hos, -1.0 *  war_estimation(hos, with, l))),
		l) - OPINION_THRESHOLD;
}

auto non_aggression_pact_tf(char_id_t hos, char_id_t with, IN(pact_data) pact, IN(g_lock) l) noexcept {
	const auto hos_mu = mu_estimate(hos, l);
	const auto with_mu = mu_estimate(with, l);
	return make_trait_list(trait_function<TRAIT_PEACEFUL, 20, 0, -20>(),
			trait_function<TRAIT_CAUTIOUS, 10, 0, -5>(),
			variable_benefit_tf(with, -1.0 * war_estimation(with, hos, l) + tribute_adjuistment(pact, hos, l)),
			variable_benefit_tf(hos, -1.0 * war_estimation(hos, with, l) + tribute_adjuistment(pact, with, l)));
}

auto break_pact_against_tf(char_id_t breaker, char_id_t b, char_id_t against, pact_id_t pid, IN(g_lock) l) {
	const auto hos_mu = mu_estimate(breaker, l);
	const auto against_mu = mu_estimate(against, l);
	IN(auto) p = get_object(pid, l);

	return make_trait_list(
			variable_benefit_tf(breaker, -continuing_desirability_of_pact(pid, breaker, l)),
			change_in_honor_tf(-honor_loss_on_break_val(p, breaker, l)),
			variable_benefit_tf(b, -continuing_desirability_of_pact(pid, b, l)),
			variable_benefit_tf(against, static_cast<double>(hos_mu / (against_mu + hos_mu))));
}

double break_pact_opinion(char_id_t from, char_id_t hos, pact_id_t pid, IN(g_lock) l) noexcept {
	IN(auto) p = get_object(pid, l);
	if (p.pact_type != pact_data::DEFENSE_AGAINST) {
		const auto other = (hos == p.a) ? p.b : p.a;
		return opinion_of_action(from,
			make_trait_list(
				variable_benefit_tf(hos, -continuing_desirability_of_pact(pid, hos, l)),
				change_in_honor_tf(-honor_loss_on_break_val(p, hos, l)),
				variable_benefit_tf(other, -continuing_desirability_of_pact(pid, other, l))),
			l) - OPINION_THRESHOLD;
	} else {
		const auto other = (hos == p.a) ? p.b : p.a;

		const auto hos_mu = mu_estimate(hos, l);
		const auto against_mu = mu_estimate(p.against, l);

		return opinion_of_action(from,
			break_pact_against_tf(hos, other, p.against, pid, l),
			l) - OPINION_THRESHOLD;
	}
	
}

double deception_plot_opinion(char_id_t from, char_id_t source, char_id_t target, IN(g_lock) l) noexcept {
	return opinion_of_action(from,
		make_trait_list(trait_function<TRAIT_KIND, -10, 0, 10>(),
			change_in_honor_tf(-10.0),
			fixed_benefit_tf<-50>(target)), // revise with accurate estimate for value of X lost
		l) - OPINION_THRESHOLD;
}



double host_mass_event_opinion(char_id_t from, char_id_t host, unsigned int type, IN(g_lock) l) noexcept {
	return opinion_of_action(from,
		make_trait_list(trait_function<TRAIT_DECADENT, 20, 0, -10>(),
			trait_function<TRAIT_EXTROVERT, 20, -0, -30>(),
			is_vassal_tf<10>(host),
			make_trait_class(is_vassal_tf<10>(host)),
			variable_benefit_tf(host, 0.10 - (((global::project_income(host, l) - current_mthly_expense(host, l)) <= event_template_by_id(type).monthly_cost(host, char_id_t(), l)) ? 0.45 : 0.0))),
		l) - OPINION_THRESHOLD;
}


double attack_opinion(char_id_t from, char_id_t attacker, char_id_t target, wargoal goal, IN(g_lock) l) noexcept {
	const double war_estimate = war_estimation(attacker, target, l);
	
	return opinion_of_action(from,
		make_trait_list(
			change_in_aggression_tf(goal.type == wargoal::WARGOAL_CONQUEST ? -aggression_cost_for_no_cb(attacker, l) : 0.0),
			change_in_honor_tf(-honor_cost_for_attack(attacker, target, l)),
			trait_function<TRAIT_CAUTIOUS, -30, -5, 5>(),
			variable_benefit_tf(target, -war_estimate),
			variable_benefit_tf(attacker, war_estimate)),
		l) - OPINION_THRESHOLD;
}

bool attack_reaction(char_id_t attacker, char_id_t target, wargoal goal, INOUT(w_lock) l) noexcept {
	std::vector<std::pair<char_id_t, double>> councilvec;
	build_concil_vec(councilvec, attacker, l);

	const auto description = [target](int x, int y, int width, IN(std::shared_ptr<uiElement>) parent) {
		size_t param = target.value;
		return create_tex_block(TX_NOCB_D, &param, 1, parent, x, y, width, global::empty, global::standard_text);
	};
	const double war_estimate = war_estimation(attacker, target, l);
	
	opinion_of_action_v(attacker, councilvec,
		make_trait_list(
			change_in_aggression_tf(goal.type == wargoal::WARGOAL_CONQUEST ? -aggression_cost_for_no_cb(attacker, l) : 0.0),
			change_in_honor_tf(-honor_cost_for_attack(attacker, target, l)),
			trait_function<TRAIT_CAUTIOUS, -30, -5, 5>(),
			variable_benefit_tf(target, -war_estimate),
			variable_benefit_tf(attacker, war_estimate)),
		std::cref(description), l);
	
	if (attacker != global::playerid || confirm_action(councilvec, std::cref(description), l)) {
		adjust_for_action_taken(attacker, councilvec, l);
		return true;
	}
	return false;
}

bool pact_reaction_a(admin_id_t a, char_id_t actor, IN(pact_data) pact, INOUT(w_lock) l) noexcept {
	switch (pact.pact_type) {
		case pact_data::DEFENSE:
		{
			std::vector<std::pair<char_id_t, double>> councilvec;
			build_concil_vec_a(councilvec, admin_id_t(a), l);

			const auto description = [&pact, &l](int x, int y, int width, IN(std::shared_ptr<uiElement>) parent) {
				int initialy = y;
				get_linear_ui(TX_PACT_D, parent, x, y, global::empty, global::standard_text);
				y += global::standard_text.csize + 5;
				pact_to_ui(x, y, parent, pact, l);
				return y - initialy;
			};
			opinion_of_action_v(actor, councilvec,
				defensive_pact_tf(pact.a, pact.b, pact, l),
				std::cref(description), l);
			if (actor != global::playerid || confirm_action(councilvec, std::cref(description), l)) {
				adjust_for_action_taken(actor, councilvec, l);
				return true;
			}
			return false;
		}
		case pact_data::DEFENSE_AGAINST:
		{
			std::vector<std::pair<char_id_t, double>> councilvec;
			build_concil_vec_a(councilvec, admin_id_t(a), l);

			const auto description = [&pact, &l](int x, int y, int width, IN(std::shared_ptr<uiElement>) parent) {
				int initialy = y;
				get_linear_ui(TX_PACT_D, parent, x, y, global::empty, global::standard_text);
				y += global::standard_text.csize + 5;
				pact_to_ui(x, y, parent, pact, l);
				return y - initialy;
			};
			opinion_of_action_v(actor, councilvec,
				defensive_against_pact_tf(pact.a, pact.b, pact, l),
				std::cref(description), l);
			if (actor != global::playerid || confirm_action(councilvec, std::cref(description), l)) {
				adjust_for_action_taken(actor, councilvec, l);
				return true;
			}
			return false;
		}
		case pact_data::NON_AGRESSION:
		{
			std::vector<std::pair<char_id_t, double>> councilvec;
			build_concil_vec_a(councilvec, admin_id_t(a), l);

			const auto description = [&pact, &l](int x, int y, int width, IN(std::shared_ptr<uiElement>) parent) {
				int initialy = y;
				get_linear_ui(TX_PACT_D, parent, x, y, global::empty, global::standard_text);
				y += global::standard_text.csize + 5;
				pact_to_ui(x, y, parent, pact, l);
				return y - initialy;
			};
			opinion_of_action_v(actor, councilvec,
				non_aggression_pact_tf(pact.a, pact.b, pact, l),
				std::cref(description), l);
			if (actor != global::playerid || confirm_action(councilvec, std::cref(description), l)) {
				adjust_for_action_taken(actor, councilvec, l);
				return true;
			}
			return false;
		}
		default:
			return true;
	}
}

bool dishonor_def_pact_reaction(char_id_t actor, char_id_t other, char_id_t attacker, pact_id_t pid, INOUT(w_lock) l) noexcept {
	std::vector<std::pair<char_id_t, double>> councilvec;
	build_concil_vec(councilvec, actor, l);

	const auto description = [actor, other, attacker](int x, int y, int width, IN(std::shared_ptr<uiElement>) parent) {
		size_t params[] = {actor.value, other.value, attacker.value};
		return create_tex_block(TX_DPACT_D, params, 3, parent, x, y, width, global::empty, global::standard_text);
	};
	opinion_of_action_v(actor, councilvec,
		break_pact_against_tf(actor, other, attacker, pid, l),
		std::cref(description), l);
	if (actor != global::playerid || confirm_action(councilvec, std::cref(description), l)) {
		adjust_for_action_taken(actor, councilvec, l);
		return true;
	}
	return false;
}