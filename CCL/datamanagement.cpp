#include "globalhelpers.h"
#include "datamanagement.hpp"
#include "structs.hpp"
#include "living_data.h"
#include "requests.h"
#include "pacts.h"
#include "mapdisplay.h"
#include "relations.h"
#include "prov_control.h"
#include "i18n.h"
#include "finances.h"
#include "court.h"
#include "laws.h"

inline std::wstring proper_title_name(title_id_t id, const int type) {
	size_t params[2] = {static_cast<size_t>(type), id.value};
	return get_p_string(TX_TITLE_DISP, params, 2);
};



char_id_t global::get_targetable_leige(admin_id_t target, admin_id_t source, IN(g_lock) l) {
	const auto lg = get_admin_holder(get_object(source, l).leige, l);
	while (valid_ids(target)) {
		IN(auto) t = get_object(target, l);
		if (!valid_ids(t.leige) || lg == get_admin_holder(t.leige, l))
			return head_of_state(t);
		target = t.leige;
	}
	return char_id_t();
}



double global::get_wealth(char_id_t charid, IN(g_lock) l) {
	return with_udata(charid, l, 0.0, [](IN(udata) d) noexcept { return d.wealth; });
}


double global::force_project_income(char_id_t charid, IN(g_lock) l) {
	double tax = 0.0;
	global::holdertotitle.for_each(charid, l, [&tax, &l](title_id_t ttl) noexcept {
		IN(auto) to = get_object(ttl);
		if (valid_ids(to.associated_admin)) {
			global::enum_control_by_admin(to.associated_admin, l, [&tax, &l](IN(controlrecord) rec) noexcept {
				if (is_prov_controlled(rec.province, l)) {
					IN(auto) p = get_object(rec.province);
					tax += p.tax * rec.tax;
				}
			});
		}
	});
	return tax;
}

double global::project_income(char_id_t charid, IN(g_lock) l) {
	return with_udata(charid, l, 0.0, [charid, &l](INOUT(udata) d) noexcept {
		if (d.income_estimate != -DBL_MAX)
			return d.income_estimate;
		else
			return d.income_estimate = global::force_project_income(charid, l);
	});
}

void global::set_holder(title_id_t t, char_id_t c, IN(w_lock) l) {
	INOUT(auto) to = get_object(t);
	global::holdertotitle.erase(to.holder, t, l);
	to.holder = c;
	global::holdertotitle.insert(c, t, l);
}

void global::update_prov_control(prov_id_t p, admin_id_t lowest, IN(w_lock) l) {
		global::remove_all_prov_control(p, l);
	
		controlrecord base;
		base.province = p;
		date d = date_offset + days(global::currentday);
		base.since = d.year() - date_offset.year();

		admin_id_t current(lowest);

		IN(auto) oadm = get_object(current, l);

		double taxreserved = 0.0;
		double tax_to_leige = oadm.tax_amount();
		current = oadm.leige;

		while (valid_ids(current)) {
			IN(auto) cadm = get_object(current, l);

			base.ad_controller = current;
			taxreserved += (base.tax = tax_to_leige);
			global::add_control(base, l);

			tax_to_leige = cadm.tax_amount();
			current = cadm.leige;
		}
		

		base.ad_controller = lowest;
		base.tax = std::max(0.0, 1.0 - taxreserved);
		global::add_control(base, l);
}


void global::update_prime_t(char_id_t ch, IN(g_lock) l) {
	title_id_t newprime;
	global::holdertotitle.for_each(ch, l, [&newprime](title_id_t t) noexcept {
		if (!valid_ids(newprime) || get_object(newprime).type > get_object(t).type)
			newprime = t;
	});
	get_object(ch).primetitle = newprime;
}

void global::update_capital(admin_id_t id, IN(w_lock) l) {

	prov_id_t maxprov;
	double maxtax = 0.0;
	INOUT(auto) adm = get_object(id, l);

	global::enum_excl_control_by_admin(id, l, [&maxprov, &maxtax](IN(controlrecord) rec) noexcept {
		IN(auto) p = get_object(rec.province);
		if (p.tax > maxtax) {
			maxtax = p.tax;
			maxprov = rec.province;
		}
	});

	if (valid_ids(maxprov)) {
		adm.capital = maxprov;
	} else {
		global::enum_control_by_admin(id, l, [&maxprov, &maxtax](IN(controlrecord) rec) noexcept {
			IN(auto) p = get_object(rec.province);
			if (p.tax > maxtax) {
				maxtax = p.tax;
				maxprov = rec.province;
			}
		});

		if (valid_ids(maxprov))
			adm.capital = maxprov;
	}
}

void global::update_title_stats(IN(g_lock) l) {
	admin_pool.for_each(l, [&l](INOUT(administration) adm) {
		adm.stats.clear();
		const auto hos = adm.executive;
		//if (valid_ids(hos)) {
			with_udata(hos, l, [&adm](IN(udata) d) noexcept { adm.stats.add(d.stats, 1.0); });

			enum_council(admin_id_t(admin_pool.get_index(adm, l)), l, [&adm, &l, hos](char_id_t id) noexcept {
				with_udata(id, l, [&adm, hos, &l, id](IN(udata) d) noexcept {
					adm.stats.add(d.stats, std::min(1.0, opinion(id, hos, l) + 0.75), -1.0);
				});
			});
		//}
	});
}

void global::get_court(admin_id_t id, INOUT(std::vector<char_id_t>) id_list, IN(w_lock) l) {
	enum_council(id, id_list, l);
	enum_court_a(id, id_list, l);
}

void global::get_court(admin_id_t id, INOUT(cvector<char_id_t>) id_list, IN(g_lock) l) {
	enum_council(id, id_list, l);
	enum_court_a(id, id_list, l);
}

void global::get_extended_court(char_id_t person, INOUT(cvector<char_id_t>) id_list, IN(g_lock) l) {
	holdertotitle.for_each(person, l, [&id_list, &l](title_id_t t) noexcept {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			get_court(to.associated_admin, id_list, l);
		}
	});
}

prov_id_t global::get_home_province(char_id_t person, IN(g_lock) l) {
	const auto pt = get_object(person).primetitle;
	if (valid_ids(pt, get_object(pt).associated_admin)) {
		return get_object(get_object(pt).associated_admin, l).capital;
	}
	
	const auto pa = get_prime_admin(char_id_t(person), l);
	if (valid_ids(pa))
		return get_object(pa, l).capital;
	
	prov_id_t cap;
	if (r_sub_titles.for_each_breakable(char_id_t(person), l, [&cap, &l](IN(r_sub_title) t) noexcept {
		IN(auto) to = get_object(t.parent);
		if (valid_ids(to.associated_admin) & (t.type != sub_title::DESIGNATED_HEIR)) {
			cap = get_object(to.associated_admin, l).capital;
			return true;
		}
		return false;
	})) {
		return cap;
	}
	return with_udata(person, l, prov_id_t(), [&l](IN(udata) d) noexcept {
		if (valid_ids(d.a_court)) {
			return get_object(d.a_court, l).capital;
		}
		return prov_id_t();
	});
}

void global::get_extended_court(char_id_t person, INOUT(std::vector<char_id_t>) id_list, IN(w_lock) l) {
	holdertotitle.for_each(person, l, [&id_list, &l](title_id_t t) noexcept {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			get_court(to.associated_admin, id_list, l);
		}
	});
}



