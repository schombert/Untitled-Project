#include "globalhelpers.h"
#include "laws.h"
#include "datamanagement.hpp"
#include "structs.hpp"
#include "envoys.h"
#include "relations.h"
#include "living_data.h"

multiindex<title_id_t, sub_title> sub_titles;
multiindex<char_id_t, r_sub_title> r_sub_titles;
multiindex<admin_id_t, concurrent_uniq<envoy_mission>> envoy_missions;
multiindex<admin_id_t, SM_TAG_TYPE> spy_missions;
multiindex<admin_id_t, admin_id_t> leigetoadmin;

admin_id_t get_associated_admin(char_id_t id, IN(g_lock) l) noexcept {
	const auto pt = get_object(id).primetitle;
	// if (valid_ids(pt)) {
		return get_object(pt).associated_admin;
	// }
	// return admin_id_t();
}

void create_sub_title(title_id_t parent, sub_title_type type, char_id_t holder, unsigned char title_flags, IN(w_lock) l) noexcept {
	sub_titles.insert(parent, sub_title{holder, type, title_flags}, l);
	r_sub_titles.insert(holder, r_sub_title{parent, type}, l);
}

void transfer_sub_title(title_id_t parent, sub_title_type type, char_id_t oldholder, char_id_t newholder, IN(w_lock) l) noexcept {
	sub_titles.for_each_breakable(parent, l, [oldholder, newholder](INOUT(sub_title) t) {
		if (t.holder == oldholder) {
			t.holder = newholder;
			return true;
		}
		return false;
	});
	r_sub_titles.range_erase_if(oldholder, l, [type, parent](IN(std::pair<char_id_t, r_sub_title>) t) { return t.second.type == type && t.second.parent == parent; });
	r_sub_titles.insert(newholder, r_sub_title{parent, type}, l);
}

void destroy_sub_title(title_id_t parent, sub_title_type type, char_id_t holder, IN(w_lock) l) noexcept {
	sub_titles.range_erase_if(parent, l, [type, holder](IN(std::pair<title_id_t, sub_title>) t) { return t.second.type == type && t.second.holder == holder; });
	r_sub_titles.range_erase_if(holder, l, [type, parent](IN(std::pair<char_id_t, r_sub_title>) t) { return t.second.type == type && t.second.parent == parent; });
}

template<typename T, typename L>
void _list_direct_controlled_admin(char_id_t id, INOUT(flat_set<admin_id_t, std::less<admin_id_t>, T>) s, IN(L) l) noexcept {
	global::holdertotitle.for_each(id, l, [&s](title_id_t t) noexcept {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			s.insert(to.associated_admin);
		}
	});
}

template<typename T, typename L>
void _list_direct_controlled_admin(char_id_t id, INOUT(std::vector<admin_id_t, T>) s, IN(L) l) noexcept {
	global::holdertotitle.for_each(id, l, [&s](title_id_t t) noexcept {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			s.push_back(to.associated_admin);
		}
	});
}

void list_direct_controlled_admin(char_id_t id, INOUT(flat_set<admin_id_t>) s, IN(w_lock) l) noexcept {
	_list_direct_controlled_admin(id, s, l);
}
void list_direct_controlled_admin(char_id_t id, INOUT(std::vector<admin_id_t>) s, IN(w_lock) l) noexcept {
	_list_direct_controlled_admin(id, s, l);
}
void list_direct_controlled_admin(char_id_t id, INOUT(cflat_set<admin_id_t>) s, IN(g_lock) l) noexcept {
	_list_direct_controlled_admin(id, s, l);
}
void list_direct_controlled_admin(char_id_t id, INOUT(cvector<admin_id_t>) s, IN(g_lock) l) noexcept {
	_list_direct_controlled_admin(id, s, l);
}

char_id_t get_sub_title_holder(title_id_t parent, sub_title_type st, IN(g_lock) l) noexcept {
	char_id_t holder;
	sub_titles.for_each_breakable(parent, l, [st, &holder](IN(sub_title) t) {
		if (t.type == st) {
			holder = t.holder;
			return true;
		}
		return false;
	});
	return holder;
}

template<typename T, typename L>
void _get_sub_title_holders(INOUT(std::vector<char_id_t, T>) v, title_id_t parent, sub_title_type st, IN(L) l) noexcept {
	sub_titles.for_each(parent, l, [st, &v](IN(sub_title) t) {
		if (t.type == st) {
			v.emplace_back(t.holder);
		}
	});
}

void get_sub_title_holders(INOUT(std::vector<char_id_t>) v, title_id_t parent, sub_title_type st, IN(w_lock) l) noexcept {
	_get_sub_title_holders(v, parent, st, l);
}

