#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"

struct expense_record {
	unsigned short amount;
	unsigned char months;
	unsigned char type;
};

#define EXPENSE_EVENT 1
#define EXPENSE_WAR 2
#define EXPENSE_TRIBUTE 3
#define EXPENSE_ESPIONAGE 4

#define EXPENSE_MAX 5

#define INCOME_MASK 0x80

#define INCOME_TRIBUTE  (0 | INCOME_MASK)

#define INCOME_MAX (1 | INCOME_MASK)

std::wstring expense_type_to_name(unsigned char type) noexcept;

extern flat_multimap<char_id_t, expense_record> expense_records;

void add_expense(char_id_t person, unsigned char type, unsigned char duration, double amount, IN(w_lock) l) noexcept;
void add_expense(char_id_t person, unsigned char type, unsigned char duration, float amount, IN(w_lock) l) noexcept;

void remove_expense(char_id_t person, unsigned char type, double amount, IN(w_lock) l) noexcept;
void remove_expense(char_id_t person, unsigned char type, float amount, IN(w_lock) l) noexcept;

void pay_all_expenses(IN(g_lock) l) noexcept;
void remove_expenses_for(char_id_t person, IN(w_lock) l) noexcept;

void remove_expense_by_type(char_id_t person, unsigned char type, IN(w_lock) l) noexcept;
void expenses_to_ui(IN(std::shared_ptr<uiElement>) p, int x, int &y, char_id_t person, IN(g_lock) l) noexcept;
void ex_income_to_ui(IN(std::shared_ptr<uiElement>) p, int x, int &y, char_id_t person, IN(g_lock) l) noexcept;
double current_mthly_expense(char_id_t person, IN(g_lock) l) noexcept;

template<typename T>
void enum_expenses_for(char_id_t person, IN(g_lock) l, IN(T) f) noexcept {
	const auto range = expense_records.equal_range(person);
	for (auto i = range.first; i != range.second; ++i) {
		if (i->second.type != 0)
			f(i->second);
	}
}

void expense_f_generate(IN_P(sqlite3) db) noexcept;
void expense_f_load(IN_P(sqlite3) db, IN(w_lock) l) noexcept;
void expense_f_save(IN_P(sqlite3) db, IN(r_lock) l) noexcept;