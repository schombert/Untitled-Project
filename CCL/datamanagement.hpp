#pragma once
#include "globalhelpers.h"
#include "structs.hpp"

#define _DEBUG_ON_

class province;

class plandata;
class administration;

class newwar;

class stmtwrapper {
public:
	sqlite3_stmt* stmt;
	stmtwrapper() : stmt(nullptr) {};
	stmtwrapper(sqlite3* const db, const char* sql) : stmt(nullptr) {
#ifdef _DEBUG_ON_
		int res =
#endif
		sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
#ifdef _DEBUG_ON_
		if (res != SQLITE_OK) {
			OutputDebugStringA((std::string(sql) + " SQL PREP ERR: ").c_str());
			OutputDebugStringA(sqlite3_errstr(res));
		}
#endif
	};
	stmtwrapper(const stmtwrapper&) = delete;
	stmtwrapper(stmtwrapper &&in) {
		stmt = in.stmt;
		in.stmt = nullptr;
	}
	~stmtwrapper() {
		if (stmt)
			sqlite3_finalize(stmt);
	};
	bool step() {
		int res = sqlite3_step(stmt);
		if (res == SQLITE_ROW)
			return true;
#ifdef _DEBUG_ON_
		if (res == SQLITE_OK || res == SQLITE_DONE)
			return false;
		OutputDebugStringA((std::string(sqlite3_sql(stmt)) + " SQL EXEC ERR: ").c_str());
		OutputDebugStringA(sqlite3_errstr(res));
#endif
		return false;

	}
	operator sqlite3_stmt*() const {
		return stmt;
	};
	stmtwrapper copy(sqlite3* const db) const {
		return stmtwrapper(db, sqlite3_sql(stmt));
	}
};

class bindings {
public:
	sqlite3_stmt* stmt;
	bindings(sqlite3_stmt* s) : stmt(s) {
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
	};
	