void get_sub_title_holders(INOUT(cvector<char_id_t>) v, title_id_t parent, sub_title_type st, IN(g_lock) l) noexcept {
	_get_sub_title_holders(v, parent, st, l);
}

v_pool_t<administration, admin_id> admin_pool;

administration& new_admin(INOUT(admin_id_t) id, IN(w_lock) l) noexcept {
	id = admin_pool.emplace(l);
	return admin_pool.get(id.value, l);
}

admin_id_t new_admin_id(IN(w_lock) l) noexcept {
	return admin_id_t(admin_pool.emplace(l));
}

void administration::random_administration(title_id_t at, prov_id_t cap, cul_id_t culture, rel_id_t religion, admin_id_t leige_id, IN(w_lock) l) noexcept {
	associated_title = at;
	INOUT(auto) to = get_object(at);
	executive = to.holder;
	to.associated_admin = get_id(*this, l);
	
	capital = cap;

	protections = 0;
	court_culture = culture;
	official_religion = religion;

	if (global_store.get_float() >= 0.25f)
		legislative = administration::BY_VOTE;
	else
		legislative = administration::EXECUTIVE;

	war = administration::EXECUTIVE;

	if (global_store.get_float() >= 0.10f)
		judgement = administration::BY_VOTE;
	else
		judgement = administration::EXECUTIVE;

	leige = leige_id;
	if(valid_ids(leige_id)) {
		IN(auto) lg = get_object(leige_id, l);
		vassal_flags = lg.standard_vassal_flags;
		vassal_req = lg.standard_vassal_req;
		leigetoadmin.insert(leige_id, admin_id_t(admin_pool.get_index(*this, l)), l);
	} else {
		vassal_flags = 0;
		vassal_req = 0;
	}

	const float fv1 = global_store.get_float();
	if (fv1 <= .1f)
		standard_vassal_req = administration::TAX_MINIMUM;
	else if (fv1 <= .3f)
		standard_vassal_req = administration::TAX_LOW;
	else if (fv1 <= .7f)
		standard_vassal_req = administration::TAX_MEDIUM;
	else if (fv1 <= .9f)
		standard_vassal_req = administration::TAX_HIGH;
	else
		standard_vassal_req = administration::TAX_MAXIMUM;

	const float fv2 = global_store.get_float();
	if (fv2 <= .1f)
		standard_vassal_req |= administration::TROOP_MINIMUM;
	else if (fv2 <= .3f)
		standard_vassal_req |= administration::TROOP_LOW;
	else if (fv2 <= .7f)
		standard_vassal_req |= administration::TROOP_MEDIUM;
	else if (fv2 <= .9f)
		standard_vassal_req |= administration::TROOP_HIGH;
	else
		standard_vassal_req |= administration::TROOP_MAXIMUM;

	standard_vassal_flags = 0;

	const float fv3 = global_store.get_float();
	if (fv3 <= .3f)
		title_flags = inheritance::PARTIBLE;
	else if (fv3 <= .7f)
		title_flags = inheritance::PRIMOGENITURE;
	else
		title_flags = inheritance::ELECTIVE;

	title_flags |= inheritance::MALE;

	const float fv4 = global_store.get_float();
	if (fv4 <= .2f)
		title_flags |= inheritance::PREFERENCE_ONLY;
	else if (fv3 <= .6f)
		title_flags = inheritance::INHERIT_THROUGH_OTHER;
}