unsigned int global::days_between(char_id_t a, char_id_t b, IN(g_lock) l) {
	const auto capital_a = get_home_province(a, l);
	const auto capital_b = get_home_province(a, l);
	if (valid_ids(capital_a, capital_b)) {
		return static_cast<unsigned int>(province_distance(get_object(capital_a), get_object(capital_b)));
	}
	return 0;
}

bool global::is_vassal_of(char_id_t lesser, char_id_t greater, IN(g_lock) l) {
	auto padm = get_prime_admin(lesser, l);
	
	for (; valid_ids(padm); padm = get_object(padm, l).leige) {
		if (get_object(get_object(padm, l).associated_title).holder == greater)
			return true;
	}

	return false;
}

bool global::is_marriable(char_id_t id) {
	IN(auto) p = get_object(id);
	return !valid_ids(p.spouse) && global::currentday >= (p.born + 365*16) && p.died == 0;
}

bool global::are_marriable(char_id_t a, char_id_t b) {
	if (!valid_ids(a)) {
		return is_marriable(b);
	} if (!valid_ids(b)) {
		return is_marriable(a);
	}
	return is_marriable(b) &&
		is_marriable(a) &&
		((get_object(a).gender == 0 && get_object(b).gender == 1) || (get_object(a).gender == 1 && get_object(b).gender == 0)) &&
		!are_close_family(a, b);
}

void global::monthly_update_title_stats(IN(g_lock) l) {
	global::update_title_stats(l);
}


bool global::is_prov_controlled(prov_id_t prov, IN(g_lock) l) {
	return !occupation_info.contains(prov, l);
}

title_id_t global::get_prime_title(char_id_t id) {
	return get_object(id).primetitle;
}

prov_id_t global::get_admin_capital(admin_id_t id, IN(g_lock) l) {
	return get_object(id, l).capital;
}

std::string global::character_name(char_id_t id) {
	IN(auto) p = get_object(id);
	return p.name.get() + " " + get_object(p.dynasty).name.get();
}

std::string global::title_name(title_id_t id) {
	return wstr_to_str(proper_title_name(id, get_object(id).type));
}

std::string global::province_name(prov_id_t id) {
	return get_object(id).name.get();
}

std::wstring global::w_character_name(char_id_t id) {
	IN(auto) p = get_object(id);
	return str_to_wstr(p.name.get() + " " + get_object(p.dynasty).name.get());
}

std::wstring global::w_title_name(title_id_t id) {
	return proper_title_name(id, get_object(id).type);
}

std::wstring global::w_province_name(prov_id_t id) {
	return str_to_wstr(get_object(id).name.get());
}

std::wstring global::get_composed_title(title_id_t id, int type) {
	size_t params[2] = {static_cast<size_t>(type), id.value};
	return get_p_string(TX_COM_TITLE, params, 2);
}

std::wstring global::get_expanded_name(char_id_t id) {
	const auto pt = get_object(id).primetitle;
	if (valid_ids(pt)) {
		size_t params[3] = {id.value, static_cast<size_t>(get_object(pt).type), pt.value};
		return get_p_string(TX_EX_NAME, params, 3);
	} else {
		return global::w_character_name(id);
	}
	
}

bool global::independant_policy_a(admin_id_t adm, IN(g_lock) l) {
	IN(auto) a = get_object(adm, l);
	if (!valid_ids(a.leige))
		return true;
	if ((a.vassal_flags & (administration::VFLAG_NO_EXT_WARS | administration::VFLAG_NO_ARMIES)) != 0)
		return false;
	return true;
}

bool global::independant_policy_a(IN(administration) a, IN(g_lock) l) {
	if (!valid_ids(a.leige))
		return true;
	if ((a.vassal_flags & (administration::VFLAG_NO_EXT_WARS | administration::VFLAG_NO_ARMIES)) != 0)
		return false;
	return true;
}

bool global::ch_has_independant_policy(char_id_t p, IN(g_lock) l) {
	return holdertotitle.for_each_breakable(p, l, [&l](title_id_t t) {
		IN(auto) to = get_object(t);
		if (valid_ids(to.associated_admin)) {
			return independant_policy_a(to.associated_admin, l);
		}
		return false;
	});
}

char_id_t global::get_t_holder(title_id_t title) {
	return get_object(title).holder;
}

char_id_t global::get_character_leige(char_id_t id, IN(g_lock) l) {
	const auto adm = get_prime_leige(id, l);
	if (valid_ids(adm))
		return get_object(get_object(adm, l).associated_title).holder;
	return char_id_t();
}

void global::get_spouses(char_id_t id, INOUT(std::vector<char_id_t>) vec) {
	const auto sz = detail::people.size();
	for (char_id it = 1; it != sz; ++it) {
		if (detail::people[it].spouse == id)
			vec.emplace_back(it);
	}
}

void global::get_children(char_id_t id, INOUT(std::vector<char_id_t>) vec) noexcept {
	const auto sz = detail::people.size();
	for (char_id it = 1; it != sz; ++it) {
		if (detail::people[it].mother == id || detail::people[it].father == id)
			vec.emplace_back(it);
	}
}

void global::get_living_children(char_id_t id, INOUT(std::vector<char_id_t>) vec) {
	for (const auto iid : global::living) {
		IN(auto) p = get_object(iid);
		if(p.mother == id || p.father == id)
			vec.emplace_back(iid);
	}
}

void global::get_siblings(char_id_t id, INOUT(std::vector<char_id_t>) vec) {
	const auto sz = detail::people.size();
	IN(auto) p = get_object(id);
	if (valid_ids(p.father, p.mother)) {
		for (char_id it = 1; it != sz; ++it) {
			if (detail::people[it].mother == p.mother || detail::people[it].father == p.father)
				vec.emplace_back(it);
		}
	} else if (valid_ids(p.father)) {
		for (char_id it = 1; it != sz; ++it) {
			if (detail::people[it].father == p.father)
				vec.emplace_back(it);
		}
	} else if (valid_ids(p.mother)) {
		for (char_id it = 1; it != sz; ++it) {
			if (detail::people[it].mother == p.mother)
				vec.emplace_back(it);
		}
	}
	vector_erase(vec, id);
}

void global::get_living_siblings(char_id_t id, INOUT(std::vector<char_id_t>) vec) noexcept {
	IN(auto) p = get_object(id);
	if (valid_ids(p.father, p.mother)) {
		for (const auto iid : global::living) {
			IN(auto) ip = get_object(iid);
			if (ip.mother == p.mother || ip.father == p.father)
				vec.emplace_back(iid);
		}
	} else if (valid_ids(p.father)) {
		for (const auto iid : global::living) {
			if (get_object(iid).father == p.father)
				vec.emplace_back(iid);
		}
	} else if (valid_ids(p.mother)) {
		for (const auto iid : global::living) {
			if (get_object(iid).mother == p.mother)
				vec.emplace_back(iid);
		}
	}
	vector_erase(vec, id);
}

bool global::are_close_family(char_id_t a, char_id_t b) {
	IN(auto) p = get_object(a);
	IN(auto) c = get_object(b);
	return (p.mother == c.mother && valid_ids(p.mother)) || (p.father == c.father && valid_ids(p.father)) || p.mother == b || p.father == b || c.father == a || c.mother == a || p.spouse == b;
}

