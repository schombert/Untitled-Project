#include "globalhelpers.h"
#include "relations.h"
#include "structs.hpp"
#include "traits.h"
#include "pacts.h"
#include "datamanagement.hpp"
#include "living_data.h"
#include "reputation.h"
#include "court.h"


namespace global {
	multiindex<char_id_t, unsigned int> chtorelations;
	multiindex<ordered_pair<char_id_t>, unsigned int> pairtorelations;
	v_pool<relation> relations_pool;

	multiindex<char_id_t, char_id_t> ch_to_feeling_relations;
	single_index_t<ordered_pair<char_id_t>, feeling_data> feeling_relations;
};

int get_feeling_rawv(char_id_t prim, char_id_t secon, IN(g_lock) l) noexcept {
	int feeling = 0;
	const auto pr = ordered_pair<char_id_t>(prim, secon);

	const auto it = global::feeling_relations.find(pr, l);
	if (it != global::feeling_relations.end(l)) {
		IN(auto) rel = it->second;

		if (prim < secon) {
			if (PRIM_HATE(rel.flags))
				feeling = -R_THRESHOLD;
			else if (PRIM_LIKE(rel.flags))
				feeling = R_THRESHOLD;
			else
				feeling = rel.ptrend;
		} else {
			if (SECON_HATE(rel.flags))
				feeling = -R_THRESHOLD;
			else if (SECON_LIKE(rel.flags))
				feeling = R_THRESHOLD;
			else
				feeling = rel.strend;
		}
	}	
	return feeling;
}

// lesser owes greater = positive value,
// greater owes lesser = negative value
int owed_favors(char_id_t favors_from, char_id_t favors_to, IN(g_lock) l) noexcept {
	const auto pr = ordered_pair<char_id_t>(favors_from, favors_to);
	const auto it = global::feeling_relations.find(pr, l);
	if (it != global::feeling_relations.end(l)) {
		if (favors_from < favors_to)
			return it->second.favors;
		return -it->second.favors;
	}
	return 0;
}

void add_favor(char_id_t favors_from, char_id_t favors_to, IN(w_lock) l) noexcept {
	const auto pr = ordered_pair<char_id_t>(favors_from, favors_to);
	auto it = global::feeling_relations.find(pr, l);
	if (it == global::feeling_relations.end(l)) {
		global::ch_to_feeling_relations.insert(favors_from, favors_to, l);
		global::ch_to_feeling_relations.insert(favors_to, favors_from, l);
		it = global::feeling_relations.emplace(pr, l).first;
	}

	if (favors_from < favors_to) {
		it->second.favors = static_cast<decltype(it->second.favors)>(std::min(it->second.favors + 1, static_cast<int>(max_value<decltype(it->second.favors)>::value)));
	} else {
		it->second.favors = static_cast<decltype(it->second.favors)>(std::max(it->second.favors - 1, static_cast<int>(min_value<decltype(it->second.favors)>::value)));
	}
}

bool call_in_favor(char_id_t favors_from, char_id_t favors_to, IN(w_lock) l) noexcept {
	const auto pr = ordered_pair<char_id_t>(favors_from, favors_to);
	const auto it = global::feeling_relations.find(pr, l);
	if (it == global::feeling_relations.end(l)) {
		return false;
	}
	if (favors_from < favors_to) {
		if (it->second.favors > 0) {
			it->second.favors -= 1;
			return true;
		}
		return false;
	} else {
		if (it->second.favors < 0) {
			it->second.favors += 1;
			return true;
		} 
		return false;
	}
}

