#pragma once
#include "globalhelpers.h"
#include "pacts.h"
#include "living_data.h"

#define REL_FEELING 1
#define REL_PEACE 2
#define REL_PACT 3

class relation {
public:
	union {
		size_t id;
		struct {
			unsigned int date;
			bool fromprimary;
		} peace;
	} typedata;
	char_id_t primary;
	char_id_t secondary;
	unsigned char type;

	bool operator ==(IN(relation) r) const noexcept {
		return primary == r.primary && secondary == r.secondary && type == r.type;
	}

	void construct() noexcept {}
	void set_clear() noexcept {
		type = 0;
	}
	bool is_clear() const noexcept {
		return type == 0;
	}
};


struct feeling_data {
public:
	__int8 ptrend = 0;
	__int8 strend = 0;
	unsigned char flags = 0;
	__int8 favors = 0;
};

namespace global {
	extern multiindex<char_id_t, unsigned int> chtorelations;
	extern multiindex<ordered_pair<char_id_t>, unsigned int> pairtorelations;
	extern v_pool<relation> relations_pool;

	extern multiindex<char_id_t, char_id_t> ch_to_feeling_relations;
	extern single_index_t<ordered_pair<char_id_t>, feeling_data> feeling_relations;
};

void add_relation(IN(relation) r,  IN(w_lock) l) noexcept;
void add_relation(relation&& r, IN(w_lock) l) noexcept;
void remove_all_relations(char_id_t c,  IN(w_lock) l) noexcept;

int get_feeling(char_id_t prim, char_id_t secon,  IN(g_lock) l) noexcept;
int get_mutual_feeling(char_id_t a, char_id_t b,  IN(g_lock) l) noexcept;

int owed_favors(char_id_t favors_from, char_id_t favors_to, IN(g_lock) l) noexcept;
void add_favor(char_id_t favors_from, char_id_t favors_to, IN(w_lock) l) noexcept;
bool call_in_favor(char_id_t favors_from, char_id_t favors_to, IN(w_lock) l) noexcept;
void dishonor_favor(char_id_t favor_from, char_id_t favor_to, IN(w_lock) l) noexcept;
double dishonor_favor_cost(char_id_t favor_from, char_id_t favor_to, IN(g_lock) l) noexcept;
double value_of_favor_from(char_id_t favor_to, IN(udata) td, char_id_t favor_from, IN(g_lock) l) noexcept;

void adjust_relation(char_id_t prim, char_id_t secon, __int8 value, IN(w_lock) l) noexcept;
void adjust_relation_symmetric(char_id_t prim, char_id_t secon, __int8 value, IN(w_lock) l) noexcept;
void clear_finished_pact(INOUT(pact_data) pact, IN(w_lock) l) noexcept;

char_id_t get_random_hated_or_enemy(char_id_t c, IN(g_lock) l) noexcept;

unsigned int peace_until(char_id_t a, char_id_t b,  IN(g_lock) l) noexcept;
bool have_peace(char_id_t a, char_id_t b, IN(g_lock) l) noexcept;
void add_peace_treaty(char_id_t a, char_id_t b, unsigned int until,  IN(w_lock) l) noexcept;
bool adjust_for_attack(char_id_t attacker, char_id_t b, IN(w_lock) l) noexcept;
double honor_cost_for_attack(char_id_t a, char_id_t b, IN(g_lock) l);

double bias_by_envoy(char_id_t source, char_id_t envoy, char_id_t destination, IN(g_lock) l) noexcept; // range from -4.5 to 3.0

float base_opinon(char_id_t from, char_id_t to, IN(g_lock) l) noexcept;
float opinion(char_id_t from, char_id_t to, IN(g_lock) l) noexcept; // range from -1.0 to 1.0
float opinion(char_id_t from, IN(udata) fdata, char_id_t to, IN(g_lock) l) noexcept;
float base_opinon_as_vassal(char_id_t from, IN(udata) fdata, char_id_t to, IN(g_lock) l) noexcept;
float base_opinon_as_independant(char_id_t from, IN(udata) fd, char_id_t to, IN(g_lock) l) noexcept;
float base_opinon_as_generic(prov_id_t source, char_id_t to, IN(g_lock) l) noexcept;

