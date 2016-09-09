#pragma once
#include "globalhelpers.h"
#include "structs.hpp"

struct wargoal {
	union wgu {
		title_id_t title_for;
		constexpr wgu() : title_for() {};
	} data;

	unsigned char type;

	static constexpr unsigned char WARGOAL_NONE = 0;
	static constexpr unsigned char WARGOAL_DEFENSIVE = 1;
	static constexpr unsigned char WARGOAL_CONQUEST = 2;
	static constexpr unsigned char WARGOAL_DEJURE = 3;

	constexpr wargoal() : type(WARGOAL_NONE), data() {};
	constexpr wargoal(unsigned char t) : type(t), data() {};
	wargoal(std::integral_constant<unsigned char, WARGOAL_DEJURE>, title_id_t t) : type(WARGOAL_DEJURE) {
		data.title_for = t;
	};
};


inline constexpr wargoal make_wg_conquest() {
	return wargoal(wargoal::WARGOAL_CONQUEST);
}
inline wargoal make_wg_dj(title_id_t tf) {
	return wargoal(std::integral_constant<unsigned char, wargoal::WARGOAL_DEJURE>(), tf);
}
inline constexpr wargoal make_wg_def() {
	return wargoal(wargoal::WARGOAL_DEFENSIVE);
}

constexpr wargoal make_wg_none() { return wargoal(wargoal::WARGOAL_NONE); }

struct newtroop {
	unsigned int payafter;
	unsigned short numlevy;
	unsigned short numstanding;

	constexpr newtroop(unsigned int pa, unsigned short l, unsigned short s) : payafter(pa), numlevy(l), numstanding(s) {};
	constexpr newtroop() : payafter(0), numlevy(0), numstanding(0) {};
};

class newwar;

struct newfront {
	float allocation = 0.0f;
	prov_id_t target;
	bool recovering = false;
	bool locked = false;
};

struct participant {
	admin_id_t adm;
	wargoal goal;
	participant() {};
	participant(admin_id_t a, wargoal g) : adm(a), goal(g) {};
};


struct seiging_info {
	war_id_t in_war;
	unsigned int since = 0; // 0 = seige finished,  under occupation
	bool lifting;
	seiging_info() {}
	seiging_info(war_id_t w, unsigned int s, bool l) : in_war(w), since(s), lifting(l) {}
};

class newwar {
public:
	constexpr static war_id NONE = max_value<war_id>::value;

	std::vector<participant> participants;
	// flat_set<prov_id_t> controlled;
	// std::vector<std::pair<prov_id_t, unsigned int>> seiging; //pair prov# + start date

	 admin_id_t primary;
	bool paid = true;

	newwar() {}
	newwar(admin_id_t p) : primary(p) {};
};

class war_pair {
public:
	newwar attacker;
	newwar defender;

	unsigned int date_started = 0;

	std::pair<newfront, newfront> fronts;

	war_pair() {}
	war_pair(admin_id_t a, admin_id_t d, unsigned int day) : attacker(a), defender(d), date_started(day) {};

	void construct(admin_id_t a, admin_id_t d, unsigned int day) noexcept {
		attacker.primary = a;
		defender.primary = d;
		attacker.paid = true;
		defender.paid = true;
		date_started = day;
	}
	bool is_clear() const noexcept {
		return date_started == 0;
	}
	void set_clear() noexcept {
		attacker.participants.clear();
		defender.participants.clear();
		date_started = 0;
	}
};

constexpr double troops_to_points(decltype(std::declval<newtroop>().numlevy) lvy, decltype(std::declval<newtroop>().numstanding) st) noexcept {
	return lvy * 1.0 + st * 2.0;
}

extern multiindex<admin_id_t, war_id_t> wars_involved_in;
extern single_index_t<admin_id_t, newtroop> raised_troops;
extern v_pool_t<war_pair, war_id> war_pool;
extern single_index_t<prov_id_t, seiging_info> occupation_info;


inline const war_pair& get_object(war_id_t id, IN(g_lock) l) noexcept {
	return war_pool.get(id.value, l);
}

inline war_pair& get_object(war_id_t id, IN(w_lock) l) noexcept {
	return war_pool.get(id.value, l);
}

