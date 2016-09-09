#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"

#define MIS_DEFENSIVE_AGAINST 1
#define MIS_DEFENSIVE_PACT 2
#define MIS_NON_AGGRESSION_TO 3

void open_mission_selection_a(admin_id_t target) noexcept;
void open_envoys_window_a(admin_id_t target) noexcept;

bool mission_possible_a(admin_id_t target, unsigned char id) noexcept;
std::wstring get_mission_name(unsigned char id) noexcept;
unsigned int max_envoy_missions_a(admin_id_t source, IN(g_lock) l) noexcept;
bool can_add_mission_a(admin_id_t source, IN(g_lock) l) noexcept;
bool can_add_defensive_mission_a(admin_id_t source, IN(g_lock) l) noexcept;
bool can_add_defensive_against_mission_a(admin_id_t source, char_id_t against, IN(g_lock) l) noexcept;
bool can_add_non_aggression_mission_a(admin_id_t source, char_id_t with, IN(g_lock) l) noexcept;

void add_defensive_pact_mission_a(admin_id_t id, char_id_t person, IN(g_lock) l) noexcept;
void add_defensive_against_mission_a(admin_id_t id, char_id_t person, char_id_t against, IN(g_lock) l) noexcept;
void add_nonagression_mission_a(admin_id_t id, char_id_t person, char_id_t target, IN(g_lock) l) noexcept;
bool can_be_envoy(char_id_t id, IN(g_lock) l) noexcept;

struct emission_selection {
	//std::shared_ptr<uiDragRect> window;
	//std::shared_ptr<uiDropDown> selection;
	//std::shared_ptr<uiScrollView> contents;

	unsigned char current_selection = 0;
	char_id_t target;
	char_id_t envoy;
};

class envoy_mission {
public:
	static constexpr unsigned char DEFENSIVE_AGAINST = 1;
	static constexpr unsigned char DEFENSIVE = 2;
	static constexpr unsigned char NON_AGGRESSION = 3;

	unsigned int start_date;
	unsigned int return_date;
	unsigned int expiration_date;
	char_id_t envoy;

	
	bool active = true;
	unsigned char type;
	unsigned char index;

	envoy_mission(unsigned char t) noexcept : type(t) {};

	virtual void display_mission_a(int x, INOUT(int) y, IN(std::shared_ptr<uiElement>) parent, admin_id_t source, IN(g_lock) l) noexcept;
	virtual bool accept_offer(admin_id_t source, char_id_t actor, size_t indx, INOUT(w_lock) l) noexcept { abort(); };
	virtual void decline_offer(admin_id_t source, char_id_t actor, size_t indx, IN(g_lock) l) noexcept { abort(); };
	virtual bool offer_available(size_t indx) noexcept { return false; };
	virtual size_t total_offers() noexcept { abort(); };
	virtual void expire_mission(admin_id_t source, IN(g_lock) l) noexcept;

	virtual void make_offer(admin_id_t source, char_id_t person, char_id_t target, INOUT(w_lock) l) noexcept { abort(); };
	virtual void get_target_list(admin_id_t source, IN(g_lock) l) noexcept { abort(); };
	virtual bool identical(IN(envoy_mission) m) const noexcept { return type == m.type; };
	virtual unsigned int days_until_offers(char_id_t person, IN(g_lock) l) noexcept { abort(); };
	virtual void start_mission(char_id_t actor, admin_id_t source, unsigned int on_day, IN(w_lock) l) noexcept { abort(); };
	virtual void setup_next_offer(char_id_t actor, admin_id_t source, unsigned int on_day, IN(w_lock) l) noexcept { abort(); };
	virtual ~envoy_mission() noexcept {};
};

unsigned char first_free_e_index(admin_id_t a, IN(g_lock) l) noexcept;
size_t count_missions(admin_id_t a, IN(g_lock) l) noexcept;
envoy_mission* get_envoy_mission(admin_id_t a, unsigned char index, IN(g_lock) l) noexcept;
void remove_envoy_mision(admin_id_t a, unsigned char index, IN(w_lock) l) noexcept;

extern update_record emission_display_rec;
extern update_record emission_selection_rec;

void display_individual_mission(IN(std::shared_ptr<uiElement>) p, INOUT(int) x, INOUT(int) y, IN(std::pair<admin_id_t, concurrent_uniq<envoy_mission>>) obj, admin_id_t source, IN(g_lock) l) noexcept;