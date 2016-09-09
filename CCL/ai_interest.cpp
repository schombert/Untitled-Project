#include "globalhelpers.h"
#include "ai_interest.h"
#include "structs.hpp"
#include "relations.h"
#include "datamanagement.hpp"
#include "living_data.h"
#include "prov_control.h"
#include "laws.h"


flat_map<ordered_pair<char_id_t>, interest_relation_contents> interest_relations;
flat_multimap<char_id_t, char_id_t> interested_in;
single_index_t<char_id_t, float> defensive_mu;

float q_aggr_value(char_id_t id, IN(g_lock) l) noexcept {
	return with_udata(id, l, 1.0f, [](IN(udata) d) noexcept { return static_cast<float>(0.5 + (1.0 - d.p_peaceful)); });
}

void init_interests(IN(w_lock) l) noexcept {
	interest_relations.clear();
	interested_in.clear();
	for (const auto lv : global::living) {
		if(valid_ids(get_prime_admin(lv, l)))
			update_territory_gained(lv, l);
	}
}

decltype(std::declval<interest_relation_contents>().tag_a) get_tag(char_id_t id, IN(std::pair<ordered_pair<char_id_t>, interest_relation_contents>) pr) noexcept {
	if (pr.first.first == id)
		return pr.second.tag_a;
	else
		return pr.second.tag_b;
}

void update_interests_char_id(char_id_t old, char_id_t n, IN(w_lock) l) noexcept {
	std::vector<std::pair<char_id_t, char_id_t>> tmp;
	for (INOUT(auto) pr : interested_in) {
		if (pr.second == old) {
			pr.second = n;
		}
		if (pr.first == old) {
			tmp.emplace_back(n, pr.second);
		}
	}
	interested_in.erase(old);
	interested_in.insert(boost::container::ordered_unique_range, tmp.begin(), tmp.end());


	std::vector<std::pair<ordered_pair<char_id_t>, interest_relation_contents>> rpairs;
	for (IN(auto) pr : interest_relations) {
		if (pr.first.first == old) {
			if(n.value <= pr.first.second.value)
				rpairs.emplace_back(ordered_pair<char_id_t>(n, pr.first.second), pr.second);
			else
				rpairs.emplace_back(ordered_pair<char_id_t>(n, pr.first.second), interest_relation_contents(pr.second.tag_b, pr.second.tag_a));
		} else if (pr.first.second == old) {
			if (n.value >= pr.first.first.value)
				rpairs.emplace_back(ordered_pair<char_id_t>(pr.first.first, n), pr.second);
			else
				rpairs.emplace_back(ordered_pair<char_id_t>(pr.first.first, n), interest_relation_contents(pr.second.tag_b, pr.second.tag_a));
		}
	}
	std::sort(rpairs.begin(), rpairs.end(), [](IN(std::pair<ordered_pair<char_id_t>, interest_relation_contents>) a, IN(std::pair<ordered_pair<char_id_t>, interest_relation_contents>) b) {
		return a.first < b.first;
	});
	boost::range::remove_erase_if(interest_relations, [old](IN(std::pair<ordered_pair<char_id_t>, interest_relation_contents>) pr) {
		return static_cast<bool>(pr.first.first == old | pr.first.second == old);
	});
	interest_relations.insert(boost::container::ordered_unique_range, rpairs.begin(), rpairs.end());
}

void update_territory_lost(char_id_t id, IN(w_lock) l) noexcept {
	global::force_project_income(id, l);

	std::vector<char_id_t> current_neighbors;
	global::get_neighbors(id, current_neighbors, l);
	std::vector<char_id_t> to_discard;

	auto rg = interested_in.equal_range(id);
	for (; rg.first != rg.second; ++rg.first) {
		if (std::find(current_neighbors.begin(), current_neighbors.end(), rg.first->second) == current_neighbors.end()) {
			to_discard.push_back(rg.first->second);
		}
	}

	{
	const auto iend = interested_in.upper_bound(id);
	interested_in.erase(
		std::remove_if(interested_in.lower_bound(id), iend, [&current_neighbors](IN(std::pair<char_id_t, char_id_t>) p) {
			return std::find(current_neighbors.begin(), current_neighbors.end(), p.second) == current_neighbors.end();
		}),
		iend);
	}

	for (auto i : to_discard) {
		const auto iend = interested_in.upper_bound(i);
		interested_in.erase(
			std::remove_if(interested_in.lower_bound(i), iend, [id](IN(std::pair<char_id_t, char_id_t>) p) {
				return p.second == id;
			}),
		iend);
	}

	for (auto i : to_discard) {
		interest_relations.erase(ordered_pair<char_id_t>(id, i));
	}
}