template<typename T, typename L>
void _get_vassals(INOUT(std::vector<char_id_t, T>) v, admin_id_t adm, IN(L) l) noexcept {
	leigetoadmin.for_each(adm, l, [&v, &l](admin_id_t vas) {
		auto hos = head_of_state(vas, l);
		if (std::find(v.begin(), v.end(), hos) == v.end())
			v.emplace_back(hos);
	});
}

void get_vassals(INOUT(std::vector<char_id_t>) v, admin_id_t adm, IN(w_lock) l) noexcept {
	_get_vassals(v, adm, l);
}

void get_vassals(INOUT(cvector<char_id_t>) v, admin_id_t adm, IN(g_lock) l) noexcept {
	_get_vassals(v, adm, l);
}

template<typename T>
void _get_living_family(char_id_t id, INOUT(std::vector<char_id_t, T>) vec) {
	vec.reserve(16);
	const auto& p = get_object(id);
	for (const auto iid : global::living) {
		const auto& c = get_object(iid);
		if ((p.mother == c.mother && valid_ids(p.mother)) || (p.father == c.father && valid_ids(p.father)) || p.mother == iid || p.father == iid || c.father == id || c.mother == id || p.spouse == iid)
			vec.push_back(iid);
	}
}

void global::get_living_family(char_id_t id, INOUT(cvector<char_id_t>) vec) {
	_get_living_family(id, vec);
}

void global::get_living_family(char_id_t id, INOUT(std::vector<char_id_t>) vec) {
	_get_living_family(id, vec);
}

template <typename A, typename L>
void _get_nom_controlled(char_id_t id, INOUT(std::vector<prov_id_t, A>) provs, IN(L) l) {
	provs.reserve(32);
	global::holdertotitle.for_each(id, l, [&l, &provs](title_id_t ttle) noexcept {
		IN(auto) to = get_object(ttle);
		if (valid_ids(to.associated_admin)) {
			global::enum_control_by_admin(to.associated_admin, l, [&provs](IN(controlrecord) rec) noexcept {
				if (std::find(provs.cbegin(), provs.cend(), rec.province) == provs.cend())
					provs.emplace_back(rec.province);
			});
		}
	});
}

void global::get_nom_controlled(char_id_t id, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) noexcept {
	_get_nom_controlled(id, provs, l);
}
void global::get_nom_controlled(char_id_t id, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) noexcept {
	_get_nom_controlled(id, provs, l);
}

template<typename T, typename L>
void _get_neighbors(char_id_t self, INOUT(std::vector<char_id_t, T>) v, IN(L) l) {
	char_id_t self_leige;

	const auto padm = get_prime_admin(self, l);
	if (!valid_ids(padm))
		return;
	self_leige = get_admin_holder(get_object(padm, l).leige, l);


	std::vector<prov_id_t, T> controlled;
	std::vector<prov_id_t, T> nb;

	_get_nom_controlled(self, controlled, l);
	for (const auto prv : controlled) {
		for (auto pr = global::province_connections.get_row(prv.value); pr.first != pr.second; ++pr.first) {
			if (!P_IS_WATER(detail::provinces[*pr.first].pflags) && std::find(controlled.cbegin(), controlled.cend(), prov_id_t(*pr.first)) == controlled.cend() && std::find(nb.cbegin(), nb.cend(), prov_id_t(*pr.first)) == nb.cend()) {
				nb.emplace_back(*pr.first);
			}
		}
	}



	for (const auto p : nb) {
		global::enum_control_by_prov(p, l, [&v, &l, self_leige](IN(controlrecord) r) {
			IN(auto) adm = get_object(r.ad_controller, l);
			if (!valid_ids(adm.leige) || get_admin_holder(adm.leige, l) == self_leige) {
				char_id_t c = get_object(adm.associated_title).holder;
				if (std::find(v.cbegin(), v.cend(), c) == v.cend()) {
					v.push_back(c);
				}
			}
		});
	}

}

void global::get_neighbors(char_id_t self, INOUT(cvector<char_id_t>) v, IN(g_lock) l) {
	_get_neighbors(self, v, l);
}
void global::get_neighbors(char_id_t self, INOUT(std::vector<char_id_t>) v, IN(w_lock) l) {
	_get_neighbors(self, v, l);
}

bool global::has_territory(char_id_t id, IN(g_lock) l) {
	return global::holdertotitle.for_each_breakable(id, l, [&l, id](title_id_t ttle) {
		IN(auto) to = get_object(ttle);
		if (valid_ids(to.associated_admin)) {
			IN(auto) adm = get_object(to.associated_admin, l);
			if (adm.executive == id || (adm.title_flags & inheritance::INHERITANCE_TYPE_MASK) == inheritance::REGENT) {
				const auto rng = admintocontrol.range(to.associated_admin, l);
				return rng.first != rng.second;
			}
			return false;
		}
	});
}

template <typename A, typename L>
void _get_dj_controlled(char_id_t id, INOUT(std::vector<prov_id_t, A>) provs, IN(L) l) {
	provs.reserve(32);
	global::holdertotitle.for_each(id, l, [ &l, &provs](title_id_t ttle) noexcept {
		global::enum_dj_by_title(ttle, l, [&provs](IN(djrecord) rec) noexcept {
			if (std::find(provs.cbegin(), provs.cend(), rec.province) == provs.cend())
				provs.push_back(rec.province);
		});
	});
}

void global::get_dj_controlled(char_id_t id, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) {
	_get_dj_controlled(id, provs, l);
}
void global::get_dj_controlled(char_id_t id, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) {
	_get_dj_controlled(id, provs, l);
}

template<typename T, typename L>
void _get_controlled_by_admin(admin_id_t id, INOUT(std::vector<prov_id_t, T>) provs, IN(L) l) noexcept {
	provs.reserve(16);
	global::enum_control_by_admin(id,l, [&provs](IN(controlrecord) rec) noexcept {
		if (std::find(provs.cbegin(), provs.cend(), prov_id_t(rec.province)) == provs.cend())
			provs.emplace_back(rec.province);
	});
}

void global::get_controlled_by_admin(admin_id_t id, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) noexcept {
	_get_controlled_by_admin(id, provs, l);
}
void global::get_controlled_by_admin(admin_id_t id, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) noexcept {
	_get_controlled_by_admin(id, provs, l);
}

template<typename T, typename L>
void _get_dj_controlled_by_t(title_id_t id, INOUT(std::vector<prov_id_t, T>) provs, IN(L) l) {
	provs.reserve(16);
	global::enum_dj_by_title(id, l, [&provs](IN(djrecord) rec) noexcept {
		if (std::find(provs.cbegin(), provs.cend(), rec.province) == provs.cend())
			provs.push_back(rec.province);
	});
}

void global::get_dj_controlled_by_t(title_id_t id, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) {
	_get_dj_controlled_by_t(id, provs, l);
}
void global::get_dj_controlled_by_t(title_id_t id, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) {
	_get_dj_controlled_by_t(id, provs, l);
}

