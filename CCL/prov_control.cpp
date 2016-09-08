#include "globalhelpers.h"
#include "prov_control.h"
#include "living_data.h"
#include "structs.hpp"
#include "ProvPane.h"
#include "laws.h"
#include "datamanagement.hpp"

namespace global {
	multiindex<prov_id_t, unsigned int> provtocontrol;
	multiindex<prov_id_t, unsigned int> provtowner;

	multiindex<admin_id_t, unsigned int> admintocontrol;
	multiindex<title_id_t, unsigned int> titletoowned;

	v_pool<controlrecord> control_pool;
	v_pool<djrecord> dj_pool;
};

bool is_dj(prov_id_t p, title_id_t t, IN(g_lock) l) noexcept {
	if (get_object(t).type == title::COUNTY)
		return t == prov_to_title(p);
	return global::provtowner.for_each_breakable(p, l, [&l, t](unsigned int id) {
		return t == global::dj_pool.get(id, l).owner;
	});
}

bool is_controlled_by_a(prov_id_t p, admin_id_t t, IN(g_lock) l) noexcept {
	return global::provtocontrol.for_each_breakable(p, l, [&l, t](unsigned int id) {
		return global::control_pool.get(id, l).ad_controller == t;
	});
}
bool is_controlled_under_t(prov_id_t p, title_id_t t, IN(g_lock) l) noexcept {
	const auto tholder = get_object(t).holder;
	return global::provtocontrol.for_each_breakable(p, l, [&l, tholder](unsigned int id) {
		return head_of_state(global::control_pool.get(id, l).ad_controller, l) == tholder;
	});
}

size_t dejure_year(title_id_t title_for, IN(controlrecord) cr) noexcept {
	return cr.since + ((5 - get_object(title_for).type) * 25);
}

size_t dejure_expires(IN(djrecord) dr) noexcept {
	return dr.lastcontrolled + 2 * ((5 - get_object(dr.owner).type) * 25);
}

template<typename T, typename L>
void _owned_by_title(title_id_t t, INOUT(std::vector<prov_id_t, T>) provs, IN(L) l) noexcept {
	global::enum_dj_by_title(t, l, [&provs](IN(djrecord) rec) {
		if (std::find(provs.cbegin(), provs.cend(), rec.province) == provs.cend())
			provs.push_back(rec.province);
	});
}

template<typename T, typename L>
void _controlled_by_admin(admin_id_t adm, INOUT(std::vector<prov_id_t, T>) provs, IN(L) l) noexcept {
	global::enum_control_by_admin(adm, l, [&provs](IN(controlrecord) rec) {
		if (std::find(provs.cbegin(), provs.cend(), rec.province) == provs.cend())
			provs.emplace_back(rec.province);
	});
}


void controlled_by_admin(admin_id_t adm, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) noexcept {
	_controlled_by_admin(adm, provs, l);
}
void owned_by_title(title_id_t t, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) noexcept {
	_owned_by_title(t, provs, l);
}
void controlled_by_admin(admin_id_t adm, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) noexcept {
	_controlled_by_admin(adm, provs, l);
}
void owned_by_title(title_id_t t, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) noexcept {
	_owned_by_title(t, provs, l);
}


void update_dj(IN(w_lock) l) noexcept {
	date d = date_offset + days(global::currentday);
	const unsigned short y = d.year() - date_offset.year();

	global::control_pool.for_each(l, [&l, y](INOUT(controlrecord) cr) {
		const auto t = get_object(cr.ad_controller, l).associated_title;
		const auto ttype = get_object(t).type;

		if (ttype != title::COUNTY) {
			if (y == cr.since + ((5 - ttype) * 25) && !is_dj(cr.province, t, l)) {
				global::add_dj(djrecord{cr.province, t, y}, l);
			}
		}
	});

	global::dj_pool.for_each(l, [&l, y](INOUT(djrecord) dr) {
		if (is_controlled_under_t(dr.province, dr.owner, l)) {
			dr.lastcontrolled = y;
		} else if (y == dr.lastcontrolled + 2 * ((5 - get_object(dr.owner).type) * 25)) {
			global::dj_pool.free(dr, l);
		}
	});

	global::provtowner.erase_if(l, [&l](IN(std::pair<prov_id_t, unsigned int>) p) {
		return global::dj_pool.get(p.second, l).is_clear();
	});
	global::titletoowned.erase_if(l, [&l](IN(std::pair<title_id_t, unsigned int>) p) {
		return global::dj_pool.get(p.second, l).is_clear();
	});

	prov_pane_rec.needs_update = true;
}