double dishonor_favor_cost(char_id_t favor_from, char_id_t favor_to, IN(g_lock) l) noexcept {
	const auto pr = ordered_pair<char_id_t>(favor_from, favor_to);
	const auto it = global::feeling_relations.find(pr, l);
	if (it == global::feeling_relations.end(l)) {
		return 0.0;
	}
	if (favor_from < favor_to) {
		if (it->second.favors > 0) {
			return with_udata_force(favor_from, l, [](IN(udata) d) { return d.p_honorable - update_reputation(reputation::p_dishonor_favor_reliable, reputation::p_dishonor_favor_unreliable, d.p_honorable); });
		}
	} else {
		if (it->second.favors < 0) {
			return with_udata_force(favor_from, l, [](IN(udata) d) { return d.p_honorable - update_reputation(reputation::p_dishonor_favor_reliable, reputation::p_dishonor_favor_unreliable, d.p_honorable); });
		}
	}
}

void dishonor_favor(char_id_t favor_from, char_id_t favor_to, IN(w_lock) l) noexcept {
	const auto pr = ordered_pair<char_id_t>(favor_from, favor_to);
	const auto it = global::feeling_relations.find(pr, l);
	if (it == global::feeling_relations.end(l)) {
		return;
	}
	if (favor_from < favor_to) {
		if (it->second.favors > 0) {
			it->second.favors -= 1;
			with_udata(favor_from, l, [](INOUT(udata) d) { d.p_honorable = update_reputation(reputation::p_dishonor_favor_reliable, reputation::p_dishonor_favor_unreliable, d.p_honorable); });
			adjust_relation(favor_to, favor_from, -1, l);
		}
	} else {
		if (it->second.favors < 0) {
			it->second.favors += 1;
			with_udata(favor_from, l, [](INOUT(udata) d) { d.p_honorable = update_reputation(reputation::p_dishonor_favor_reliable, reputation::p_dishonor_favor_unreliable, d.p_honorable); });
			adjust_relation(favor_to, favor_from, -1, l);
		}
	}
}

constexpr double favor_usefulness_factor = 0.5;

double value_of_favor_from(char_id_t favor_to, IN(udata) td, char_id_t favor_from, IN(g_lock) l) noexcept {
	if (!valid_ids(global::get_prime_title(favor_to))) {
		// case: favor to is some courtier
		const auto prmlg = td.a_court;
		if (prmlg == get_prime_leige(favor_from, l) || prmlg == get_court(favor_from, l)) {
			const auto prob_honor_favor = with_udata(favor_from, l, 0.0, [](IN(udata) d) { return (1.0 - reputation::p_dishonor_favor_reliable) * d.p_honorable + (1.0 - reputation::p_dishonor_favor_unreliable) * (1.0 - d.p_honorable); });
			const auto court_power = power_in_administration(prmlg, favor_from, l);
			return court_power * prob_honor_favor * favor_usefulness_factor;
		}
	} else {
		// case: favor to is a title holder
		const auto prmlg = get_prime_leige(favor_to, l);
		const auto own_court = get_prime_admin(favor_to, l);

		const auto from_leige = get_prime_leige(favor_from, l);
		const auto from_court = get_court(favor_from, l);

		if ((prmlg == from_leige) | (prmlg == from_court)) {
			// case: both in court of common leige
			const auto prob_honor_favor = with_udata(favor_from, l, 0.0, [](IN(udata) d) { return (1.0 - reputation::p_dishonor_favor_reliable) * d.p_honorable + (1.0 - reputation::p_dishonor_favor_unreliable) * (1.0 - d.p_honorable); });
			const auto court_power = power_in_administration(prmlg, favor_from, l);
			return court_power * prob_honor_favor * favor_usefulness_factor;
		} else if ((own_court == from_leige) | (own_court == from_court)) {
			// case: favor from in court of favor to
			const auto prob_honor_favor = with_udata(favor_from, l, 0.0, [](IN(udata) d) { return (1.0 - reputation::p_dishonor_favor_reliable) * d.p_honorable + (1.0 - reputation::p_dishonor_favor_unreliable) * (1.0 - d.p_honorable); });
			const auto court_power = power_in_administration(own_court, favor_from, l);
			return court_power * prob_honor_favor * favor_usefulness_factor;
		}
	}


	
	return 0.0;
}