inline war_id_t get_id(IN(war_pair) obj, IN(g_lock) l) noexcept {
	return war_id_t(war_pool.get_index(obj, l));
}

class uiElement;

war_id_t new_war_pair(char_id_t a, wargoal attacker_goal, char_id_t d, INOUT(w_lock) l);

void list_participants(IN(newwar) w, INOUT(flat_set<admin_id_t>) p, IN(w_lock) l) noexcept;
void list_participants(IN(newwar) w, INOUT(cflat_set<admin_id_t>) p, IN(g_lock) l) noexcept;
void list_war_participants_ch(IN(newwar) w, INOUT(cflat_set<char_id_t>) p, IN(g_lock) l);
void list_war_participants_ch(IN(newwar) w, INOUT(flat_set<char_id_t>) p, IN(w_lock) l);
double totalpointsinwar(IN(cflat_set<admin_id_t>) participants, IN(g_lock) l) noexcept;
double totalpointsinwar(IN(flat_set<admin_id_t>) participants, IN(w_lock) l) noexcept;
bool can_enforce(IN(newwar) other, IN(flat_set<admin_id_t>) apart, IN(flat_set<admin_id_t>) bpart, IN(std::vector<prov_id_t>) nom_b_controlled, IN(w_lock) l) noexcept;
bool can_enforce(IN(newwar) other, IN(cflat_set<admin_id_t>) apart, IN(cflat_set<admin_id_t>) bpart, IN(cvector<prov_id_t>) nom_b_controlled, IN(g_lock) l) noexcept;
unsigned int totaltroops(IN(flat_set<admin_id_t>) participants, IN(w_lock) l) noexcept;
unsigned int totaltroops(IN(cflat_set<admin_id_t>) participants, IN(g_lock) l) noexcept;
unsigned int totaltroops(war_id_t war_for, bool agressor, IN(g_lock) l) noexcept;
unsigned int totaltroops(war_id_t war_for, bool agressor, IN(w_lock) l) noexcept;
bool wargoal_possible(IN(wargoal) goal, char_id_t ch_for, IN(flat_set<admin_id_t>) against, IN(w_lock) l) noexcept;
double wargoal_value(IN(wargoal) goal, admin_id_t adm_for, IN(flat_set<admin_id_t>) against, IN(w_lock) l);
bool wargoal_possible(IN(wargoal) goal, char_id_t ch_for, IN(cflat_set<admin_id_t>) against, IN(g_lock) l) noexcept;
double wargoal_value(IN(wargoal) goal, admin_id_t adm_for, IN(cflat_set<admin_id_t>) against, IN(g_lock) l);
double list_goal_provinces_a(INOUT(std::vector<prov_id_t>) lst, IN(wargoal) goal, admin_id_t adm_for, IN(flat_set<admin_id_t>) against, double totalval, bool enforcing, INOUT(std::vector<prov_id_t>) ppool, IN(w_lock) l);
double list_goal_provinces_a(INOUT(cvector<prov_id_t>) lst, IN(wargoal) goal, admin_id_t adm_for, IN(cflat_set<admin_id_t>) against, double totalval, bool enforcing, INOUT(cvector<prov_id_t>) ppool, IN(g_lock) l);
bool can_enforce_goal(admin_id_t goal_for, IN(wargoal) goal, IN(newwar) wr, IN(newwar) otherside, IN(std::vector<prov_id_t>) noma, IN(std::vector<prov_id_t>) nomb, IN(w_lock) l);
bool can_enforce_goal(admin_id_t goal_for, IN(wargoal) goal, IN(newwar) wr, IN(newwar) otherside, IN(cvector<prov_id_t>) noma, IN(cvector<prov_id_t>) nomb, IN(g_lock) l);

bool in_war_against(char_id_t a, char_id_t b, IN(g_lock) l);
double count_troops_raised(char_id_t id, IN(g_lock) l);
void raise_all_troops(IN(flat_set<admin_id_t>) p, IN(w_lock) l);
void add_to_war(INOUT(newwar) w, INOUT(flat_map<char_id_t, bool>) asked, admin_id_t a, char_id_t on_behalf_of, char_id_t war_against, wargoal primarygoal, IN(w_lock) l);
void add_goal_to_war(INOUT(newwar) w, admin_id_t a, wargoal g, IN(w_lock) l);