	bindings(const bindings& b) = delete;
	~bindings() {
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
	}
	template <typename T>
	void operator() (const int indx, T val) noexcept {
		if (val.value != T::NONE)
			sqlite3_bind_int64(stmt, indx, val.value);
		else
			sqlite3_bind_null(stmt, indx);
	}
	template <>
	void operator() (const int indx, const sqlite_int64 val) noexcept {
		sqlite3_bind_int64(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const size_t val) noexcept {
		sqlite3_bind_int64(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const int val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const short val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const char val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const unsigned int val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const unsigned short val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const unsigned char val) noexcept {
		sqlite3_bind_int(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const double val) noexcept {
		sqlite3_bind_double(stmt, indx, val);
	}
	template <>
	void operator() (const int indx, const float val) noexcept {
		sqlite3_bind_double(stmt, indx, static_cast<double>(val));
	}
	void operator() (const int indx) noexcept {
		sqlite3_bind_null(stmt, indx);
	}
	template <>
	void operator()<sref>(const int indx, sref r) noexcept {
		if(r.to_int() != 0)
			sqlite3_bind_int64(stmt, indx, r.to_int());
		else
			sqlite3_bind_null(stmt, indx);
	}
	void operator() (const int indx, const void* data, size_t size) noexcept {
		sqlite3_bind_blob(stmt, indx, data, static_cast<int>(size), SQLITE_STATIC);
	}
	
};

inline void load_sql_value(IN_P(void) destination, size_t bytes, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	bytes = std::min(bytes, static_cast<size_t>(sqlite3_column_bytes(stmt, indx)));
	memcpy(destination, sqlite3_column_blob(stmt, indx), bytes);
}

template<typename T>
inline void load_sql_value(INOUT(T) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	if (sqlite3_column_type(stmt, indx) != SQLITE_NULL)
		destination.value = static_cast<decltype(destination.value)>(sqlite3_column_int64(stmt, indx));
	else
		destination.value = T::NONE;
}

template<typename V, typename A>
inline void load_sql_value(INOUT(std::vector<V, A>) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	const size_t elements = static_cast<size_t>(sqlite3_column_bytes(stmt, indx)) / sizeof(V);
	destination.resize(elements);
	memcpy(destination.data(), sqlite3_column_blob(stmt, indx), elements * sizeof(V));
}

template<>
inline void load_sql_value<std::string>(INOUT(std::string) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination.assign((char*)(sqlite3_column_text(stmt, indx)));
}

template<>
inline void load_sql_value<sref>(INOUT(sref) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination.load(sqlite3_column_int64(stmt, indx));
}

template<>
inline void load_sql_value<std::wstring>(INOUT(std::wstring) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination.assign((wchar_t*)(sqlite3_column_text16(stmt, indx)));
}

template<>
inline void load_sql_value<float>(INOUT(float) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<float>(sqlite3_column_double(stmt, indx));
}
template<>
inline void load_sql_value<double>(INOUT(double) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = sqlite3_column_double(stmt, indx);
}
template<>
inline void load_sql_value<int>(INOUT(int) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = sqlite3_column_int(stmt, indx);
}
template<>
inline void load_sql_value<unsigned int>(INOUT(unsigned int) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<unsigned int>(sqlite3_column_int64(stmt, indx));
}
template<>
inline void load_sql_value<long long>(INOUT(long long) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = sqlite3_column_int64(stmt, indx);
}
template<>
inline void load_sql_value<unsigned long long>(INOUT(unsigned long long) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<unsigned long long>(sqlite3_column_int64(stmt, indx));
}
template<>
inline void load_sql_value<short>(INOUT(short) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<short>(sqlite3_column_int(stmt, indx));
}
template<>
inline void load_sql_value<unsigned short>(INOUT(unsigned short) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<unsigned short>(sqlite3_column_int(stmt, indx));
}
template<>
inline void load_sql_value<char>(INOUT(char) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<char>(sqlite3_column_int(stmt, indx));
}
template<>
inline void load_sql_value<unsigned char>(INOUT(unsigned char) destination, IN_P(sqlite3_stmt) stmt, const int indx) noexcept {
	destination = static_cast<unsigned char>(sqlite3_column_int(stmt, indx));
}

#define BLOB_BY_VEC


namespace global {
	title_id_t get_prime_title(char_id_t id);
	prov_id_t get_admin_capital(admin_id_t id, IN(g_lock) l);
	std::string character_name(char_id_t id);
	std::string title_name(title_id_t id);
	std::string province_name(prov_id_t id);
	std::wstring w_character_name(char_id_t id);
	std::wstring w_title_name(title_id_t id);
	std::wstring w_province_name(prov_id_t id);
	bool independant_policy_a(admin_id_t adm, IN(g_lock) l);
	bool independant_policy_a(IN(administration) a, IN(g_lock) l);
	bool ch_has_independant_policy(char_id_t p, IN(g_lock) l);
	void get_spouses(char_id_t id, INOUT(std::vector<char_id_t>) vec);
	void get_children(char_id_t id, INOUT(std::vector<char_id_t>) vec) noexcept;
	void get_living_children(char_id_t id, INOUT(std::vector<char_id_t>) vec);
	void get_siblings(char_id_t id, INOUT(std::vector<char_id_t>) vec);
	void get_living_siblings(char_id_t id, INOUT(std::vector<char_id_t>) vec) noexcept;

	void get_living_family(char_id_t id, INOUT(std::vector<char_id_t>) vec);
	void get_living_family(char_id_t id, INOUT(cvector<char_id_t>) vec);
	void get_realm(char_id_t id, INOUT(cvector<char_id_t>) vec, IN(g_lock) l);
	void get_realm(char_id_t id, INOUT(std::vector<char_id_t>) vec, IN(w_lock) l);

	void get_recent_people(INOUT(std::vector<char_id_t>) vec);
	void get_recent_people(INOUT(cvector<char_id_t>) vec);

	void get_nom_controlled(char_id_t id, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) noexcept;
	void get_nom_controlled(char_id_t id, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) noexcept;
	bool has_territory(char_id_t id, IN(g_lock) l);
	void get_dj_controlled_by_t(title_id_t id, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l);
	void get_dj_controlled_by_t(title_id_t id, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l);
	void get_controlled_by_admin(admin_id_t id, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l) noexcept;
	void get_controlled_by_admin(admin_id_t id, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l) noexcept;
	void get_dj_controlled(char_id_t id, INOUT(cvector<prov_id_t>) provs, IN(g_lock) l);
	void get_dj_controlled(char_id_t id, INOUT(std::vector<prov_id_t>) provs, IN(w_lock) l);
	void get_dyn_members(dyn_id_t id, INOUT(std::vector<char_id_t>) alive, INOUT(std::vector<char_id_t>) dead);

	char_id_t get_t_holder(title_id_t title);
	char_id_t get_character_leige(char_id_t id, IN(g_lock) l);

	admin_id_t get_top_title(admin_id_t id, IN(g_lock) l);
	bool is_prov_controlled(prov_id_t prov, IN(g_lock) l);
	
	void list_vassals_a(admin_id_t adm, INOUT(cvector<admin_id_t>) lst, IN(g_lock) l) noexcept;
	void list_vassals_a(admin_id_t adm, INOUT(std::vector<admin_id_t>) lst, IN(w_lock) l) noexcept;

	double force_project_income(char_id_t charid, IN(g_lock) l);
	double project_income(char_id_t charid, IN(g_lock) l);
	double get_wealth(char_id_t charid, IN(g_lock) l);
	void get_neighbors(char_id_t self, INOUT(cvector<char_id_t>) v, IN(g_lock) l);
	void get_neighbors(char_id_t self, INOUT(std::vector<char_id_t>) v, IN(w_lock) l);
	char_id_t get_targetable_leige(admin_id_t target, admin_id_t source, IN(g_lock) l);
	

	template<typename T>
	void enum_council(admin_id_t id, INOUT(std::vector<char_id_t, T>) id_list, IN(g_lock) l) {
		id_list.reserve(16);
		enum_council(id, l, [&id_list](char_id_t id) {id_list.push_back(id); });
	}

	void get_court(admin_id_t id, INOUT(std::vector<char_id_t>) id_list, IN(w_lock) l);
	void get_court(admin_id_t id, INOUT(cvector<char_id_t>) id_list, IN(g_lock) l);
	void get_extended_court(char_id_t person, INOUT(cvector<char_id_t>) id_list, IN(g_lock) l);
	void get_extended_court(char_id_t person, INOUT(std::vector<char_id_t>) id_list, IN(w_lock) l);
	void get_nearby_provinces(char_id_t id, INOUT(cvector<prov_id_t>) list, IN(g_lock) l);
	void get_nearby_independant(char_id_t id, INOUT(cvector<char_id_t>) list, IN(g_lock) l);
	void get_nearby_independant(char_id_t id, INOUT(cvector<char_id_t>) list, int rank, IN(g_lock) l);
	void get_nearby_provinces(char_id_t id, INOUT(std::vector<prov_id_t>) list, IN(w_lock) l);
	void get_nearby_independant(char_id_t id, INOUT(std::vector<char_id_t>) list, int rank, IN(w_lock) l);
	void get_nearby_independant(char_id_t id, INOUT(std::vector<char_id_t>) list, IN(w_lock) l);
	void get_adjacent_provinces(char_id_t id, INOUT(cvector<prov_id_t>) list, IN(g_lock) l);
	void get_adjacent_provinces(char_id_t id, INOUT(std::vector<prov_id_t>) list, IN(w_lock) l);
	void get_adjacent_independant(char_id_t id, INOUT(cvector<char_id_t>) list, int rank, IN(g_lock) l);
	void get_adjacent_independant(char_id_t id, INOUT(std::vector<char_id_t>) list, int rank, IN(w_lock) l);
	std::wstring get_expanded_name(char_id_t id);
	unsigned int days_between(char_id_t a, char_id_t b, IN(g_lock) l);
	bool is_vassal_of(char_id_t lesser, char_id_t greater, IN(g_lock) l);
	bool is_marriable(char_id_t id);
	bool are_marriable(char_id_t a, char_id_t b);
	bool are_close_family(char_id_t a, char_id_t b);
	prov_id_t get_home_province(char_id_t person, IN(g_lock) l);
	std::wstring get_composed_title(title_id_t id, int type);

	template<typename T>
	void leige_chain(admin_id_t a, IN(g_lock) l, IN(T) f) noexcept {
		while (valid_ids(a)) {
			f(a);
			a = get_object(a, l).leige.value;
		}
	}
	template<typename T>
	void enum_vassals(admin_id_t adm, IN(g_lock) l, IN(T) func) noexcept {
		leigetoadmin.for_each(adm, l, func);
	}
	void get_vassals(INOUT(std::vector<char_id_t>) v, admin_id_t adm, IN(w_lock) l) noexcept;
	void get_vassals(INOUT(cvector<char_id_t>) v, admin_id_t adm, IN(g_lock) l) noexcept;
	template<typename T>
	void enum_council(admin_id_t id, IN(g_lock) l, IN(T) f) noexcept {
		const auto at = get_object(id, l).associated_title;
		const char_id_t hid = get_object(at).holder;
		if (!valid_ids(hid))
			return;

		//TODO: prevent duplicates
		leigetoadmin.for_each(id, l, [&f, &l](const admin_id_t vas) noexcept {
			f(get_object(vas, l).executive);
		});

		sub_titles.for_each(at, l, [&f](IN(sub_title) t) {
			f(t.holder);
		});

		IN(auto) hlder = get_object(hid);
		IN(auto) sp = get_object(hlder.spouse);
		if (sp.died == 0 && !valid_ids(sp.primetitle)) {
			f(hlder.spouse);
		}
	}
};

namespace global {
	void delta_money(char_id_t id, double change, IN(g_lock) l);
	void get_tax_income(IN(g_lock) l);
	void set_holder(title_id_t t, char_id_t c, IN(w_lock) l);
	void update_prov_control(prov_id_t p, admin_id_t lowest, IN(w_lock) l);
	void update_prime_t(char_id_t ch, IN(g_lock) l);
	void update_capital(admin_id_t id, IN(w_lock) l);
	void update_title_stats(IN(g_lock) l);
	void monthly_update_title_stats(IN(g_lock) l);
};

void titles_f_generate(IN_P(sqlite3) db);
void titles_f_load(IN_P(sqlite3) db, IN(w_lock) l);
void titles_f_save(IN_P(sqlite3) db, IN(r_lock) l);

void cul_f_generate(IN_P(sqlite3) db);
void cul_f_load(IN_P(sqlite3) db, IN(w_lock) l);
void cul_f_save(IN_P(sqlite3) db, IN(r_lock) l);

void rel_f_generate(IN_P(sqlite3) db);
void rel_f_load(IN_P(sqlite3) db, IN(w_lock) l);
void rel_f_save(IN_P(sqlite3) db, IN(r_lock) l);

void dyn_f_generate(IN_P(sqlite3) db);
void dyn_f_load(IN_P(sqlite3) db, IN(w_lock) l);
void dyn_f_save(IN_P(sqlite3) db, IN(r_lock) l);

void prov_f_generate(IN_P(sqlite3) db);
void prov_f_load(IN_P(sqlite3) db, IN(w_lock) l);
void prov_f_save(IN_P(sqlite3) db, IN(r_lock) l);

void prov_map_f_generate(IN_P(sqlite3) db);
void prov_map_f_load(IN_P(sqlite3) db, IN(w_lock) l, bool no_ui);
void prov_map_f_save(IN_P(sqlite3) db, IN(r_lock) l, IN(flat_multimap<edge, std::vector<sf::Vector2<short>>>) brder);

void char_f_generate(IN_P(sqlite3) db);
void char_f_load(IN_P(sqlite3) db, IN(w_lock) l);
void char_f_save(IN_P(sqlite3) db, IN(r_lock) l);

void file_generate(IN_P(sqlite3) db);
void file_load(IN_P(sqlite3) db, IN(w_lock) l, bool no_ui = false);
void file_save(IN_P(sqlite3) db, IN(r_lock) l);
void file_save_scenario(IN_P(sqlite3) db, IN(r_lock) l, IN(flat_multimap<edge, std::vector<sf::Vector2<short>>>) brder);