#define BOTH_LIKE(x) (x == 0x000A)
#define PRIM_LIKE(x) ((x & 0x03) == 0x0002)
#define SECON_LIKE(x) ((x & 0x0C) == 0x0008)
#define PRIM_HATE(x) ((x & 0x03) == 0x0001)
#define SECON_HATE(x) ((x & 0x0C) == 0x0004)
#define BOTH_HATE(x) (x == 0x0005)
#define SECON_INDIF(x) ((x & 0x0C) == 0)
#define PRIM_INDIF(x) ((x & 0x03) == 0)
#define NONE_SET(x) (x == 0x0000)
#define SET_PRIM_HATE(x) ((x &= ~0x03) |= 0x0001)
#define SET_SECON_HATE(x) ((x &= ~0x0C) |= 0x0004)
#define SET_PRIM_LIKE(x) ((x &= ~0x03) |= 0x0002)
#define SET_SECON_LIKE(x) ((x &= ~0x0C) |= 0x0008)
#define SET_PRIM_INDIF(x) (x &= ~0x03) 
#define SET_SECON_INDIF(x) (x &= ~0x0C)
#define R_THRESHOLD 5

template<typename T>
void enum_friends(char_id_t c, IN(g_lock) l, IN(T) func) noexcept {
	global::ch_to_feeling_relations.for_each(c, l, [&func, &l, c](char_id_t o) noexcept {
		IN(auto) rel = global::feeling_relations.get(ordered_pair<char_id_t>(c, o), l);
		if (BOTH_LIKE(rel.flags)) {
			func(o);
		}
	});
}

template<typename T>
void enum_enemies(char_id_t c, IN(g_lock) l, IN(T) func) noexcept {
	global::ch_to_feeling_relations.for_each(c, l, [&func, &l, c](char_id_t o) noexcept {
		IN(auto) rel = global::feeling_relations.get(ordered_pair<char_id_t>(c, o), l);
		if (BOTH_HATE(rel.flags)) {
			func(o);
		}
	});
}

template<typename T>
void enum_hated(char_id_t c, IN(g_lock) l, IN(T) func) noexcept {
	global::ch_to_feeling_relations.for_each(c, l, [&func, &l, c](char_id_t o) noexcept {
		IN(auto) rel = global::feeling_relations.get(ordered_pair<char_id_t>(c, o), l);
		if ((!BOTH_HATE(rel.flags)) & (((c < o) & PRIM_HATE(rel.flags)) | ((o < c) & SECON_HATE(rel.flags))))
			func(o);
	});
}

template<typename T>
void enum_hated_or_enemies(char_id_t c, IN(g_lock) l, IN(T) func) noexcept {
	global::ch_to_feeling_relations.for_each(c, l, [&func, &l, c](char_id_t o) noexcept {
		IN(auto) rel = global::feeling_relations.get(ordered_pair<char_id_t>(c, o), l);
		if (((c < o) & PRIM_HATE(rel.flags)) | ((o < c) & SECON_HATE(rel.flags)))
			func(o);
	});
}

template<typename T>
void enum_not_indifferent(char_id_t c, IN(g_lock) l, IN(T) func) noexcept {
	global::ch_to_feeling_relations.for_each(c, l, [&func, &l, c](char_id_t o) noexcept {
		IN(auto) rel = global::feeling_relations.get(ordered_pair<char_id_t>(c, o), l);
		if (((c < o) & !PRIM_INDIF(rel.flags)) | ((o < c) & !SECON_INDIF(rel.flags)))
			func(o);
	});
}

template<typename T>
void enum_peace_treaties(char_id_t c, IN(g_lock) l, IN(T) func) noexcept {
	global::chtorelations.for_each(c, l, [&func, &l, c](unsigned int indx) noexcept {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PEACE) {
			if (r.primary == c && r.typedata.peace.date > global::currentday)
				func(r.secondary, r.typedata.peace.date);
		}
	});
}

template<typename T>
void enum_pacts_for(char_id_t target, IN(g_lock) l, IN(T) func) noexcept {
	global::chtorelations.for_each(target, l, [&func, &l](unsigned int indx) {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PACT) {
			func(pact_id_t(static_cast<pact_id>(r.typedata.id)));
		}
	});
}

template<typename T>
void enum_pacts_between(char_id_t a, char_id_t b, IN(g_lock) l, IN(T) func) noexcept {
	global::pairtorelations.for_each(ordered_pair<char_id_t>(a, b), l, [&func, &l](unsigned int indx) {
		IN(auto) r = global::relations_pool.get(indx, l);
		if (r.type == REL_PACT) {
			func(pact_id_t(static_cast<pact_id>(r.typedata.id)));
		}
	});
}