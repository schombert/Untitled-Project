#pragma once
#include "globalhelpers.h"
#include "laws.h"

struct interest_relation_contents {
	static constexpr unsigned char TAG_NONE = 0;
	static constexpr unsigned char TAG_TARGET = 1;
	static constexpr unsigned char TAG_THREAT = 2;
	static constexpr unsigned char TAG_PTHREAT = 3;

	unsigned char tag_a;
	unsigned char tag_b;
	interest_relation_contents(unsigned char t, unsigned char u) : tag_a(t), tag_b(u) {};
	interest_relation_contents() : tag_a(TAG_NONE), tag_b(TAG_NONE) {};
};

float q_aggr_value(char_id_t id, IN(g_lock) l) noexcept;

struct interest_info_pack {
	float mu = 0.0f;
	float sigmasq = 0.0f;
	float d_mu = 0.0f;
	float d_sigmasq = 0.0f;
	float aggr;

	interest_info_pack(char_id_t id, IN(g_lock) l) noexcept : aggr(q_aggr_value(id, l)) {
		get_force_estimate(id, mu, sigmasq, l);
		get_defensive_force_estimate(id, d_mu, d_sigmasq, l);
	}
};

extern flat_map<ordered_pair<char_id_t>, interest_relation_contents> interest_relations;
extern flat_multimap<char_id_t, char_id_t> interested_in; // bidirectional mapping
extern single_index_t<char_id_t, float> defensive_mu; //saved def_mu_value

void update_interests_char_id(char_id_t old, char_id_t n, IN(w_lock) l) noexcept;
void update_territory_lost(char_id_t id, IN(w_lock) l) noexcept;
void update_territory_gained(char_id_t id, IN(w_lock) l) noexcept;
void update_expectation_changed(char_id_t id, double ptsraised, IN(w_lock) l) noexcept;
void update_defensive_pact_gained(char_id_t id, IN(w_lock) l) noexcept;
void update_defensive_pact_lost(char_id_t id, IN(w_lock) l) noexcept;
interest_relation_contents tags_from_ids(char_id_t a, IN(interest_info_pack) avals, admin_id_t a_leige, char_id_t b, IN(g_lock) l) noexcept;
void init_interests(IN(w_lock) l) noexcept; //call only after living data & force estimates are set up
float max_threat_percentage(char_id_t id, IN(g_lock) l) noexcept;
std::pair<float, float> max_p_threat_percentage(char_id_t id, float minus_defensive, IN(g_lock) l) noexcept; // pair of currenent threat , potential threat
float mu_estimate(char_id_t id, IN(g_lock) l) noexcept;
float def_mu_estimate(char_id_t id, IN(g_lock) l) noexcept;
float def_against_mu_estimate(char_id_t id, char_id_t against, IN(g_lock) l) noexcept;
unsigned char interest_status_of(char_id_t a, char_id_t b, IN(g_lock) l) noexcept;
void get_interest_totals(char_id_t id, INOUT(int) num_threats, INOUT(int) num_targets, INOUT(int) num_pthreats, IN(g_lock) l) noexcept;
unsigned int total_threats(char_id_t id, IN(g_lock) l) noexcept;
char_id_t get_random_target(char_id_t id, IN(g_lock) l) noexcept;
char_id_t get_random_threat(char_id_t id, IN(g_lock) l) noexcept;

void interest_f_generate(IN_P(sqlite3) db) noexcept;
void interest_f_load(IN_P(sqlite3) db, IN(w_lock) l) noexcept;
void interest_f_save(IN_P(sqlite3) db, IN(r_lock) l) noexcept;