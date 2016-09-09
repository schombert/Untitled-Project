#pragma once
#include "globalhelpers.h"

void remove_from_court(char_id_t person, IN(w_lock) l) noexcept;
void add_to_court_a(char_id_t person, admin_id_t court, IN(w_lock) l) noexcept;
void enum_court_a(admin_id_t t, INOUT(std::vector<char_id_t>) c, IN(w_lock) l) noexcept;
void enum_court_a(admin_id_t t, INOUT(cvector<char_id_t>) c, IN(g_lock) l) noexcept;
void enum_court_family(admin_id_t t, char_id_t ch, INOUT(std::vector<char_id_t>) c, IN(w_lock) l) noexcept;
void enum_court_family(admin_id_t t, char_id_t ch, INOUT(cvector<char_id_t>) c, IN(g_lock) l) noexcept;
void clear_court(IN(w_lock) l) noexcept;
void insert_to_court(char_id_t person, admin_id_t court, IN(w_lock) l) noexcept;
admin_id_t get_court(char_id_t person, IN(g_lock) l) noexcept;