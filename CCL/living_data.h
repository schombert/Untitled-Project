#pragma once
#include "globalhelpers.h"
#include "wardata.h"
#include "ai_interest.h"
#include "envoys.h"
#include "spies.h"
#include "laws.h"

#define P_FLAG_PERPARING_EVENT	0x0002
#define P_FLAG_VALID			0x0001
#define P_FLAG_ON_MISSION		0x0004

struct untitled_data {
	peronal_attributes attrib;
	statstruct stats;

	double wealth = 0.0;
	double p_honorable = 0.5;
	double p_peaceful = 0.5;
	double p_just = 0.5;
	double income_estimate = min_value<double>::value;

	float mu;			
	float sigma_sq;	

	unsigned int activity = 0;

	admin_id_t a_court;
	unsigned short flags = P_FLAG_VALID;
	
	cul_id_t culture;
	rel_id_t religion;

	void construct() noexcept {
		culture = cul_id_t();
		religion = rel_id_t();
		a_court = admin_id_t();
		activity = 0;
		wealth = 0.0;
		p_honorable = 0.5;
		p_peaceful = 0.5;
		p_just = 0.5;
		mu = 0.0f;
		sigma_sq = 0.0f;
		income_estimate = min_value<double>::value;
	}
	bool is_clear() const noexcept {
		return flags == 0;
	}
	void set_clear() noexcept {
		flags = 0;
	}
};

using udata = untitled_data;

class untitled_data_check {
public:
	
};

/*
class titled_data {
public:
	// interest_manager interests;
	// warcollection wars;

	// double strength_estimate = min_value<double>::value;
	// double defensive_estimate = min_value<double>::value;
	char_id_t associated;
	// admin_id_t executive_of;

	
	//SM_TAG_TYPE spymissions[8] = {max_value<SM_TAG_TYPE>::value,max_value<SM_TAG_TYPE>::value,
	//								max_value<SM_TAG_TYPE>::value,max_value<SM_TAG_TYPE>::value,
	//								max_value<SM_TAG_TYPE>::value,max_value<SM_TAG_TYPE>::value,
	//								max_value<SM_TAG_TYPE>::value,max_value<SM_TAG_TYPE>::value};
	
	
	titled_data(char_id_t id) noexcept : associated(id) {}
};

class titled_data_check {
public:
	static bool is_clear(IN(titled_data) i) noexcept {
		return i.associated == 0;
	}
	static void set_clear(INOUT(titled_data) i) noexcept {
		i.~titled_data();
		i.associated = 0;
	}
};*/


namespace global {
	extern flat_set<char_id_t> living;
	// extern v_pool<titled_data, titled_data_check> titled_pool;
	extern v_pool<untitled_data> untitled_pool;
}

void remove_living(char_id_t c, IN(w_lock) l) noexcept;
// void create_titled(char_id_t c, untitled_data* __restrict &u, titled_data* __restrict &t, IN(w_lock) l) noexcept;
void create_untitled(char_id_t c, untitled_data* __restrict &u, IN(w_lock) l) noexcept;
// __declspec(restrict)untitled_data* get_udata(char_id_t c, IN(g_lock) l) noexcept;
// __declspec(restrict)titled_data* get_tdata(char_id_t c, IN(g_lock) l) noexcept;
void get_living_data(char_id_t c, untitled_data* __restrict &u, IN(g_lock) l) noexcept;
// void remove_titled_data(char_id_t c, IN(w_lock) l) noexcept;
// __declspec(restrict)titled_data*  add_titled_data(char_id_t c, IN(w_lock) l) noexcept;
void clear_living_data(IN(w_lock) l) noexcept;

template<typename T, typename LK, typename F>
T with_udata(char_id_t p, IN(LK) l, T def_return, IN(F) func) noexcept {
	const auto h = get_object(p).utdata;
	if (h != global::untitled_pool.tag_max) {
		return func(global::untitled_pool.get(h, l));
	}
	return def_return;
}

template<typename LK, typename F>
auto with_udata_force(char_id_t p, IN(LK) l, IN(F) func) noexcept {
	return func(global::untitled_pool.get(get_object(p).utdata, l));
}

template<typename LK, typename F>
void with_udata(char_id_t p, IN(LK) l, IN(F) func) noexcept {
	const auto h = get_object(p).utdata;
	if (h != global::untitled_pool.tag_max) {
		func(global::untitled_pool.get(h, l));
	}
}

template<typename LK, typename F>
void with_udata_2(char_id_t a, char_id_t b, IN(LK) l, IN(F) func) noexcept {
	const auto h = get_object(a).utdata;
	const auto i = get_object(b).utdata;
	if ((h != global::untitled_pool.tag_max) & (i != global::untitled_pool.tag_max)) {
		func(global::untitled_pool.get(h, l), global::untitled_pool.get(i, l));
	}
}

template<typename T, typename LK, typename F>
T with_udata_2(char_id_t a, char_id_t b, IN(LK) l, T def_return, IN(F) func) noexcept {
	const auto h = get_object(a).utdata;
	const auto i = get_object(b).utdata;
	if ((h != global::untitled_pool.tag_max) & (i != global::untitled_pool.tag_max)) {
		return func(global::untitled_pool.get(h, l), global::untitled_pool.get(i, l));
	}
	return def_return;
}