template<typename T, typename L>
void _get_adjacent_provinces(char_id_t id, INOUT(std::vector<prov_id_t, T>) list, IN(L) l) {

	std::vector<prov_id_t, T> controlled;
	_get_nom_controlled(id, controlled, l);

	if (controlled.size() == 0)
		return;

	list.reserve(controlled.size());

	for (const auto p : controlled) {
		auto row = global::province_connections.get_row(p.value);
		for (; row.first != row.second; ++row.first) {
			const prov_id_t c(*row.first);
			if (std::find(controlled.begin(), controlled.end(), c) == controlled.end() &&
				std::find(list.begin(), list.end(), c) == list.end()) {
				if (prov_has_title(c)) {
					list.emplace_back(c);
				} else if (P_IS_WATER(get_object(c).pflags)) {
					auto innerrow = global::province_connections.get_row(c.value);
					for (; innerrow.first != innerrow.second; ++innerrow.first) {
						const prov_id_t d(*innerrow.first);
						if (std::find(controlled.begin(), controlled.end(), d) == controlled.end() &&
							std::find(list.begin(), list.end(), d) == list.end() && prov_has_title(d)) {
							list.emplace_back(d);
						}
					}
				}
			}
		}
	}

}

template<typename T, typename L>
void _get_adjacent_independant(char_id_t id, INOUT(std::vector<char_id_t, T>) list, int rank, IN(L) l) {
	std::vector<prov_id_t, T> provs;
	_get_adjacent_provinces(id, provs, l);
	list.reserve(provs.size() / 2);
	for (const auto p : provs) {
		global::provtocontrol.for_each(p, l, [&list, &l, rank](unsigned int indx) noexcept {
			IN(controlrecord) r = global::control_pool.get(indx, l);
			IN(auto) to = get_object(get_object(r.ad_controller, l).associated_title);
			const auto holder = to.holder;
			if (valid_ids(holder) &&
				std::find(list.cbegin(), list.cend(), holder) == list.cend() &&
				global::independant_policy_a(r.ad_controller, l) &&
				(to.type <= rank + 1 && to.type >= rank - 1)) {
				list.push_back(holder);
			}
		});
	}

	vector_erase_if(list, [id, &l](char_id_t c) {return global::is_vassal_of(c, id, l) || global::is_vassal_of(id, c, l); });
}

void global::get_adjacent_independant(char_id_t id, INOUT(cvector<char_id_t>) list, int rank, IN(g_lock) l) {
	_get_adjacent_independant(id, list, rank, l);
}
void global::get_adjacent_independant(char_id_t id, INOUT(std::vector<char_id_t>) list, int rank, IN(w_lock) l) {
	_get_adjacent_independant(id, list, rank, l);
}


void global::get_adjacent_provinces(char_id_t id, INOUT(cvector<prov_id_t>) list, IN(g_lock) l) {
	_get_adjacent_provinces(id, list, l);
}
void global::get_adjacent_provinces(char_id_t id, INOUT(std::vector<prov_id_t>) list, IN(w_lock) l) {
	_get_adjacent_provinces(id, list, l);
}

template<typename T, typename L>
void _get_nearby_provinces(char_id_t id, INOUT(std::vector<prov_id_t, T>) list, IN(L) l) {
	const static float additonal_angle = static_cast<float>(cos(4.0 / 360.0 * 2.0 * 3.14159));
	const static float rt_additional = static_cast<float>(sqrt(1.0 - additonal_angle* additonal_angle));
	std::vector<prov_id_t, T> controlled;
	_get_nom_controlled(id, controlled, l);

	list.reserve(controlled.size());

	if (controlled.size() == 0)
		return;

	float leastproduct = 1.0f;

	glm::vec3 sum(0.0f, 0.0f, 0.0f);
	for (const auto p : controlled) {
		sum += get_object(p).centroid;
	}
	double sdenom = sqrt(sum.x * sum.x + sum.y*sum.y + sum.z * sum.z);
	sum /= sdenom;

	for (const auto p : controlled) {
		leastproduct = std::min(glm::dot(get_object(p).centroid, sum), leastproduct);
	}

	leastproduct = leastproduct * additonal_angle - sqrt(1.0f - leastproduct*leastproduct) * rt_additional;

	for (prov_id indx = 1; indx <= province::last_titled_p; ++indx) {
		if (glm::dot(detail::provinces[indx].centroid, sum) > leastproduct &&
			std::find(controlled.cbegin(), controlled.cend(), prov_id_t(indx)) == controlled.cend()) {
			list.emplace_back(indx);
		}
	}
}

template<typename T, typename L>
void _get_nearby_independant(char_id_t id, INOUT(std::vector<char_id_t, T>) list, int rank, IN(L) l) {
	std::vector<prov_id_t, T> provs;
	_get_nearby_provinces(id, provs, l);
	list.reserve(provs.size() / 2);
	for (const auto p : provs) {
		global::provtocontrol.for_each(p, l, [&list, &l, rank](unsigned int indx) noexcept {
			IN(controlrecord) r = global::control_pool.get(indx, l);
			IN(auto) adm = get_object(r.ad_controller, l);
			const auto holder = head_of_state(adm);
			IN(auto) to = get_object(adm.associated_title);
			if (valid_ids(holder) &&
				std::find(list.cbegin(), list.cend(), holder) == list.cend() &&
				global::independant_policy_a(r.ad_controller, l) &&
				(to.type <= rank + 1 && to.type >= rank - 1)) {
				list.push_back(holder);
			}
		});
	}

	vector_erase_if(list, [id, &l](char_id_t c) {return global::is_vassal_of(c, id, l) || global::is_vassal_of(id, c, l); });
}

template<typename T, typename L>
void _get_nearby_independant(char_id_t id, INOUT(std::vector<char_id_t, T>) list, IN(L) l) {
	std::vector<prov_id_t, T> provs;
	_get_nearby_provinces(id, provs, l);
	list.reserve(provs.size() / 2);
	for (const auto p : provs) {
		global::provtocontrol.for_each(p, l, [&list, &l](unsigned int indx) noexcept {
			IN(controlrecord) r = global::control_pool.get(indx, l);
			const auto holder = head_of_state(r.ad_controller, l);
			if (valid_ids(holder) &&
				std::find(list.cbegin(), list.cend(), holder) == list.cend() &&
				global::independant_policy_a(r.ad_controller, l)) {
				list.push_back(holder);
			}
		});
	}

	vector_erase_if(list, [id, &l](char_id_t c) {return global::is_vassal_of(c, id, l) || global::is_vassal_of(id, c, l); });
}


void global::get_nearby_provinces(char_id_t id, INOUT(cvector<prov_id_t>) list, IN(g_lock) l) {
	_get_nearby_provinces(id, list, l);
}
void global::get_nearby_independant(char_id_t id, INOUT(cvector<char_id_t>) list, int rank, IN(g_lock) l) {
	_get_nearby_independant(id, list, rank, l);
}
void global::get_nearby_independant(char_id_t id, INOUT(cvector<char_id_t>) list, IN(g_lock) l) {
	_get_nearby_independant(id, list, l);
}

void global::get_nearby_provinces(char_id_t id, INOUT(std::vector<prov_id_t>) list, IN(w_lock) l) {
	_get_nearby_provinces(id, list, l);
}
void global::get_nearby_independant(char_id_t id, INOUT(std::vector<char_id_t>) list, int rank, IN(w_lock) l) {
	_get_nearby_independant(id, list, rank, l);
}
void global::get_nearby_independant(char_id_t id, INOUT(std::vector<char_id_t>) list, IN(w_lock) l) {
	_get_nearby_independant(id, list, l);
}

