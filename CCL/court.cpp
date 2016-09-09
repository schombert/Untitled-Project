#include "globalhelpers.h"
#include "court.h"
#include "living_data.h"
#include "laws.h"
#include "datamanagement.hpp"

namespace global {
	multiindex<admin_id_t, char_id_t> admintocourt;
}

admin_id_t get_court(char_id_t person, IN(g_lock) l) noexcept {
	return with_udata(person, l, admin_id_t(), [](IN(udata) d) { return d.a_court; });
}

void remove_from_court(char_id_t person, IN(w_lock) l) noexcept {
	with_udata(person, l, [person, &l](INOUT(udata) d) noexcept {
		global::admintocourt.erase(d.a_court, person, l);
		d.a_court = admin_id_t::NONE;
	});
}

void add_to_court_a(char_id_t person, admin_id_t court, IN(w_lock) l) noexcept {
	with_udata(person, l, [&l, person, court](INOUT(udata) d) noexcept {
		if (valid_ids(d.a_court)) {
			global::admintocourt.erase(d.a_court, person, l);
		}
		d.a_court = court;
		global::admintocourt.insert(court, person, l);
	});
}

void enum_court_a(admin_id_t t, INOUT(std::vector<char_id_t>) c, IN(w_lock) l) noexcept {
	global::admintocourt.to_vector(t, c, l);
}

void enum_court_a(admin_id_t t, INOUT(cvector<char_id_t>) c, IN(g_lock) l) noexcept {
	global::admintocourt.to_vector(t, c, l);
}


template<typename T, typename L>
void _enum_court_family(admin_id_t t, char_id_t ch, INOUT(std::vector<char_id_t, T>) c, IN(L) l) noexcept {
	global::admintocourt.for_each(t, l, [ch, &c](char_id_t i) {
		if (global::are_close_family(i, ch))
			c.push_back(i);
	});
}

void enum_court_family(admin_id_t t, char_id_t ch, INOUT(std::vector<char_id_t>) c, IN(w_lock) l) noexcept {
	_enum_court_family(t, ch, c, l);
}

void enum_court_family(admin_id_t t, char_id_t ch, INOUT(cvector<char_id_t>) c, IN(g_lock) l) noexcept {
	_enum_court_family(t, ch, c, l);
}

void clear_court(IN(w_lock) l) noexcept {
	global::admintocourt.clear(l);
}

void insert_to_court(char_id_t person, admin_id_t court, IN(w_lock) l) noexcept {
	global::admintocourt.insert(court, person, l);
}