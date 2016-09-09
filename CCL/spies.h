#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"


void open_spy_mission_select() noexcept;
void open_intrigue_window() noexcept;

#define PLOT_TYRANNY			1
#define PLOT_AGGRESSION			2
#define PLOT_DISHONORERABLE		3

class spy_mission;

class plot_event {
public:
	const size_t text;
	const size_t success;
	const size_t failure;
	const double cost;
	const double base_chance;
	const double plot_bonus;

	plot_event(size_t tx, size_t s, size_t f, double cst, double chance, double bonus) : text(tx), success(s), failure(f), cost(cst), base_chance(chance), plot_bonus(bonus) {};
};

class plot_advancement {
public:
	const size_t goal;
	const double adv_chance_bonus;
	const double adv_chance_multiplier;

	const float plot_failure_chance;
	const bool violent;

	plot_advancement(bool v, size_t g, double cb, double cm, float plot_failure) : violent(v), goal(g), adv_chance_bonus(cb), adv_chance_multiplier(cm), plot_failure_chance(plot_failure) {}
};

class plot_type {
public:
	const std::function<void(admin_id_t, spy_mission&, w_lock&)> finish;
	const std::vector<unsigned char> stages; // 0 = event; else id of plot advancement

	const size_t name;
	const double monthly_cost;

	plot_type(size_t n, double cost, IN(std::function<void(admin_id_t, spy_mission&, w_lock&)>) f, IN(std::vector<unsigned char>) vec) : name(n), monthly_cost(cost), finish(f), stages(vec) {};
};



#define MAX_EVENTS_SEEN 8
#define MAX_PEOPLE_TRIED 10
#define MAX_PEOPLE_INVOLVED 5

class spy_mission {
public:
	double plot_bonus = 0.0;

	
	char_id_t people_tried[MAX_PEOPLE_TRIED] = {char_id_t()};
	char_id_t people_involved[MAX_PEOPLE_INVOLVED] = {char_id_t()};
	unsigned char events_seen[MAX_EVENTS_SEEN] = {0};

	char_id_t spy;
	char_id_t target;

	unsigned char type = 0;
	unsigned char stage = 0;

	spy_mission(char_id_t t, char_id_t s, unsigned char y) noexcept : spy(s), target(t), type(y) {};

	void construct(char_id_t t, char_id_t s, unsigned char y) noexcept {
		spy = s;
		target = t;
		type = y;
		stage = 0;
		memset(people_tried, sizeof(people_tried), 0);
		memset(people_involved, sizeof(people_involved), 0);
		memset(events_seen, sizeof(events_seen), 0);
	}
	void set_clear() noexcept {
		type = 0;
	}
	bool is_clear() const noexcept {
		return type == 0;
	}
};

struct smission_selection {
	char_id_t spy;
	char_id_t target;
	unsigned char type = 0;
};

void execute_plot_event(admin_id_t from, INOUT(spy_mission) sm, IN(plot_event) p, INOUT(w_lock) l) noexcept;
bool recruit_plot_member(admin_id_t from, INOUT(spy_mission) sm, IN(plot_advancement) p, IN(plot_type) plottype, INOUT(w_lock) l, INOUT(int) intrigue) noexcept; //returns success
bool execute_plot_stage(admin_id_t from, INOUT(spy_mission) sm, IN(plot_type) plottype, INOUT(w_lock) l) noexcept; //returns completion

const plot_type& plot_by_type(unsigned char type) noexcept;
const plot_event& plot_event_by_type(unsigned char type) noexcept;
const plot_advancement& plot_adv_by_type(unsigned char type) noexcept;

void free_spy_mission(SM_TAG_TYPE id, IN(w_lock) l) noexcept;
spy_mission& get_spy_mission(SM_TAG_TYPE tg, IN(g_lock) l) noexcept;
size_t count_spy_missions(admin_id_t a, IN(g_lock) l) noexcept;

class sm_iter_f {
public:
	static spy_mission& apply(IN(std::pair<admin_id_t, SM_TAG_TYPE>) i) noexcept {
		return get_spy_mission(i.second, fake_lock());
	}
};

size_t max_spy_missions_a(admin_id_t a, IN(g_lock) l) noexcept;
bool new_spy_mission_possible(admin_id_t source, IN(g_lock) l) noexcept;
bool new_spy_mission_possible(admin_id_t source, char_id_t target, unsigned char type, IN(g_lock) l) noexcept;
bool plot_exists(admin_id_t source, char_id_t target, unsigned char type, IN(g_lock) l) noexcept;
bool can_be_spy(char_id_t id, IN(g_lock) l) noexcept;

//execute the following only from action or schedule
void update_spy_mission(admin_id_t owner, SM_TAG_TYPE mission_number, INOUT(w_lock) l) noexcept;
void add_new_spy_mission(admin_id_t source, char_id_t target, char_id_t spy, unsigned char type, IN(w_lock) l) noexcept;
void terminate_spy_mission(admin_id_t owner, SM_TAG_TYPE mission_number, INOUT(w_lock) l) noexcept;

extern update_record smission_display_rec;
extern update_record smission_selection_rec;