float base_opinon_as_independant(char_id_t from, IN(udata) fd, char_id_t to, IN(g_lock) l) noexcept {
	return with_udata(to, l, 0.0f, [from, to, &fd, &l](IN(udata) td) noexcept {
		const float feeling = (is_emotional(fd) ? 1.2f : 1.0f) * static_cast<float>(get_feeling_rawv(from, to, l)) / static_cast<float>(R_THRESHOLD);
		const float attribbias = (is_kind(td) ? 0.1f : 0.0f) + (is_just(td) ? 0.1f : 0.0f) + (is_cruel(td) ? -0.1f : 0.0f);

		IN(auto) f = get_object(from);
		IN(auto) t = get_object(to);

		const float relcul = (fd.culture == td.culture ? 0.2f : -0.2f) + (fd.religion == td.religion ? 0.2f : -0.5f);
		const float family = (global::are_close_family(from, to) ? 0.5f : 0.0f) + (((f.dynasty == t.dynasty) & valid_ids(f.dynasty)) ? 0.2f : 0.0f);

		const float reputation = static_cast<float>((td.p_peaceful - 0.5) + (td.p_honorable - 0.5)) * 0.5f;

		const float social_bias = (static_cast<float>(td.stats.social) - 3.0f) / 12.0f;

		return (feeling + attribbias + relcul + family + reputation + social_bias) * 1.4f;
	});
}

float base_opinon_as_vassal(char_id_t from, IN(udata) fd, char_id_t to, IN(g_lock) l) noexcept {
	return with_udata(to, l, 0.0f, [from, to, &fd, &l](IN(udata) td) noexcept {
		const float feeling = (is_emotional(fd) ? 1.2f : 1.0f) * static_cast<float>(get_feeling_rawv(from, to, l)) / static_cast<float>(R_THRESHOLD);
		const float attribbias = (is_kind(td) ? 0.1f : 0.0f) + (is_just(td) ? 0.1f : 0.0f) + (is_cruel(td) ? -0.1f : 0.0f);

		IN(auto) f = get_object(from);
		IN(auto) t = get_object(to);

		const float relcul = (fd.culture == td.culture ? 0.2f : -0.2f) + (fd.religion == td.religion ? 0.2f : -0.5f);
		const float family = (global::are_close_family(from, to) ? 0.5f : 0.0f) + (((f.dynasty == t.dynasty) & valid_ids(f.dynasty)) ? 0.2f : 0.0f);

		const float reputation = static_cast<float>((td.p_just - 0.5) + (td.p_honorable - 0.5)) * 0.5f;

		const float social_bias = (static_cast<float>(td.stats.social) - 3.0f) / 12.0f;

		return (feeling + attribbias + relcul + family + reputation + social_bias) * 1.4f;
	});
}

float base_opinon(char_id_t from, IN(udata) fdata, char_id_t to, IN(g_lock) l) noexcept {
	const auto padm = get_prime_admin(from, l);

	if (valid_ids(padm)) {
		IN(auto) pa = get_object(padm, l);
		if (valid_ids(pa.leige)) {
			IN(auto) lg = get_object(pa.leige, l);
			if ((lg.executive == to) | (get_object(lg.associated_title).holder == to))
				return base_opinon_as_vassal(from, fdata, to, l);
		}
	} else {
		if (valid_ids(fdata.a_court)) {
			IN(auto) crt = get_object(fdata.a_court, l);
			if ((crt.executive == to) | (get_object(crt.associated_title).holder == to))
				return base_opinon_as_vassal(from, fdata, to, l);
		}
	}

	return base_opinon_as_independant(from, fdata, to, l);
}

float base_opinon_as_generic(prov_id_t source, char_id_t to, IN(g_lock) l) noexcept {
	return with_udata(to, l, 0.0f, [source, to](IN(udata) td) noexcept {
		const float attribbias = (is_kind(td) ? 0.1f : 0.0f) + (is_just(td) ? 0.1f : 0.0f) + (is_cruel(td) ? -0.1f : 0.0f);

		IN(auto) prov = get_object(source);
		IN(auto) t = get_object(to);

		const float relcul = (prov.culture == td.culture ? 0.2f : -0.2f) + (prov.religion == td.religion ? 0.2f : -0.5f);

		const float reputation = static_cast<float>((td.p_just - 0.5) + (td.p_honorable - 0.5)) * 0.5f;

		const float social_bias = (static_cast<float>(td.stats.social) - 3.0f) / 12.0f;

		return (attribbias + relcul + reputation + social_bias) * 1.4f;
	});
}