double aggression_cost_for_no_cb(char_id_t id, IN(g_lock) l);

void end_war(war_id_t wid, IN(w_lock) l);
void adjust_alloc_to(size_t front_index, INOUT(war_pair) w, bool attacker, float target, IN(w_lock) l);
double points_in_war(admin_id_t adm, IN(g_lock) l) noexcept;
void collapse_participants(IN(newwar) w, INOUT(flat_map<admin_id_t, std::vector<wargoal>>) cmap);
void update_seiges(IN(w_lock) l);
void stand_down_troop(admin_id_t adm, IN(w_lock) l);
void reduce_troops(IN(flat_set<admin_id_t>) participants, double percent_loss, IN(w_lock) l) noexcept;

double enact_wargoal_on_a(bool enforced, IN(wargoal) goal, double totalval, admin_id_t adm_for, IN(flat_set<admin_id_t>) against, INOUT(std::vector<prov_id_t>) ppool, INOUT(flat_set<char_id_t>) territory_losers, IN(w_lock) l);

double war_base_tax_value(war_id_t wid, IN(cvector<prov_id_t>) selfcontrolled, IN(cvector<prov_id_t>) tcontrolled, IN(g_lock) l);
double war_base_tax_value(war_id_t wid, IN(std::vector<prov_id_t>) selfcontrolled, IN(std::vector<prov_id_t>) tcontrolled, IN(w_lock) l);
double war_prediction_value(war_id_t wid, bool attacker, IN(cvector<prov_id_t>) selfcontrolled, IN(cvector<prov_id_t>) tcontrolled, IN(g_lock) l);
double war_prediction_value(war_id_t wid, bool attacker, IN(std::vector<prov_id_t>) selfcontrolled, IN(std::vector<prov_id_t>) tcontrolled, IN(w_lock) l);
double war_estimation(char_id_t from, char_id_t to, IN(g_lock) l);

double slow_war_base_value(war_id_t wid, IN(g_lock) l);
double slow_war_prediction(war_id_t wid, IN(g_lock) l);
bool prov_belongs_to_attacker(war_id_t wid, prov_id_t prov, IN(g_lock) l);

bool wargoal_allows_partial(IN(wargoal) goal);
bool wargoal_has_prov_list(IN(wargoal) goal);
double display_goal(IN(std::shared_ptr<uiElement>) parent, int x, int &y, IN(wargoal) goal, char_id_t actor, double totalval, IN(g_lock) l, bool enforcing); //returns value used by goal
void display_goal_name(IN(std::shared_ptr<uiElement>) parent, int x, int &y, IN(wargoal) goal, IN(g_lock) l);

inline double troopCost(size_t l, size_t s) noexcept;
float troop_cost_est(float points) noexcept;
void balance_war(INOUT(war_pair) wp, bool attacker, IN(w_lock) l);

void updatewar(INOUT(war_pair) wc, INOUT(w_lock) wlk);
void pay_for_war(INOUT(newwar) wr, unsigned int currentday, IN(w_lock) l) noexcept;

inline double pointstoprob(const double in) noexcept;
bool is_at_war(char_id_t id, IN(g_lock) l) noexcept;

void pack_war_participants(war_id_t war_for, bool aggressor, INOUT(std::vector<std::pair<admin_id_t, std::vector<wargoal>>>) vec, IN(g_lock) l) noexcept;
bool is_agressor(admin_id_t adm, war_id_t war_in, IN(g_lock) l) noexcept;

void provsbygoal(INOUT(cvector<prov_id_t>) provs, char_id_t c, char_id_t target, IN(wargoal) g,  IN(g_lock) l) noexcept;
void provsbygoal(INOUT(std::vector<prov_id_t>) provs, char_id_t c, char_id_t target, IN(wargoal) g, IN(w_lock) l) noexcept;
bool can_attack(char_id_t source, char_id_t target, IN(g_lock) l) noexcept;