void update_territory_gained(char_id_t id, IN(w_lock) l) noexcept {
	global::force_project_income(id, l);

	std::vector<char_id_t> current_neighbors;
	global::get_nearby_independant(id, current_neighbors, l);

	auto rg = interested_in.equal_range(id);
	for (; rg.first != rg.second; ++rg.first) {
		vector_erase(current_neighbors, rg.first->second);
	}

	admin_id_t pl = get_prime_leige(id, l);
	vector_erase_if(current_neighbors, [pl, &l](char_id_t c) { return pl != get_prime_leige(c, l);  });

	for (auto i : current_neighbors) {
		interested_in.emplace(id, i);
		interested_in.emplace(i, id);
	}

	interest_info_pack avals(id, l);
	const admin_id_t a_leige(get_prime_leige(id, l));

	for (auto i : current_neighbors) {
		interest_relations.emplace(ordered_pair<char_id_t>(id, char_id_t(i)), tags_from_ids(id, avals, a_leige, char_id_t(i), l));
	}
}

interest_relation_contents tags_from_ids(char_id_t a, IN(interest_info_pack) avals, admin_id_t a_leige, char_id_t b, IN(g_lock) l) noexcept {
	interest_relation_contents result;

	if (!global::is_vassal_of(a, b, l) && !global::is_vassal_of(b, a, l)) {
		INOUT(auto) aref = a < b ? result.tag_a : result.tag_b;
		INOUT(auto) bref = !(a < b) ? result.tag_a : result.tag_b;

		float total_a_mu = avals.mu + avals.d_mu;
		float total_a_simgasq = avals.sigmasq + avals.d_sigmasq;

		get_defensive_against_force_estimate(a, b, total_a_mu, total_a_simgasq, l);
	
		float base_b_mu = 0.0f;
		float base_b_simgasq = 0.0f;
		get_force_estimate(b, base_b_mu, base_b_simgasq, l);

		float total_b_mu = base_b_mu;
		float total_b_simgasq = base_b_simgasq;
		get_defensive_force_estimate(b, total_b_mu, total_b_simgasq, l);
		get_defensive_against_force_estimate(b, a, total_b_mu, total_b_simgasq, l);


		const float b_aggr = q_aggr_value(b, l);
		const admin_id_t b_leige(get_prime_leige(b, l));

	
		if ( avals.mu * avals.aggr > total_b_mu * 1.10f) {
			if (!(get_mutual_feeling(a, b, l) > 0) && !has_non_agression_pact(a, b, l))
				bref = interest_relation_contents::TAG_THREAT;
		}

		if ( base_b_mu * b_aggr > total_a_mu * 1.10f) {
			if (!(get_mutual_feeling(a, b, l) > 0) && !has_non_agression_pact(a, b, l))
				aref = interest_relation_contents::TAG_THREAT;
		}

		if ( (!valid_ids(b_leige) | b_leige == a_leige) & (avals.mu > total_b_mu * 1.10f)) {
			aref = interest_relation_contents::TAG_TARGET;
		} else if ( (!valid_ids(a_leige) | b_leige == a_leige) & (base_b_mu > total_a_mu * 1.10f)) {
			bref = interest_relation_contents::TAG_TARGET;
		}
	}

	return result;
}

void update_expectation_changed(char_id_t id, double ptsraised, IN(w_lock) l) noexcept {
	update_force_est(id, static_cast<float>(ptsraised), l);

	auto rg = interested_in.equal_range(id);

	interest_info_pack avals(id, l);
	const admin_id_t a_leige(get_prime_leige(id, l));

	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		it->second = tags_from_ids(id, avals, a_leige, rg.first->second, l);
	}
}

void update_defensive_pact_gained(char_id_t id, IN(w_lock) l) noexcept {
	auto rg = interested_in.equal_range(id);
	interest_info_pack avals(id, l);
	const admin_id_t a_leige(get_prime_leige(id, l));

	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		if (get_tag(id, *it) == interest_relation_contents::TAG_THREAT) {
			it->second = tags_from_ids(id, avals, a_leige, rg.first->second, l);
		}
	}
}

void update_defensive_pact_lost(char_id_t id, IN(w_lock) l) noexcept {
	auto rg = interested_in.equal_range(id);
	float mu = 0.0f;
	interest_info_pack avals(id, l);
	const admin_id_t a_leige(get_prime_leige(id, l));

	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		const auto tg = get_tag(id, *it);
		if (tg != interest_relation_contents::TAG_THREAT && tg != interest_relation_contents::TAG_TARGET) {
			it->second = tags_from_ids(id, avals, a_leige, rg.first->second, l);
		}
	}
}