float opinion(char_id_t from, IN(udata) fdata, char_id_t to, IN(g_lock) l) noexcept {
	return tanh(base_opinon(from, fdata, to, l));
}

float opinion(char_id_t from, char_id_t to, IN(g_lock) l) noexcept {
	return with_udata(from, l, 0.0f, [from, to, &l](IN(udata) u) noexcept { return opinion(from, u, to, l); });
}

double bias_by_envoy(char_id_t source, char_id_t envoy, char_id_t destination, IN(g_lock) l) noexcept {
	return with_udata_2(destination, envoy, l, 0.0, [destination, envoy, source, &l](IN(udata) dd, IN(udata) ed) noexcept {
		const double emotional_mul = is_emotional(dd) ? 1.5 : is_measured(dd) ? 0.7 : 1.0;
		const static int differences = (ed.culture == dd.culture ? 0 : -((R_THRESHOLD *MAX_TRAITS) / 2)) + (ed.religion == dd.religion ? 0 : -((R_THRESHOLD *MAX_TRAITS) / 2));
		return emotional_mul *
			static_cast<double>(get_feeling_rawv(destination, source, l) * 2 * MAX_TRAITS +
				get_feeling_rawv(destination, envoy, l) * MAX_TRAITS +
				differences +
				similarity_score(destination, envoy, l) * R_THRESHOLD) /
			static_cast<double>(MAX_TRAITS * R_THRESHOLD * 2);
	});
}

void clear_finished_pacts(IN(w_lock) l) noexcept {
	std::vector<size_t> toerase;
	global::relations_pool.for_each(l, [&toerase, &l](INOUT(relation) r) noexcept {
		if (r.type == REL_PACT && global::pacts.get(static_cast<pact_id>(r.typedata.id), l).pact_type == pact_data::NONE) {
			const auto i = global::relations_pool.get_index(r, l);
			toerase.push_back(i);
			global::relations_pool.free(i,l);
		}
	});
	if (toerase.size() > 0) {
		global::chtorelations.erase_if(l, [&toerase](IN(std::pair<char_id_t, unsigned int>) p) noexcept {
			return std::find(toerase.begin(), toerase.end(), p.second) != toerase.end();
		});
		global::pairtorelations.erase_if(l, [&toerase](IN(std::pair<ordered_pair<char_id_t>, unsigned int>) p) noexcept {
			return std::find(toerase.begin(), toerase.end(), p.second) != toerase.end();
		});
	}
}

void clear_finished_pact(INOUT(pact_data) pact, IN(w_lock) l) noexcept {
	decltype(global::relations_pool)::tag_type rtag = global::relations_pool.tag_max;
	const auto pid = global::pacts.get_index(pact, l);
	ordered_pair<char_id_t> ppair(pact.a, pact.b);

	if(global::pairtorelations.for_each_breakable(ppair, l, [&l, pid, &rtag](unsigned int indx) noexcept {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PACT && r.typedata.id == pid) {
			rtag = indx;
			global::relations_pool.free(indx, l);
			return true;
		}
		return false;
	})) {
		global::chtorelations.erase(pact.a, rtag, l);
		global::chtorelations.erase(pact.b, rtag, l);
		global::pairtorelations.erase(ppair, rtag, l);
	}
}

void add_relation(IN(relation) r,  IN(w_lock) l) noexcept {
	const auto indx = global::relations_pool.add(r, l);
	global::chtorelations.insert(r.primary, indx, l);
	global::chtorelations.insert(r.secondary, indx, l);
	global::pairtorelations.insert(ordered_pair<char_id_t>(r.primary, r.secondary), indx, l);
}