char_id_t best_candidate(INOUT(std::vector<char_id_t>) candidates, unsigned char gender_law) noexcept {
	if (candidates.size() == 0)
		return char_id_t();

	if (gender_law == inheritance::MALE) {
		vector_erase_if(candidates, [](char_id_t id) { return get_object(id).gender != person::MALE; });
	} else if (gender_law == inheritance::FEMALE) {
		vector_erase_if(candidates, [](char_id_t id) { return get_object(id).gender != person::FEMALE; });
	}

	std::sort(candidates.begin(), candidates.end(), [](char_id_t a, char_id_t b) {
		return get_object(a).born < get_object(b).born;
	});
	
	std::vector<char_id_t> tempc;
	if (gender_law == inheritance::GENDER_EQUALITY) {
		for (char_id_t c : candidates) {
			if (get_object(c).died != 0) {
				global::get_children(c, tempc);
				const auto r = best_candidate(tempc, gender_law);
				tempc.clear();
				if (valid_ids(r))
					return r;
			} else {
				return c;
			}
		}
	} else if ((gender_law & inheritance::MALE) != 0) {
		//first pass : preferred gender
		for (char_id_t c : candidates) {
			if (get_object(c).gender == person::MALE) {
				if (get_object(c).died != 0) {
					global::get_children(c, tempc);
					const auto r = best_candidate(tempc, gender_law);
					tempc.clear();
					if (valid_ids(r))
						return r;
				} else {
					return c;
				}
			}
		}
		//second pass : other gender, if permitted
		for (char_id_t c : candidates) {
			if (get_object(c).gender == person::FEMALE) {
				if (get_object(c).died != 0 || (gender_law & inheritance::INHERIT_THROUGH_OTHER) != 0) {
					global::get_children(c, tempc);
					const auto r = best_candidate(tempc, gender_law);
					tempc.clear();
					if (valid_ids(r))
						return r;
				} else if((gender_law & inheritance::PREFERENCE_ONLY) != 0) {
					return c;
				}
			}
		}
	} else {
		//first pass : preferred gender
		for (char_id_t c : candidates) {
			if (get_object(c).gender == person::FEMALE) {
				if (get_object(c).died != 0) {
					global::get_children(c, tempc);
					const auto r = best_candidate(tempc, gender_law);
					tempc.clear();
					if (valid_ids(r))
						return r;
				} else {
					return c;
				}
			}
		}
		//second pass : other gender, if permitted
		for (char_id_t c : candidates) {
			if (get_object(c).gender == person::MALE) {
				if (get_object(c).died != 0 || (gender_law & inheritance::INHERIT_THROUGH_OTHER) != 0) {
					global::get_children(c, tempc);
					const auto r = best_candidate(tempc, gender_law);
					tempc.clear();
					if (valid_ids(r))
						return r;
				} else if ((gender_law & inheritance::PREFERENCE_ONLY) != 0) {
					return c;
				}
			}
		}
	}

	return char_id_t();
}

char_id_t get_prim_heir(IN(administration) adm, char_id_t current_holder, IN(g_lock) l) noexcept {
	char_id_t dh = get_sub_title_holder(adm.associated_title, sub_title::DESIGNATED_HEIR, l);
	if (valid_ids(dh))
		return dh;

	const unsigned char inheritance = (adm.title_flags & inheritance::GENDER_LAW_MASK);

	std::vector<char_id_t> candidates;
	global::get_children(current_holder, candidates);

	char_id_t can = best_candidate(candidates, inheritance);
	if (!valid_ids(can)) {
		//search to siblings
		global::get_living_siblings(current_holder, candidates);
		can = best_candidate(candidates, inheritance);
	}

	return can;
}

char_id_t head_of_state(admin_id_t a, IN(g_lock) l) noexcept {
	return get_object(get_object(a, l).associated_title).holder;
}

char_id_t head_of_state(IN(administration) adm) noexcept {
	return get_object(adm.associated_title).holder;
}

char_id_t get_admin_holder(admin_id_t id, IN(g_lock) l) noexcept {
	if (valid_ids(id)) {
		return head_of_state(get_object(id, l));
	}
	return char_id_t();
}

bool can_raise_troops(char_id_t c, IN(administration) adm) noexcept {
	if (head_of_state(adm) == c) {
		return (adm.vassal_flags & administration::VFLAG_NO_ARMIES) == 0;
	}
	return false;
}

admin_id_t get_prime_leige(char_id_t id, IN(g_lock) l) noexcept {
	const auto pt = get_object(id).primetitle;
	// if (valid_ids(pt)) {
		IN(auto) t = get_object(pt);
		if(valid_ids(t.associated_admin))
			return get_object(t.associated_admin, l).leige;
	// }
	return admin_id_t();
}

admin_id_t get_prime_admin(char_id_t id, IN(g_lock) l) noexcept {
	const auto pt = get_object(id).primetitle;
	// if (valid_ids(pt))
		return get_object(pt).associated_admin;
	return admin_id_t();
}

char_id_t get_diplo_decider(IN(administration) adm, IN(g_lock) l) noexcept {
	return adm.executive;
}

char_id_t get_local_decider(IN(administration) adm, IN(g_lock) l) noexcept {
	return adm.executive;
}

char_id_t get_spy_decider(IN(administration) adm, IN(g_lock) l) noexcept {
	return adm.executive;
}

char_id_t get_leg_decider(IN(administration) adm, IN(g_lock) l) noexcept {
	if (adm.legislative == administration::EXECUTIVE || adm.legislative == administration::BY_VOTE) {
		return adm.executive;
	}
	char_id_t result;
	sub_titles.for_each_breakable(adm.associated_title, l, [&result, t = adm.legislative](IN(sub_title) s) {
		if (s.type == t) {
			result = s.holder;
			return true;
		}
		return false;
	});
	return result;
}