float mu_estimate(char_id_t id, IN(g_lock) l) noexcept {
	// float mu, sigmasq = 0.0f;
	// get_force_estimate(id, mu, sigmasq, l);
	return with_udata(id, l, 0.0f, [](IN(udata) d) { return d.mu; });
}

float def_mu_estimate(char_id_t id, IN(g_lock) l) noexcept {
	float d_mu = 0.0f;
	float d_sigmasq = 0.0f;
	get_defensive_force_estimate(id, d_mu, d_sigmasq, l);
	return d_mu;
}

float def_against_mu_estimate(char_id_t id, char_id_t against, IN(g_lock) l) noexcept {
	float d_mu = 0.0f;
	float d_sigmasq = 0.0f;
	get_defensive_against_force_estimate(id, against, d_mu, d_sigmasq, l);
	return d_mu;
}

unsigned char interest_status_of(char_id_t a, char_id_t b, IN(g_lock) l) noexcept {
	auto it = interest_relations.find(ordered_pair<char_id_t>(a, b));
	if (it != interest_relations.end()) {
		if (it->first.first == a) {
			return it->second.tag_a;
		}
		return it->second.tag_b;
	}
	return interest_relation_contents::TAG_NONE;
}



void get_interest_totals(char_id_t id, INOUT(int) num_threats, INOUT(int) num_targets, INOUT(int) num_pthreats, IN(g_lock) l) noexcept {
	auto rg = interested_in.equal_range(id);
	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		const auto tg = get_tag(id, *it);
		
		if (tg == interest_relation_contents::TAG_THREAT)
			++num_threats;
		else if (tg == interest_relation_contents::TAG_TARGET)
			++num_targets;
		else if (tg == interest_relation_contents::TAG_PTHREAT)
			++num_pthreats;
	}
}

unsigned int total_threats(char_id_t id, IN(g_lock) l) noexcept {
	unsigned int total = 0;
	auto rg = interested_in.equal_range(id);
	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		if(get_tag(id, *it) == interest_relation_contents::TAG_THREAT)
			++total;
	}
	return total;
}

constexpr size_t inner_sv_size = 16;

char_id_t get_random_threat(char_id_t id, IN(g_lock) l) noexcept {
	small_vector<char_id_t, inner_sv_size, concurrent_allocator<char_id_t>> threats;
	auto rg = interested_in.equal_range(id);
	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		if (get_tag(id, *it) == interest_relation_contents::TAG_THREAT)
			threats.push_back(it->first.get_other(id));
	}
	if (threats.size() == 0)
		return char_id_t();
	return threats[global_store.get_fast_int() % threats.size()];
}

char_id_t get_random_target(char_id_t id, IN(g_lock) l) noexcept {
	small_vector<char_id_t, inner_sv_size, concurrent_allocator<char_id_t>> targets;
	auto rg = interested_in.equal_range(id);
	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		if (get_tag(id, *it) == interest_relation_contents::TAG_TARGET)
			targets.push_back(it->first.get_other(id));
	}
	if (targets.size() == 0)
		return char_id_t();
	return targets[global_store.get_fast_int() % targets.size()];
}

float max_threat_percentage(char_id_t id, IN(g_lock) l) noexcept {
	auto rg = interested_in.equal_range(id);
	float maxmu = 0.0f;
	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		if (get_tag(id, *it) == interest_relation_contents::TAG_THREAT) {
			maxmu = std::max(maxmu, mu_estimate(rg.first->second, l));
		}
	}

	return maxmu / (maxmu + mu_estimate(id, l) + def_mu_estimate(id,l));
}

std::pair<float, float> max_p_threat_percentage(char_id_t id, float minus_defensive, IN(g_lock) l) noexcept {
	auto rg = interested_in.equal_range(id);
	float maxmu = 0.0f;
	float maxpmu = 0.0f;
	for (; rg.first != rg.second; ++rg.first) {
		auto it = interest_relations.find(ordered_pair<char_id_t>(rg.first->second, id));
		const auto omu = mu_estimate(rg.first->second, l);
		if (get_tag(id, *it) == interest_relation_contents::TAG_THREAT) {
			maxmu = std::max(maxmu, omu);
		}
		maxpmu = std::max(maxpmu, omu);
	}
	const auto self_estimate = mu_estimate(id, l);
	const auto def_estimate = def_mu_estimate(id, l);
	return std::make_pair(maxmu / (maxmu + self_estimate + def_estimate), maxpmu / (maxpmu + self_estimate + def_estimate - minus_defensive));
}


void interest_f_generate(IN_P(sqlite3) db) noexcept {}

void interest_f_load(IN_P(sqlite3) db, IN(w_lock) l) noexcept {
	init_interests(l);
}

void interest_f_save(IN_P(sqlite3) db, IN(r_lock) l) noexcept {}