void add_relation(relation&& r,  IN(w_lock) l) noexcept {
	const auto p = r.primary;
	const auto s = r.secondary;
	const auto indx = global::relations_pool.add(std::move(r), l);
	global::chtorelations.insert(p, indx, l);
	global::chtorelations.insert(s, indx, l);
	global::pairtorelations.insert(ordered_pair<char_id_t>(p,s), indx, l);
}

void remove_all_relations(char_id_t c,  IN(w_lock) l) noexcept {
	std::vector<std::pair<char_id_t, unsigned int>> toerase;
	toerase.reserve(16);
	
	global::chtorelations.for_each(c, l, [c, &l, &toerase](unsigned int indx) noexcept {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.primary == c)
			toerase.emplace_back(r.secondary, indx);
		else
			toerase.emplace_back(r.primary, indx);
	});
	
	
	for (IN(auto) pr : toerase) {
		global::chtorelations.erase(pr.first, pr.second, l);
		global::pairtorelations.eraseall(ordered_pair<char_id_t>(c, pr.first), l);
	}

	global::chtorelations.eraseall(c, l);

	std::vector<ordered_pair<char_id_t>> to_erase_b;
	global::ch_to_feeling_relations.erase_if(l, [&to_erase_b, c](IN(std::pair<char_id_t, char_id_t>) pr) {
		if (pr.first == c) {
			to_erase_b.emplace_back(c, pr.second);
			return true;
		}
		return pr.second == c;
	});
	for (IN(auto) i : to_erase_b) {
		global::feeling_relations.erase(i, l);
	}
	
}

int get_feeling(char_id_t prim, char_id_t secon,  IN(g_lock) l) noexcept {
	int feeling = 0;

	const auto pr = ordered_pair<char_id_t>(prim, secon);
	const auto it = global::feeling_relations.find(pr, l);
	if (it != global::feeling_relations.end(l)) {
		IN(auto) rel = it->second;

		if (prim < secon) {
			if (PRIM_HATE(rel.flags))
				feeling = -1;
			else if (PRIM_LIKE(rel .flags))
				feeling = 1;
		} else {
			if (SECON_HATE(rel.flags))
				feeling = -1;
			else if (SECON_LIKE(rel.flags))
				feeling = 1;
		}
	}

	return feeling;
}

int get_mutual_feeling(char_id_t a, char_id_t b,  IN(g_lock) l) noexcept {
	int feeling = 0;

	const auto pr = ordered_pair<char_id_t>(a, b);
	const auto it = global::feeling_relations.find(pr, l);
	if (it != global::feeling_relations.end(l)) {
		IN(auto) rel = it->second;
		if (BOTH_LIKE(rel.flags)) {
			feeling = 1;
		} else if (BOTH_HATE(rel.flags)) {
			feeling = -1;
		}
	}

	return feeling;
}

char_id_t get_random_hated_or_enemy(char_id_t c, IN(g_lock) l) noexcept {
	small_vector<char_id_t, 16, concurrent_allocator<char_id_t>> lst;
	enum_hated_or_enemies(c, l, [&lst](char_id_t id) { lst.push_back(id); });
	if (lst.size() == 0)
		return char_id_t();
	return lst[global_store.get_fast_int() % lst.size()];
}

#define IN_THRESHOLD(x) (x < R_THRESHOLD && x > -R_THRESHOLD)
#define CONSTRAIN(x, y) if(x < -y) x = -y; else if(x > y) x = y;
#define PROB_TOTAL (4 * R_THRESHOLD * MAX_TRAITS)
#define SUM_PROB(value, t, s) (R_THRESHOLD * MAX_TRAITS * static_cast<int>(value) + (3 * MAX_TRAITS * static_cast<int>(t)) / 4 + (3 * R_THRESHOLD * s) / 4)