void global::get_dyn_members(dyn_id_t id, INOUT(std::vector<char_id_t>) alive, INOUT(std::vector<char_id_t>) dead) {
	for (char_id it = 1; it < detail::people.size(); ++it) {
		if (detail::people[it].dynasty == id) {
			if (detail::people[it].died == 0) {
				alive.emplace_back(it);
			} else {
				dead.emplace_back(it);
			}
		}
	}
}

void global::list_vassals_a(admin_id_t adm, INOUT(cvector<admin_id_t>) lst, IN(g_lock) l) noexcept {
	leigetoadmin.to_vector(adm, lst, l);
}
void global::list_vassals_a(admin_id_t adm, INOUT(std::vector<admin_id_t>) lst, IN(w_lock) l) noexcept {
	leigetoadmin.to_vector(adm, lst, l);
}

template<typename T, typename L>
void get_realm_r(char_id_t id, INOUT(std::vector<char_id_t, T>) vec, IN(L) l) {
	global::holdertotitle.for_each(id, l, [&vec, &l, id](title_id_t t) noexcept {
		const auto assadm = get_object(t).associated_admin;
		if (valid_ids(assadm)) {
			global::enum_vassals(assadm, l, [&l, &vec](admin_id_t t) noexcept {
				const auto h = get_object(get_object(t, l).associated_title).holder;
				if (valid_ids(h) && std::find(vec.cbegin(), vec.cend(), h) == vec.cend()) {
					vec.push_back(h);
					get_realm_r(h, vec, l);
				}
			});
		}
	});
}

void global::get_realm(char_id_t id, INOUT(std::vector<char_id_t>) vec, IN(w_lock) l) {
	vec.reserve(32);
	get_realm_r(id, vec, l);
}

void global::get_realm(char_id_t id, INOUT(cvector<char_id_t>) vec, IN(g_lock) l) {
	vec.reserve(32);
	get_realm_r(id, vec, l);
}

template<typename T>
void _get_recent_people(INOUT(std::vector<char_id_t, T>) vec) {
	vec.reserve(32);
	for (auto it = global::mhistory.rbegin(); it != global::mhistory.rend(); ++it) {
		if (it->first == 2 && std::find(vec.cbegin(), vec.cend(), char_id_t(static_cast<char_id>(it->second))) == vec.cend()) {
			vec.push_back(char_id_t(static_cast<char_id>(it->second)));
		}
	}
}


void global::get_recent_people(INOUT(cvector<char_id_t>) vec) {
	_get_recent_people(vec);
	vec.reserve(32);
}

void global::get_recent_people(INOUT(std::vector<char_id_t>) vec) {
	_get_recent_people(vec);
	vec.reserve(32);
}

double estimatetroopstrength_a(IN(administration) adm, admin_id_t a, char_id_t behalfof, INOUT(cvector<admin_id_t>) raisedset, IN(g_lock) l) {

	double str = 0;
	const auto holder = get_object(adm.associated_title).holder;

	raisedset.push_back(a);

	str += 200.0; //placeholder for levey from admin

	leigetoadmin.for_each(a, l, [&raisedset, &l, behalfof, &str](admin_id_t id) noexcept {
		if (std::find(raisedset.cbegin(), raisedset.cend(), id) == raisedset.cend()) {
			IN(auto) adm = get_object(id, l);
			str += estimatetroopstrength_a(adm, id, behalfof, raisedset, l);
		}
	});

	return str;
}

admin_id_t global::get_top_title(admin_id_t id, IN(g_lock) l) {
	while (true) {
		IN(auto) adm = get_object(id, l);
		if (!valid_ids(adm.leige))
			return id;
		id = adm.leige;
	}
	return admin_id_t();
}

void global::get_tax_income(IN(g_lock) l) {
	for_all_control(l, [&l](IN(controlrecord) rec) noexcept {
		with_udata(head_of_state(rec.ad_controller, l), l, [&rec, &l](INOUT(udata) d) noexcept {
			if (!occupation_info.contains(rec.province, l)) {
				IN(auto) p = get_object(rec.province);
				d.wealth += p.tax * rec.tax;
			}
		});
	});

	pay_all_expenses(l);
}

void global::delta_money(char_id_t id, double change, IN(g_lock) l) {
	with_udata(id, l, [change](INOUT(udata) d) noexcept { d.wealth += change; });
}

void rel_f_generate(IN_P(sqlite3) db) {
	sqlite3_exec(db, "CREATE TABLE religions (id INTEGER PRIMARY KEY, name INTEGER, rgroup INTEGER)", nullptr, nullptr, nullptr);
}

void rel_f_load(IN_P(sqlite3) db, IN(w_lock) l) {
	detail::religions.clear();

	stmtwrapper stmt(db, "SELECT id, name, rgroup FROM religions");

	while(stmt.step()) {
		rel_id id;
		load_sql_value(id, stmt, 0);

		if (detail::religions.size() <= id) {
			detail::religions.resize(id + 1);
		}
		religion& r = detail::religions[id];

		load_sql_value(r.name, stmt, 1);
		load_sql_value(r.group, stmt, 2);
	}
}

void rel_f_save(IN_P(sqlite3) db, IN(r_lock) l) {
	sqlite3_exec(db, "DELETE FROM religions", nullptr, nullptr, nullptr);
	stmtwrapper add_rel(db, "INSERT INTO religions (id, name, rgroup) VALUES(?1, ?2, ?3)");

	for (size_t i = 1; i != detail::religions.size(); ++i) {
		IN(religion) r = detail::religions[i];
		bindings bind(add_rel);
		bind(1, i);
		bind(2, r.name);
		bind(3, r.group);
		add_rel.step();
	}
}

void cul_f_generate(IN_P(sqlite3) db) {
	sqlite3_exec(db, "CREATE TABLE cultures (id INTEGER PRIMARY KEY, name INTEGER, cgroup INTEGER, \
		dprefix INTEGER, malenames BLOB, femalenames BLOB)", nullptr, nullptr, nullptr);
}

void cul_f_load(IN_P(sqlite3) db, IN(w_lock) l) {
	detail::cultures.clear();

	stmtwrapper stmt(db, "SELECT id, name, cgroup, dprefix, malenames, femalenames FROM cultures");

	while(stmt.step()) {
		cul_id id = static_cast<title_id>(sqlite3_column_int64(stmt, 0));
		if (detail::cultures.size() <= id) {
			detail::cultures.resize(id + 1);
		}
		culture& c = detail::cultures[id];

		load_sql_value(c.name, stmt, 1);
		load_sql_value(c.group, stmt, 2);
		load_sql_value(c.prefix, stmt, 3);

		load_sql_value(c.mnames, stmt, 4);
		load_sql_value(c.fnames, stmt, 5);
	}
}

void cul_f_save(IN_P(sqlite3) db, IN(r_lock) l) {
	sqlite3_exec(db, "DELETE FROM cultures", nullptr, nullptr, nullptr);

	stmtwrapper add_cul(db, "INSERT INTO cultures (id, name, cgroup, dprefix, malenames, femalenames) VALUES(?1, ?2, ?3, ?4, ?5, ?6)");

	for (size_t i = 1; i != detail::cultures.size(); ++i) {
		IN(culture) c = detail::cultures[i];
		bindings bind(add_cul);
		bind(1, i);
		bind(2, c.name);
		bind(3, c.group);
		bind(4, c.prefix);
		bind(5, c.mnames.data(), c.mnames.size() * sizeof(sref));
		bind(6, c.fnames.data(), c.fnames.size() * sizeof(sref));
		add_cul.step();
	}
}