char_id_t get_war_decider(IN(administration) adm, IN(g_lock) l) noexcept {
	if (adm.war == administration::EXECUTIVE || adm.war == administration::BY_VOTE) {
		return adm.executive;
	}
	char_id_t result;
	sub_titles.for_each_breakable(adm.associated_title, l, [&result, t = adm.war](IN(sub_title) s) {
		if (s.type == t) {
			result = s.holder;
			return true;
		}
		return false;
	});
	return result;
}

char_id_t get_judge_decider(IN(administration) adm, IN(g_lock) l) noexcept {
	if (adm.judgement == administration::EXECUTIVE || adm.judgement == administration::BY_VOTE) {
		return adm.executive;
	}
	char_id_t result;
	sub_titles.for_each_breakable(adm.associated_title, l, [&result, t = adm.judgement](IN(sub_title) s) {
		if (s.type == t) {
			result = s.holder;
			return true;
		}
		return false;
	});
	return result;
}

constexpr double power_type_fraction = 0.2;

void indiv_power_by_power_type(INOUT(double) executive_power, INOUT(double) indiv_power, char_id_t individual_for, title_id_t associated_title, unsigned char type, bool is_vassal, bool is_executive, double vassal_power, IN(g_lock) l) {
	if (type != sub_title::EXECUTIVE) {
		executive_power -= power_type_fraction;
		if (type == administration::BY_VOTE) {
			if (is_vassal || is_executive) {
				indiv_power += power_type_fraction * vassal_power;
			}
		} else {
			double number_of_st_holders = 0.0;
			bool is_st_holder = false;
			sub_titles.for_each(associated_title, l, [individual_for, type, &is_st_holder, &number_of_st_holders](IN(sub_title) st) {
				if (st.type == type) {
					++number_of_st_holders;
					if (st.holder == individual_for) {
						is_st_holder = true;
					}
				}
			});
			if (is_st_holder) {
				indiv_power += power_type_fraction * (1.0 / number_of_st_holders);
			}
		}
	}
}

double power_in_administration(admin_id_t a, char_id_t ch, IN(g_lock) l) noexcept {
	IN(auto) adm = get_object(a, l);

	const bool is_vassal = leigetoadmin.for_each_breakable(a, l, [&l, ch](admin_id_t vassal) { return head_of_state(vassal, l) == ch; });
	const double vassal_power = 1.0 / (static_cast<double>(leigetoadmin.count(a, l)) + 1.0);
	const bool is_executive = adm.executive == ch;

	double executive_power = 1.0;
	double total_power = 0.0;

	indiv_power_by_power_type(executive_power, total_power, ch, adm.associated_title, adm.legislative, is_vassal, is_executive, vassal_power, l);
	indiv_power_by_power_type(executive_power, total_power, ch, adm.associated_title, adm.war, is_vassal, is_executive, vassal_power, l);
	indiv_power_by_power_type(executive_power, total_power, ch, adm.associated_title, adm.judgement, is_vassal, is_executive, vassal_power, l);
	indiv_power_by_power_type(executive_power, total_power, ch, adm.associated_title, adm.religious_head, is_vassal, is_executive, vassal_power, l);

	return total_power + (is_executive ? executive_power : 0.0);
}

void get_force_estimate(char_id_t p, INOUT(float) mu, INOUT(float) sigmasq, IN(g_lock) l) noexcept {
	with_udata(p, l, [&mu, &sigmasq](IN(udata) d) {
		mu += d.mu;
		sigmasq += d.sigma_sq;
	});
}


void get_defensive_force_estimate(char_id_t p, INOUT(float) mu, INOUT(float) sigmasq, IN(g_lock) l) noexcept {
	enum_pacts_for(p, l, [&mu, &sigmasq, p, &l](pact_id_t id) {
		IN(auto) pact = get_object(id, l);
		if (pact.pact_type == P_DEFENCE) {
			if (pact.a != p)
				get_force_estimate(pact.a, mu, sigmasq, l);
			else
				get_force_estimate(pact.b, mu, sigmasq, l);
		}
	});
}

void get_defensive_against_force_estimate(char_id_t p, char_id_t against, INOUT(float) mu, INOUT(float) sigmasq, IN(g_lock) l) noexcept {
	enum_pacts_for(p, l, [&mu, &sigmasq, p, against, &l](pact_id_t id) {
		IN(auto) pact = get_object(id, l);
		if (pact.pact_type == P_DEFENCE_AGAINST && pact.against == against) {
			if (pact.a != p)
				get_force_estimate(pact.a, mu, sigmasq, l);
			else
				get_force_estimate(pact.b, mu, sigmasq, l);
		}
	});
}