bool adjust(__int8 value, __int8& trend, int similarity) noexcept {
	if (value > 0 && trend >= R_THRESHOLD) return true;
	if (value < 0 && trend <= -R_THRESHOLD) return true;

	const int p = SUM_PROB(value, trend, similarity);
	trend += value;
	if (value > 0 && p <= 0) return false;
	if (value < 0 && p >= 0) return false;

	const int v = global_store.get_fast_int() % PROB_TOTAL;
	return v < abs(p);
}

void adjust_relation_symmetric(char_id_t prim, char_id_t secon, __int8 value,  IN(w_lock) l) noexcept {
	const auto pr = ordered_pair<char_id_t>(prim, secon);
	auto it = global::feeling_relations.find(pr, l);
	if (it == global::feeling_relations.end(l)) {
		global::ch_to_feeling_relations.insert(prim, secon, l);
		global::ch_to_feeling_relations.insert(secon, prim, l);
		it = global::feeling_relations.emplace(pr, l).first;
	}
	INOUT(auto) rel = it->second;

	bool initial_display = static_cast<bool>(BOTH_LIKE(rel.flags) | BOTH_HATE(rel.flags));

	const int sim = similarity_score(prim, secon, l);

	if (adjust(value, rel.ptrend, sim)) {
		if (PRIM_HATE(rel .flags)) {
			if (value > 0)
				SET_PRIM_INDIF(rel.flags);
		} else if (PRIM_INDIF(rel.flags)) {
			if (value > 0)
				SET_PRIM_LIKE(rel.flags);
			else
				SET_PRIM_HATE(rel.flags);
		} else {
			if (value < 0)
				SET_PRIM_INDIF(rel.flags);
		}
	}

	if (adjust(value, rel.strend, sim)) {
		if (SECON_HATE(rel.flags)) {
			if (value > 0)
				SET_SECON_INDIF(rel.flags);
		} else if (SECON_INDIF(rel.flags)) {
			if (value > 0)
				SET_SECON_LIKE(rel.flags);
			else
				SET_SECON_HATE(rel.flags);
		} else {
			if (value < 0)
				SET_SECON_INDIF(rel.flags);
		}
	}

	bool final_display = static_cast<bool>(BOTH_LIKE(rel.flags) | BOTH_HATE(rel.flags));

	if ((final_display != initial_display) && (global::interested.in_set(prim.value) || global::interested.in_set(secon.value))) {
		make_relationship_announcement(prim.value, secon.value, BOTH_HATE(rel.flags), !initial_display);
	}
}

void adjust_relation(char_id_t prim, char_id_t secon, __int8 value,  IN(w_lock) l) noexcept {

	const auto pr = ordered_pair<char_id_t>(prim, secon);
	auto it = global::feeling_relations.find(pr, l);
	if (it == global::feeling_relations.end(l)) {
		global::ch_to_feeling_relations.insert(prim, secon, l);
		global::ch_to_feeling_relations.insert(secon, prim, l);
		it = global::feeling_relations.emplace(pr, l).first;
	}
	INOUT(auto) rel = it->second;

	bool initial_display = static_cast<bool>(BOTH_LIKE(rel.flags) | BOTH_HATE(rel.flags));
	const int sim = similarity_score(prim, secon, l);

	if (prim < secon) {
		if (adjust(value, rel.ptrend, sim)) {
			if (PRIM_HATE(rel.flags)) {
				if (value > 0)
					SET_PRIM_INDIF(rel.flags);
			} else if (PRIM_INDIF(rel.flags)) {
				if (value > 0)
					SET_PRIM_LIKE(rel.flags);
				else
					SET_PRIM_HATE(rel.flags);
			} else {
				if (value < 0)
					SET_PRIM_INDIF(rel.flags);
			}
		}
	} else {
		if (adjust(value, rel.strend, sim)) {
			if (SECON_HATE(rel.flags)) {
				if (value > 0)
					SET_SECON_INDIF(rel.flags);
			} else if (SECON_INDIF(rel.flags)) {
				if (value > 0)
					SET_SECON_LIKE(rel.flags);
				else
					SET_SECON_HATE(rel.flags);
			} else {
				if (value < 0)
					SET_SECON_INDIF(rel.flags);
			}
		}
	}

	bool final_display = static_cast<bool>(BOTH_LIKE(rel.flags) | BOTH_HATE(rel.flags));

	if ((final_display != initial_display) && (global::interested.in_set(prim.value) || global::interested.in_set(secon.value))) {
		make_relationship_announcement(prim.value, secon.value, BOTH_HATE(rel.flags), !initial_display);
	}
}