void titles_f_generate(IN_P(sqlite3) db) {
	sqlite3_exec(db, "CREATE TABLE titles (id INTEGER PRIMARY KEY, type INTEGER, color1 INTEGER, \
		color2 INTEGER, holder INTEGER, rname INTEGER, adj INTEGER, admin INTEGER)", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "CREATE TABLE sub_titles (title INTEGER, holder INTEGER, type INTEGER, flags INTEGER)", nullptr, nullptr, nullptr);
}

void titles_f_load(IN_P(sqlite3) db, IN(w_lock) l) {
	detail::titles.clear();
	global::holdertotitle.clear(l);

	stmtwrapper count(db, "SELECT COUNT(*) FROM titles");
	count.step();
	size_t newsz;
	load_sql_value(newsz, count, 0);
	detail::titles.resize(newsz + 1);


	{
		stmtwrapper stmt(db, "SELECT id, type, color1, color2, holder, rname, adj, admin FROM titles");

		while (stmt.step()) {
			title_id id = static_cast<title_id>(sqlite3_column_int64(stmt, 0));
			if (detail::titles.size() <= id) {
				detail::titles.resize(id + 1);
			}
			title& t = detail::titles[id];

			load_sql_value(t.type, stmt, 1);
			t.color1 = sf::Color(static_cast<sf::Uint32>((sqlite3_column_int64(stmt, 2) << 8) | 255));
			load_sql_value(t.holder, stmt, 4);
			load_sql_value(t.rname, stmt, 5);
			load_sql_value(t.adj, stmt, 6);
			load_sql_value(t.associated_admin, stmt, 7);

			global::holdertotitle.insert(t.holder, title_id_t(id), l);
		}
	}

	sub_titles.clear(l);
	r_sub_titles.clear(l);

	{
		stmtwrapper stmt(db, "SELECT title, holder, type, flags FROM sub_titles");

		while (stmt.step()) {
			title_id_t tid;
			load_sql_value(tid, stmt, 0);
			sub_title st;
			load_sql_value(st.holder, stmt, 1);
			load_sql_value(st.type, stmt, 2);
			load_sql_value(st.title_flags, stmt, 3);

			create_sub_title(tid, st.type, st.holder, st.title_flags, l);
		}
	}
}

void titles_f_save(IN_P(sqlite3) db, IN(r_lock) l) {
	sqlite3_exec(db, "DELETE FROM titles", nullptr, nullptr, nullptr);

	stmtwrapper add_title(db, "INSERT INTO titles (id, type, color1, holder, rname, adj, admin) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)");

	for(size_t i = 1; i != detail::titles.size(); ++i) {
		IN(title) t = detail::titles[i];
		bindings bind(add_title);
		bind(1, i);
		bind(2, t.type);
		bind(3, t.color1.toInteger() >> 8);
		bind(4, t.holder);
		bind(5, t.rname);
		bind(6, t.adj);
		bind(7, t.associated_admin);

		add_title.step();
	}

	stmtwrapper add_sub_title(db, "INSERT INTO sub_titles (title, holder, type, flags) VALUES (?1, ?2, ?3, ?4)");
	sub_titles.for_all(l, [&add_sub_title](title_id_t t, IN(sub_title) st) {
		bindings bind(add_sub_title);
		bind(1, t);
		bind(2, st.holder);
		bind(3, st.type);
		bind(4, st.title_flags);

		add_sub_title.step();
	});
}

void dyn_f_generate(IN_P(sqlite3) db) {
	sqlite3_exec(db, "CREATE TABLE dynasties (id INTEGER PRIMARY KEY, name INTEGER, culture INTEGER)", nullptr, nullptr, nullptr);
}

void dyn_f_load(IN_P(sqlite3) db, IN(w_lock) l) {
	detail::dynasties.clear();
	
	stmtwrapper count(db, "SELECT COUNT(*) FROM dynasties");
	count.step();
	size_t newsz;
	load_sql_value(newsz, count, 0);
	detail::dynasties.resize(newsz + 1);
	
	stmtwrapper stmt(db, "SELECT id, name, culture FROM dynasties");

	while(stmt.step()) {
		dyn_id id = static_cast<dyn_id>(sqlite3_column_int64(stmt, 0));
		if (detail::dynasties.size() <= id) {
			detail::dynasties.resize(id + 1);
		}
		dynasty& d = detail::dynasties[id];

		load_sql_value(d.name, stmt, 1);
		load_sql_value(d.culture, stmt, 2);
	}
}

void dyn_f_save(IN_P(sqlite3) db, IN(r_lock) l) {
	sqlite3_exec(db, "DELETE FROM dynasties", nullptr, nullptr, nullptr);

	stmtwrapper add_dyn(db, "INSERT INTO dynasties (id, name, culture) VALUES (?1, ?2, ?3)");

	for (size_t i = 1; i != detail::dynasties.size(); ++i) {
		IN(dynasty) d = detail::dynasties[i];
		bindings bind(add_dyn);
		bind(1, i);
		bind(2, d.name);
		bind(3, d.culture);
		add_dyn.step();
	}
}

void prov_f_generate(IN_P(sqlite3) db) {
	sqlite3_exec(db, "CREATE TABLE prov_state (id INTEGER PRIMARY KEY, culture INTEGER, religion INTEGER, \
		flags INTEGER, tax FLOAT)", nullptr, nullptr, nullptr);
}

void prov_f_load(IN_P(sqlite3) db, IN(w_lock) l) {
	//don't clear provinces to save map data

	stmtwrapper stmt(db, "SELECT id, culture, religion, flags, tax FROM prov_state");
	province::last_titled_p = 0;

	while(stmt.step()) {
		prov_id id = static_cast<prov_id>(sqlite3_column_int64(stmt, 0));
		if (detail::provinces.size() <= id) {
			detail::provinces.resize(id + 1);
		}
		province& p = detail::provinces[id];

		load_sql_value(p.culture, stmt, 1);
		load_sql_value(p.religion, stmt, 2);
		load_sql_value(p.pflags, stmt, 3);
		load_sql_value(p.tax, stmt, 4);

		if ((p.pflags & province::FLAG_HASTITLE) != 0) {
			if (id > province::last_titled_p)
				province::last_titled_p = id;
		}
	}
}

void prov_f_save(IN_P(sqlite3) db, IN(r_lock) l) {
	sqlite3_exec(db, "DELETE FROM prov_state", nullptr, nullptr, nullptr);

	stmtwrapper add_prov(db, "INSERT INTO prov_state (id, culture, religion, flags, tax) VALUES (?1, ?2, ?3, ?4, ?5)");

	for (size_t i = 1; i != detail::provinces.size(); ++i) {
		IN(province) p = detail::provinces[i];
		bindings bind(add_prov);
		bind(1, i);
		bind(2, p.culture);
		bind(3, p.religion);
		bind(4, p.pflags & ~province::FLAG_HIHGLIGHT);
		bind(5, p.tax);
		add_prov.step();
	}
}

