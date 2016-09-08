#pragma once
#include "globalhelpers.h"

class administration;
class person;
struct untitled_data;

void do_indv_step(char_id_t id, IN(person) p, IN(untitled_data) d) noexcept;
void do_admin_step(IN(administration) adm) noexcept;