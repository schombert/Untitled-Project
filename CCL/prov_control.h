#pragma once
#include "globalhelpers.h"
#include "structs.hpp"

struct controlrecord {
	double tax;
	prov_id_t province;
	admin_id_t ad_controller;
	unsigned short since; //year

	void construct() noexcept {}
	bool is_clear() const noexcept {
		return !valid_ids(province);
	}
	void set_clear() noexcept {
		province = prov_id_t();
	}
};

struct djrecord {
	prov_id_t province;
	title_id_t owner;
	unsigned short lastcontrolled; //year

	void construct() noexcept {}
	bool is_clear() const noexcept {
		return !valid_ids(province);
	}
	void set_clear() noexcept {
		province = prov_id_t();
	}
};

namespace global {
	extern multiindex<prov_id_t, unsigned int> provtocontrol;
	extern multiindex<prov_id_t, unsigned int> provtowner;

	extern multiindex<admin_id_t, unsigned int> admintocontrol;
	extern multiindex<title_id_t, unsigned int> titletoowned;

	extern v_pool<controlrecord> control_pool;
	extern v_pool<djrecord> dj_pool;
};

namespace global {
	void add_dj(IN(djrecord) r, IN(w_lock) l) noexcept;
	void add_control(IN(controlrecord) r, IN(w_lock) l) noexcept;
	void add_dj(djrecord&& r, IN(w_lock) l) noexcept;
	void add_control(controlrecord&& r, IN(w_lock) l) noexcept;

	void remove_dj(prov_id_t p, title_id_t t, IN(w_lock) l) noexcept;
	void remove_control_a(prov_id_t p, admin_id_t t, IN(w_lock) l) noexcept;

	void remove_all_prov_dj(prov_id_t p, IN(w_lock) l) noexcept;
	void remove_all_prov_control(prov_id_t p, IN(w_lock) l) noexcept;

	void remove_all_title_dj(title_id_t t, IN(w_lock) l) noexcept;
	void remove_all_admin_control(admin_id_t t, IN(w_lock) l) noexcept;

	template<typename T>
	void for_all_control(IN(g_lock) l, IN(T) f) noexcept {
		control_pool.for_each(l, f);
	}
	template<typename T>
	void for_all_dj(IN(g_lock) l, IN(T) f) noexcept {
		dj_pool.for_each(l, f);
	}
	template<typename T>
	void enum_control_by_prov(prov_id_t p, IN(g_lock) l, IN(T) f) noexcept {
		provtocontrol.for_each(p, l, [&f, &l](unsigned int h) {
			INOUT(controlrecord) r = control_pool.get(h, l);
			f(r);
		});
	}
	template<typename T>
	void enum_dj_by_prov(prov_id_t p, IN(g_lock) l, IN(T) f) noexcept {
		provtowner.for_each(p, l, [&f, &l](unsigned int h) {
			INOUT(djrecord) r = dj_pool.get(h, l);
			f(r);
		});
	}
	template<typename T>
	void enum_control_by_admin(admin_id_t t, IN(g_lock) l, IN(T) f) noexcept {
		admintocontrol.for_each(t, l, [&f, &l](unsigned int h) {
			INOUT(controlrecord) r = control_pool.get(h, l);
			f(r);
		});
	}
	template<typename T>
	void enum_excl_control_by_admin(admin_id_t t, IN(g_lock) l, IN(T) f) noexcept {
		admintocontrol.for_each(t, l, [&f, &l](unsigned int h) {
			INOUT(controlrecord) r = control_pool.get(h, l);
			if (provtocontrol.count(r.province, l) == 1) {
				f(r);
			}
		});
	}
	template<typename T>
	void enum_dj_by_title(title_id_t t, IN(g_lock) l, IN(T) f) noexcept {
		titletoowned.for_each(t, l, [&f, &l](unsigned int h) {
			INOUT(djrecord) r = dj_pool.get(h, l);
			f(r);
		});
	}
};

inline bool prov_has_title(prov_id_t p) noexcept {
	return valid_ids(p) & (p.value <= province::last_titled_p);
}
inline title_id_t prov_to_title(prov_id_t p) noexcept {
	return prov_has_title(p) ? title_id_t(p.value) : title_id_t();
}
inline bool title_has_prov(title_id_t t) noexcept {
	return valid_ids(t) & (t.value <= province::last_titled_p);
}
inline prov_id_t title_to_prov(title_id_t t) noexcept {
	return title_has_prov(t) ? prov_id_t(t.value) : prov_id_t();
}

void update_dj(IN(w_lock) l) noexcept;
bool is_dj(prov_id_t p, title_id_t t, IN(g_lock) l) noexcept;
bool is_controlled_by_a(prov_id_t p, admin_id_t t, IN(g_lock) l) noexcept;
bool is_controlled_under_t(prov_id_t p, title_id_t t, IN(g_lock) l) noexcept;
size_t dejure_year(title_id_t title_for, IN(controlrecord) cr) noexcept;
size_t dejure_expires(IN(djrecord) dr) noexcept;

void controlled_by_admin(admin_id_t adm, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) noexcept;
void owned_by_title(title_id_t t, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) noexcept;
void controlled_by_admin(admin_id_t adm, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) noexcept;
void owned_by_title(title_id_t t, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) noexcept;

void control_f_generate(IN_P(sqlite3) db) noexcept;
void control_f_load(IN_P(sqlite3) db, IN(w_lock) l) noexcept;
void control_f_save(IN_P(sqlite3) db, IN(r_lock) l) noexcept;

admin_id_t lowest_admin_in_prov(prov_id_t p, IN(g_lock) l) noexcept;
admin_id_t highest_admin_in_prov(prov_id_t p, IN(g_lock) l) noexcept;