void prov_map_f_generate(IN_P(sqlite3) db) {
	sqlite3_exec(db, "CREATE TABLE prov_map (id INTEGER PRIMARY KEY, name INTEGER, \
		climate INTEGER, terrain BLOB, intersects BLOB, bounds BLOB)", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "CREATE TABLE borders (prova INTEGER, provb INTEGER, border BLOB)", nullptr, nullptr, nullptr);
}

//load before loading province data
void prov_map_f_load(IN_P(sqlite3) db, IN(w_lock) l, bool no_ui) {
	detail::provinces.clear();

	stmtwrapper count(db, "SELECT COUNT(*) FROM prov_state");
	count.step();
	size_t newsz;
	load_sql_value(newsz, count, 0);
	detail::provinces.resize(newsz + 1);

	stmtwrapper stmt(db, "SELECT id, name, climate, terrain, intersects, bounds FROM prov_map");

	while(stmt.step()) {
		prov_id id = static_cast<prov_id>(sqlite3_column_int64(stmt, 0));
		if (detail::provinces.size() <= id) {
			detail::provinces.resize(id + 1);
		}
		province& p = detail::provinces[id];

		load_sql_value(p.name, stmt, 1);
		load_sql_value(p.climate, stmt, 2);
		load_sql_value(p.terrain, sizeof(p.terrain), stmt, 3);
		
		p.intersect.clear();

		const auto elements = sqlite3_column_bytes(stmt, 4) / (sizeof(short) + sizeof(char));
		const char* data = static_cast<const char*>(sqlite3_column_blob(stmt, 4));
		for (size_t i = 0; i != elements; ++i) {
			short* ival = (short*)(data + (i *  (sizeof(short) + sizeof(char))));
			const char* cval = data + sizeof(short) + (i *  (sizeof(short) + sizeof(char)));
			if (*ival == -1)
				p.intersect.new_row();
			else
				p.intersect.push_back(std::pair<int, int>(static_cast<int>(*ival), static_cast<int>(*cval)));
		}

		load_sql_value(&(p.bounds), sizeof(p.bounds), stmt, 5);
	}

	detail::provinces.shrink_to_fit();

	global::province_connections.clear();

	stmtwrapper stmt2(db, "SELECT prova, provb, border FROM borders");

	flat_multimap<edge, std::vector<sf::Vector2<short>>> brder;

	while(stmt2.step()) {
		std::vector<sf::Vector2<short>> temp;
		load_sql_value(temp, stmt2, 2);

		const prov_id prov_a = static_cast<prov_id>(sqlite3_column_int(stmt2, 0));
		const prov_id prov_b = static_cast<prov_id>(sqlite3_column_int(stmt2, 1));
		emplace_vv_unique(global::province_connections, prov_a, prov_b);
		emplace_vv_unique(global::province_connections, prov_b, prov_a);

		brder.emplace(edge(prov_a, prov_b), std::move(temp));
	}

	if (!no_ui) {
		std::vector<cvector<sf::Vector2f>> quads;
		quads.resize(detail::provinces.size());

		concurrency::parallel_for_each(detail::provinces.begin() + 1, detail::provinces.end(), [&quads](province& p) {
			if (p.intersect.elements.size() > 0)
				intersects_to_triangles(p.intersect, p.bounds.top, quads[get_id(p, fake_lock()).value]);
		});

		{
			OGLLock win(global::lockWindow());
			load_display_data(quads, brder, win);
		}
	} else {
		fake_load_display_data(brder);
	}

	global::province_connections.elements.shrink_to_fit();
	global::province_connections.index.shrink_to_fit();
}

void prov_map_f_save(IN_P(sqlite3) db, IN(r_lock) l, IN(flat_multimap<edge, std::vector<sf::Vector2<short>>>) brder) {
	sqlite3_exec(db, "DELETE FROM prov_map", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "DELETE FROM borders", nullptr, nullptr, nullptr);

	stmtwrapper add_prov(db, "INSERT INTO prov_map (id, name, climate, terrain, intersects, bounds) VALUES (?1, ?2, ?3, ?4, ?5, ?6)");

	for (size_t i = 1; i != detail::provinces.size(); ++i) {
		IN(province) p = detail::provinces[i];
		bindings bind(add_prov);
		bind(1, i);
		bind(2, p.name);
		bind(3, p.climate);

		bind(4, p.terrain, sizeof(p.terrain));

		std::vector<std::pair<int, char>> intersectsvector;
		for (size_t ii = 0; ii < p.intersect.row_size(); ++ii) {
			const auto range = p.intersect.get_row(ii);
			for (auto it = range.first; it != range.second; ++it) {
				intersectsvector.emplace_back(it->first, it->second);
			}
			intersectsvector.emplace_back(-1, 0);
		}

		if(intersectsvector.size() > 0)
			intersectsvector.pop_back();

		char* rawdata = new char[(sizeof(short) + sizeof(char)) * intersectsvector.size()];
		size_t off = 0;
		for (const auto& pr : intersectsvector) {
			short* ival = (short*)(rawdata + off);
			char* cval = rawdata + off + sizeof(short);
			*ival = static_cast<short>(pr.first);
			*cval = static_cast<unsigned char>(pr.second);
			off += (sizeof(short) + sizeof(unsigned char));
		}
		bind(5, rawdata, static_cast<int>(intersectsvector.size() * (sizeof(short) + sizeof(char))));

		bind(6, &p.bounds, sizeof(p.bounds));
		add_prov.step();

		delete[] rawdata;
	}

	stmtwrapper add_border(db, "INSERT INTO borders (prova, provb, border) VALUES(?1, ?2, ?3)");

	for (IN(auto) b : brder) {
		bindings bind(add_border);

		bind(1, b.first.first);
		bind(2, b.first.second);
		bind(3, b.second.data(), static_cast<int>(b.second.size() * sizeof(sf::Vector2<short>)));

		add_border.step();
		
	}
}

void char_f_generate(IN_P(sqlite3) db) {
	sqlite3_exec(db, "CREATE TABLE people (id INTEGER PRIMARY KEY, name INTEGER, culture INTEGER, religion INTEGER, \
		dynasty INTEGER, father INTEGER, mother INTEGER, gender INTEGER, \
		born INTEGER, died INTEGER, spouse INTEGER, primt INTEGER, money FLOAT, \
		atrib INTEGER, activity INTEGER, court INTEGER, flags INTEGER, just FLOAT, honorable FLOAT, peaceful FLOAT, mu FLOAT, sigmasq FLOAT)", nullptr, nullptr, nullptr);
}

