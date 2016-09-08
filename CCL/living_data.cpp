#include "globalhelpers.h"
#include "living_data.h"
#include "structs.hpp"

namespace global {
	flat_set<char_id_t> living;
	// v_pool<titled_data, titled_data_check> titled_pool;
	v_pool<untitled_data> untitled_pool;
}

void remove_living(char_id_t c, IN(w_lock) l) noexcept {
	// const auto th = global::people[c.value].tdata;
	// if (th != global::titled_pool.tag_max)
	//	global::titled_pool.free(th, l);
	const auto uh = get_object(c).utdata;
	if(uh != global::untitled_pool.tag_max)
		global::untitled_pool.free(uh, l);
	global::living.erase(c);
}

/*void create_titled(char_id_t c, untitled_data * __restrict & u, titled_data * __restrict & t, IN(w_lock) l) noexcept {
	const auto th = global::people[c.value].tdata = global::titled_pool.emplace(l, c);
	const auto uh = global::people[c.value].utdata = global::untitled_pool.emplace(l);
	u = &global::untitled_pool.get(uh, l);
	t = &global::titled_pool.get(th, l);
	global::living.emplace(c);
} */

void create_untitled(char_id_t c, untitled_data* __restrict &u, IN(w_lock) l) noexcept {
	// global::people[c.value].tdata = global::untitled_pool.tag_max;
	const auto uh = get_object(c).utdata = global::untitled_pool.emplace(l);
	u = &global::untitled_pool.get(uh, l);
	global::living.emplace(c);
}

__declspec(restrict)untitled_data* get_udata(char_id_t c, IN(g_lock) l) noexcept {
	const auto h = get_object(c).utdata;
	if (h != global::untitled_pool.tag_max) {
		return &global::untitled_pool.get(h, l);
	}
	return nullptr;
}

/*__declspec(restrict)titled_data* get_tdata(char_id_t c, IN(g_lock) l) noexcept {
	const auto h = global::people[c.value].tdata;
	if (h != global::titled_pool.tag_max) {
		return &global::titled_pool.get(h, l);
	}

	return nullptr;
}*/

void get_living_data(char_id_t c, untitled_data * __restrict & u, IN(g_lock) l) noexcept {
	//const auto th = global::people[c.value].tdata;
	//if (th != global::titled_pool.tag_max)
	//	t = &global::titled_pool.get(th, l);
	//else
	//	t = nullptr;

	const auto uh = get_object(c).utdata;
	if (uh != global::untitled_pool.tag_max)
		u = &global::untitled_pool.get(uh, l);
	else
		u = nullptr;
	return;
	
}


/*void remove_titled_data(char_id_t c, IN(w_lock) l) noexcept {
	const auto th = global::people[c.value].tdata;
	if (th != global::titled_pool.tag_max) {
		global::titled_pool.free(th, l);
		global::people[c.value].tdata = global::titled_pool.tag_max;
	}
}

__declspec(restrict) titled_data * add_titled_data(char_id_t c, IN(w_lock) l) noexcept {
	if (global::people[c.value].tdata == global::titled_pool.tag_max) {
		global::people[c.value].tdata = global::titled_pool.emplace(l, c);
	}
	return &global::titled_pool.get(global::people[c.value].tdata, l);
}*/

void clear_living_data(IN(w_lock) l) noexcept {
	global::living.clear();
	//global::titled_pool.clear(l);
	global::untitled_pool.clear(l);
}