unsigned int peace_until(char_id_t a, char_id_t b,  IN(g_lock) l) noexcept {
	unsigned int date = 0;
	global::pairtorelations.for_each_breakable(ordered_pair<char_id_t>(a, b), l, [&date, &l, a](unsigned int indx) noexcept {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PEACE && r.typedata.peace.date > global::currentday && a == r.primary) {
			date = r.typedata.peace.date;
			return true;
		}
		return false;
	});
	return date;
}

bool have_peace(char_id_t a, char_id_t b, IN(g_lock) l) noexcept {
	return global::pairtorelations.for_each_breakable(ordered_pair<char_id_t>(a, b), l, [ &l, a](unsigned int indx) noexcept {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PEACE && r.typedata.peace.date > global::currentday && a == r.primary) {
			return true;
		} else if (r.type == REL_PACT && global::pacts.get(static_cast<pact_id>(r.typedata.id), l).pact_type == P_NON_AGRESSION) {
			return true;
		}
		return false;
	});
}

bool adjust_for_attack(char_id_t a, char_id_t b, IN(w_lock) l) noexcept {
	bool found = false;
	global::pairtorelations.for_each(ordered_pair<char_id_t>(a, b), l, [&found, &l, a](unsigned int indx) noexcept {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PEACE && r.typedata.peace.date > global::currentday && a == r.primary) {
			found = true;
			with_udata(a, l, [](INOUT(udata) adat) noexcept {
				adat.p_honorable = update_reputation(reputation::p_dishonor_reliable, reputation::p_dishonor_unreliable, adat.p_honorable);
			});
		} else if (r.type == REL_PACT) {
			INOUT(auto) pact = global::pacts.get(static_cast<pact_id>(r.typedata.id), l);
			if (pact.pact_type == P_NON_AGRESSION && pact.guarantees.size() != 0) {
				found = true;
				break_pact(pact, a, l);
			}
		}
	});
	return found;
}

double honor_cost_for_attack(char_id_t a, char_id_t b, IN(g_lock) l) {
	double cost = 0.0;
	global::pairtorelations.for_each(ordered_pair<char_id_t>(a, b), l, [&cost, &l, a](unsigned int indx) noexcept {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PEACE && r.typedata.peace.date > global::currentday && a == r.primary) {
			cost += with_udata(a, l, 0.0, [](INOUT(udata) adat) noexcept {
				return adat.p_honorable - update_reputation(reputation::p_dishonor_reliable, reputation::p_dishonor_unreliable, adat.p_honorable);
			});
		} else if (r.type == REL_PACT) {
			INOUT(auto) pact = global::pacts.get(static_cast<pact_id>(r.typedata.id), l);
			if (pact.pact_type == P_NON_AGRESSION && pact.guarantees.size() != 0) {
				cost += honor_loss_on_break_val( pact, a, l);
			}
		}
	});
	return cost;
}

void add_peace_treaty(char_id_t a, char_id_t b, unsigned int until, IN(w_lock) l) noexcept {
	if(!global::pairtorelations.for_each_breakable(ordered_pair<char_id_t>(a, b), l, [ &l, until, a](unsigned int indx) noexcept {
		INOUT(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PEACE && a == r.primary) {
			if (r.typedata.peace.date < until)
				r.typedata.peace.date = until;
			return true;
		}
		return false;
	})) {
		relation r;
		r.type = REL_PEACE;
		r.primary = a;
		r.secondary = b;
		r.typedata.peace.date = until;
		add_relation(std::move(r), l);
	}
}