void char_f_load(IN_P(sqlite3) db, IN(w_lock) l) {
	detail::people.clear();
	clear_living_data(l);
	clear_court(l);


	stmtwrapper count(db, "SELECT COUNT(*) FROM people");
	count.step();
	size_t newsz;
	load_sql_value(newsz, count, 0);
	detail::people.resize(newsz + 1);

	detail::people[0].died = 1;

	stmtwrapper stmt(db, "SELECT id, name, culture, religion, dynasty, father, mother, \
		gender, born, died, spouse, primt, atrib, activity, court, flags, money, just, honorable, peaceful, mu, sigmasq FROM people");

	while(stmt.step()) {
		char_id_t id;
		load_sql_value(id, stmt, 0);
		if (detail::people.size() <= id.value) {
			detail::people.resize(id.value + 1);
		}
		person& p = detail::people[id.value];

		load_sql_value(p.name, stmt, 1);
		
		load_sql_value(p.dynasty, stmt, 4);
		load_sql_value(p.father, stmt, 5);
		load_sql_value(p.mother, stmt, 6);
		load_sql_value(p.gender, stmt, 7);
		load_sql_value(p.born, stmt, 8);
		load_sql_value(p.died, stmt, 9);
		load_sql_value(p.spouse, stmt, 10);
		load_sql_value(p.primetitle, stmt, 11);


		if (p.died == 0) {
			untitled_data* ld;
			create_untitled(id, ld, l);

			sqlite_int64 statvals = sqlite3_column_int64(stmt, 12);
			ld->stats.analytic = statvals & 0x7;
			statvals >>= 3;
			ld->stats.martial = statvals & 0x7;
			statvals >>= 3;
			ld->stats.social = statvals & 0x7;
			statvals >>= 3;
			ld->stats.intrigue = statvals & 0x7;
			statvals >>= 3;
			ld->attrib.load(statvals);

			load_sql_value(ld->culture, stmt, 2);
			load_sql_value(ld->religion, stmt, 3);

			load_sql_value(ld->activity, stmt, 13);
			load_sql_value(ld->a_court, stmt, 14);
			ld->flags = P_FLAG_VALID | static_cast<unsigned short>(sqlite3_column_int64(stmt, 15));
			load_sql_value(ld->wealth, stmt, 16);
			load_sql_value(ld->p_just, stmt, 17);
			load_sql_value(ld->p_honorable, stmt, 18);
			load_sql_value(ld->p_peaceful, stmt, 19);

			load_sql_value(ld->mu, stmt, 20);
			load_sql_value(ld->sigma_sq, stmt, 21);

			if (valid_ids(ld->a_court)) {
				insert_to_court(id, ld->a_court, l);
			}
		}
	}
}

void char_f_save(IN_P(sqlite3) db, IN(r_lock) l) {
	sqlite3_exec(db, "DELETE FROM people", nullptr, nullptr, nullptr);

	stmtwrapper add_ch(db, "INSERT INTO people (id, name, culture, religion, dynasty, father, mother, \
		gender, born, died, spouse, primt, atrib, activity, court, flags, money, just, honorable, peaceful, mu, sigmasq) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, \
		?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21, ?22)");

	for (char_id i = 1; i != detail::people.size(); ++i) {
		IN(person) p = detail::people[i];

		bindings bind(add_ch);

		bind(1, i);
		bind(2, p.name);
		
		bind(5, p.dynasty);
		bind(6, p.father);
		bind(7, p.mother);
		bind(8, p.gender);
		bind(9, p.born);
		bind(10, p.died);
		bind(11, p.spouse);
		bind(12, p.primetitle);

		
		if (with_udata(char_id_t(i), l, true, [&bind](IN(udata) d) noexcept {
			bind(3, d.culture);
			bind(4, d.religion);

			sqlite_int64 statvals = d.attrib.to_int();
			statvals <<= 3;
			statvals |= d.stats.intrigue;
			statvals <<= 3;
			statvals |= d.stats.social;
			statvals <<= 3;
			statvals |= d.stats.martial;
			statvals <<= 3;
			statvals |= d.stats.analytic;

			bind(13, statvals);

			bind(14, d.activity);
			bind(15, d.a_court);
			bind(16, d.flags);
			bind(17, d.wealth);
			bind(18, d.p_just);
			bind(19, d.p_honorable);
			bind(20, d.p_peaceful);
			bind(21, d.mu);
			bind(22, d.sigma_sq);
			return false;
		})) {
			if (p.died == 0) {
				bind(13, global_store.get_int());
				bind(14, global::currentday);
				bind(15, administration::NONE);
				bind(16, P_FLAG_VALID);
				bind(17, 0.0);
				bind(18, 0.5);
				bind(19, 0.5);
				bind(20, 0.5);
				bind(21, 0.0);
				bind(22, 0.0);
			} else {
				bind(13);
				bind(14);
				bind(15);
				bind(16);
				bind(17);
				bind(18);
				bind(19);
				bind(20);
				bind(21);
				bind(22);
			}
		}
		
		add_ch.step();
	}
}

void file_generate(IN_P(sqlite3) db) {
	titles_f_generate(db);
	cul_f_generate(db);
	rel_f_generate(db);
	dyn_f_generate(db);
	prov_f_generate(db);
	prov_map_f_generate(db);
	char_f_generate(db);
	admin_f_generate(db);
	control_f_generate(db);
	interest_f_generate(db);
	expense_f_generate(db);

	sqlite3_exec(db, "CREATE TABLE globals (id INTEGER PRIMARY KEY, mastertext BLOB, day INTEGER)", nullptr, nullptr, nullptr);
}

void file_load(IN_P(sqlite3) db, IN(w_lock) l, bool no_ui) {
	sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

	prov_map_f_load(db, l, no_ui);
	prov_f_load(db, l);
	
	titles_f_load(db, l);
	control_f_load(db, l);

	cul_f_load(db, l);
	rel_f_load(db, l);
	dyn_f_load(db, l);

	char_f_load(db, l);
	admin_f_load(db, l);
	expense_f_load(db, l);

	interest_f_load(db, l);

	for (auto p : global::living) {
		with_udata(p, l, [p, &l](INOUT(udata) d) noexcept {
			d.income_estimate = global::force_project_income(p, l);
		});
	}

	stmtwrapper getplayer(db, "SELECT id, mastertext, day FROM globals");
	getplayer.step();
	load_sql_value(global::playerid, getplayer, 0);
	master_string_data.assign((char*)sqlite3_column_blob(getplayer, 1), sqlite3_column_bytes(getplayer, 1));
	load_sql_value(global::currentday, getplayer, 2);

	sqlite3_exec(db, "END TRANSACTION", nullptr, nullptr, nullptr);

	global::interested.add(global::playerid.value);

	global::mapmode = MAP_MODE_POL;
	global::setFlag(FLG_MAP_UPDATE | FLG_BORDER_UPDATE);
}

void file_save_scenario(IN_P(sqlite3) db, IN(r_lock) l, IN(flat_multimap<edge, std::vector<sf::Vector2<short>>>) brder) {
	prov_map_f_save(db, l, brder);
	file_save(db, l);
}

void file_save(IN_P(sqlite3) db, IN(r_lock) l) {
	sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

	titles_f_save(db, l);
	cul_f_save(db, l);
	rel_f_save(db, l);
	dyn_f_save(db, l);
	prov_f_save(db, l);

	char_f_save(db, l);
	admin_f_save(db, l);
	control_f_save(db, l);
	interest_f_save(db, l);
	expense_f_save(db, l);

	sqlite3_exec(db, "DELETE FROM globals", nullptr, nullptr, nullptr);
	stmtwrapper addplayer(db, "INSERT INTO globals (id, mastertext, day) VALUES (?1, ?2, ?3)");
	{
		bindings bind(addplayer);
		bind(1, global::playerid);
		bind(2, master_string_data.data(), master_string_data.size());
		bind(3, global::currentday);

		addplayer.step();
	}

	sqlite3_exec(db, "END TRANSACTION", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "VACUUM", nullptr, nullptr, nullptr);
}