namespace global {
	void add_dj(IN(djrecord) r, IN(w_lock) l) noexcept {
		const auto h = dj_pool.add(r, l);
		titletoowned.insert(r.owner, h, l);
		provtowner.insert(r.province, h, l);
	}

	void add_control(IN(controlrecord) r, IN(w_lock) l) noexcept {
		const auto h = control_pool.add(r, l);
		admintocontrol.insert(r.ad_controller, h, l);
		provtocontrol.insert(r.province, h, l);
	}

	void add_dj(djrecord&& r, IN(w_lock) l) noexcept {
		const auto o = r.owner;
		const auto p = r.province;
		const auto h = dj_pool.add(std::move(r), l);
		titletoowned.insert(o, h, l);
		provtowner.insert(p, h, l);
	}

	void add_control(controlrecord&& r, IN(w_lock) l) noexcept {
		const auto c = r.ad_controller;
		const auto p = r.province;
		const auto h = control_pool.add(std::move(r), l);
		admintocontrol.insert(c, h, l);
		provtocontrol.insert(p, h, l);
	}

	void remove_dj(prov_id_t p, title_id_t t, IN(w_lock) l) noexcept {
		small_vector<unsigned int, 5> toerase;
		provtowner.for_each(p, l, [&toerase, t, &l](unsigned int h) {
			if(dj_pool.get(h,l).owner == t)
				toerase.emplace_back(h);
		});
		for (const auto e : toerase) {
			dj_pool.free(e, l);
			provtowner.erase(p, e, l);
			titletoowned.erase(t, e, l);
		}
	}

	void remove_control_a(prov_id_t p, admin_id_t t, IN(w_lock) l) noexcept {
		small_vector<unsigned int, 5> toerase;
		provtocontrol.for_each(p, l, [&toerase, t, &l](unsigned int h) {
			if (control_pool.get(h, l).ad_controller == t)
				toerase.emplace_back(h);
		});
		for (const auto e : toerase) {
			control_pool.free(e, l);
			provtocontrol.erase(p, e, l);
			admintocontrol.erase(t, e, l);
		}
	}

	void remove_all_prov_dj(prov_id_t p, IN(w_lock) l) noexcept {
		small_vector<std::pair<unsigned int, title_id_t>, 5> toerase;
		provtowner.for_each(p, l, [&toerase, &l](unsigned int h) {
			toerase.emplace_back(h, dj_pool.get(h, l).owner);
		});
		for (IN(auto) e : toerase) {
			dj_pool.free(e.first, l);
			provtowner.erase(p, e.first, l);
			titletoowned.erase(e.second, e.first, l);
		}
	}

	void remove_all_prov_control(prov_id_t p, IN(w_lock) l) noexcept {
		small_vector<std::pair<unsigned int, admin_id_t>, 5> toerase;
		provtocontrol.for_each(p, l, [&toerase, &l](unsigned int h) {
			toerase.emplace_back(h, control_pool.get(h, l).ad_controller);
		});
		for (IN(auto) e : toerase) {
			control_pool.free(e.first, l);
			provtocontrol.erase(p, e.first, l);
			admintocontrol.erase(e.second, e.first, l);
		}
	}

	void remove_all_title_dj(title_id_t t, IN(w_lock) l) noexcept {
		small_vector<std::pair<unsigned int, prov_id_t>, 5> toerase;
		titletoowned.for_each(t, l, [&toerase, &l](unsigned int h) {
			toerase.emplace_back(h, dj_pool.get(h, l).province);
		});
		for (IN(auto) e : toerase) {
			dj_pool.free(e.first, l);
			provtowner.erase(e.second, e.first, l);
			titletoowned.erase(t, e.first, l);
		}
	}