void update_force_est(char_id_t chfor, float pts_raised, IN(w_lock) l) noexcept {
	with_udata(chfor, l, [pts_raised](INOUT(udata) d) {
		const float sigma_n_sq = (d.sigma_sq * known_variance) / (d.sigma_sq + known_variance);
		d.mu = sigma_n_sq * (d.mu / d.sigma_sq + pts_raised / known_variance);
		d.sigma_sq = sigma_n_sq;
	});
}

void admin_f_generate(IN_P(sqlite3) db) noexcept {
	sqlite3_exec(db, "CREATE TABLE admin (id INTEGER PRIMARY KEY, associated_title INTEGER, stats INTEGER, executive INTEGER, \
		protections INTEGER, capital INTEGER, court_culture INTEGER, official_religion INTEGER, leige INTEGER, \
		legislative INTEGER, war INTEGER, judgement INTEGER, religious_head INTEGER, vassal_req INTEGER, \
		vassal_flags INTEGER, standard_vassal_req INTEGER, standard_vassal_flags INTEGER, title_flags INTEGER)", nullptr, nullptr, nullptr);
}

void admin_f_load(IN_P(sqlite3) db, IN(w_lock) l) noexcept {
	stmtwrapper stmt(db, "SELECT id, associated_title, stats, executive, protections, capital, court_culture, official_religion, \
		leige, legislative, war, judgement, religious_head, vassal_req, vassal_flags, standard_vassal_req, standard_vassal_flags, \
		title_flags FROM admin");

	admin_pool.clear(l);
	leigetoadmin.clear(l);

	while(stmt.step()) {
		admin_id id = static_cast<admin_id>(sqlite3_column_int64(stmt, 0));
		admin_pool.expand_capacity(id + 1, l);
		admin_pool.reserve(id, l);
		INOUT(administration) adm = admin_pool.get(id, l);

		load_sql_value(adm.associated_title, stmt, 1);
		adm.stats.load(sqlite3_column_int64(stmt, 2));
		load_sql_value(adm.executive, stmt, 3);
		load_sql_value(adm.protections, stmt, 4);
		load_sql_value(adm.capital, stmt, 5);
		load_sql_value(adm.court_culture, stmt, 6);
		load_sql_value(adm.official_religion, stmt, 7);
		load_sql_value(adm.leige, stmt, 8);
		// load_sql_value(adm.mu, stmt, 9);
		// load_sql_value(adm.sigma_sq, stmt, 10);
		load_sql_value(adm.legislative, stmt, 9);
		load_sql_value(adm.war, stmt, 10);
		load_sql_value(adm.judgement, stmt, 11);
		load_sql_value(adm.religious_head, stmt, 12);
		load_sql_value(adm.vassal_req, stmt, 13);
		load_sql_value(adm.vassal_flags, stmt, 14);
		load_sql_value(adm.standard_vassal_req, stmt, 15);
		load_sql_value(adm.standard_vassal_flags, stmt, 16);
		load_sql_value(adm.title_flags, stmt, 17);

		if(valid_ids(adm.leige))
			leigetoadmin.insert(adm.leige, admin_id_t(id), l);
	}
}

void admin_f_save(IN_P(sqlite3) db, IN(r_lock) l) noexcept {
	sqlite3_exec(db, "DELETE FROM admin", nullptr, nullptr, nullptr);
	stmtwrapper add_admin(db, "INSERT INTO admin (id, associated_title, stats, executive, protections, capital, court_culture, official_religion, \
		leige, legislative, war, judgement, religious_head, vassal_req, vassal_flags, standard_vassal_req, standard_vassal_flags, \
		title_flags) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18)");

	admin_pool.for_each(l, [&add_admin, &l](IN(administration) adm) noexcept {
		bindings bind(add_admin);
		bind(1, admin_pool.get_index(adm, l));
		bind(2, adm.associated_title);
		bind(3, adm.stats.save());
		bind(4, adm.executive);
		bind(5, adm.protections);
		bind(6, adm.capital);
		bind(7, adm.court_culture);
		bind(8, adm.official_religion);
		bind(9, adm.leige);
		// bind(10, adm.mu);
		// bind(11, adm.sigma_sq);
		bind(10, adm.legislative);
		bind(11, adm.war);
		bind(12, adm.judgement);
		bind(13, adm.religious_head);
		bind(14, adm.vassal_req);
		bind(15, adm.vassal_flags);
		bind(16, adm.standard_vassal_req);
		bind(17, adm.standard_vassal_flags);
		bind(18, adm.title_flags);

		add_admin.step();
	});
}