	void remove_all_admin_control(admin_id_t t, IN(w_lock) l) noexcept {
		small_vector<std::pair<unsigned int, prov_id_t>, 5> toerase;
		admintocontrol.for_each(t, l, [&toerase, &l](unsigned int h) {
			toerase.emplace_back(h, control_pool.get(h, l).province);
		});
		for (IN(auto) e : toerase) {
			control_pool.free(e.first, l);
			provtocontrol.erase(e.second, e.first, l);
			admintocontrol.erase(t, e.first, l);
		}
	}

};


admin_id_t lowest_admin_in_prov(prov_id_t p, IN(g_lock) l) noexcept {
	admin_id_t result;
	int type = title::EMPIRE - 1;
	global::provtocontrol.for_each(p, l, [&l, &result, &type](unsigned int h) {
		IN(auto) cr = global::control_pool.get(h, l);
		const auto ttype = get_object(get_object(cr.ad_controller, l).associated_title).type;
		if (ttype > type) {
			type = ttype;
			result = cr.ad_controller;
		}
	});
	return result;
}

admin_id_t highest_admin_in_prov(prov_id_t p, IN(g_lock) l) noexcept {
	admin_id_t result;
	global::provtocontrol.for_each_breakable(p, l, [&l, &result](unsigned int h) {
		IN(auto) cr = global::control_pool.get(h, l);
		if (!valid_ids(get_object(cr.ad_controller, l).leige)) {
			result = cr.ad_controller;
			return true;
		}
		return false;
	});
	return result;
}

void control_f_generate(IN_P(sqlite3) db) noexcept {
	sqlite3_exec(db, "CREATE TABLE control (province INTEGER, admin INTEGER, since INTEGER, tax FLOAT)", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "CREATE TABLE dejure (province INTEGER, owner INTEGER, lastcontrolled INTEGER)", nullptr, nullptr, nullptr);
}

void control_f_load(IN_P(sqlite3) db, IN(w_lock) l) noexcept {
	global::provtocontrol.clear(l);
	global::provtowner.clear(l);

	global::admintocontrol.clear(l);
	global::titletoowned.clear(l);

	global::control_pool.clear(l);
	global::dj_pool.clear(l);

	{
		stmtwrapper stmt(db, "SELECT province, admin, since, tax FROM control");

		
		while (stmt.step()) {
			controlrecord cr;
			load_sql_value(cr.province, stmt, 0);
			load_sql_value(cr.ad_controller, stmt, 1);
			load_sql_value(cr.since, stmt, 2);
			load_sql_value(cr.tax, stmt, 3);
			global::add_control(cr, l);
		}
	}

	{
		stmtwrapper stmt(db, "SELECT province, owner, lastcontrolled FROM dejure");

		while (stmt.step()) {
			djrecord dj;
			load_sql_value(dj.province, stmt, 0);
			load_sql_value(dj.owner, stmt, 1);
			load_sql_value(dj.lastcontrolled, stmt, 2);
			global::add_dj(dj, l);
		}
	}
}

void control_f_save(IN_P(sqlite3) db, IN(r_lock) l) noexcept {
	sqlite3_exec(db, "DELETE FROM control", nullptr, nullptr, nullptr);
	sqlite3_exec(db, "DELETE FROM dejure", nullptr, nullptr, nullptr);

	stmtwrapper add_con(db, "INSERT INTO control (province, admin, since, tax) VALUES(?1, ?2, ?3, ?4)");

	global::for_all_control(l, [&add_con](IN(controlrecord) r){
		bindings bind(add_con);
		bind(1, r.province);
		bind(2, r.ad_controller);
		bind(3, r.since);
		bind(4, r.tax);
		add_con.step();
	});

	stmtwrapper add_dj(db, "INSERT INTO dejure (province, owner, lastcontrolled) VALUES(?1, ?2, ?3)");

	global::for_all_dj(l, [&add_dj](IN(djrecord) r) {
		bindings bind(add_dj);
		bind(1, r.province);
		bind(2, r.owner);
		bind(3, r.lastcontrolled);
		add_dj.step();
	});
}