#include "globalhelpers.h"
#include "finances.h"
#include "i18n.h"
#include "structs.hpp"
#include "living_data.h"
#include "uielements.hpp"
#include "datamanagement.hpp"

flat_multimap<char_id_t, expense_record> expense_records;

std::wstring expense_type_to_name(unsigned char type) noexcept {
	size_t param = type;
	return get_p_string(TX_EXPENSE_NAME, &param, 1);
}

#define EXPENSE_VALUE(x) (static_cast<double>(x) / 8.0)
#define TO_EXPENSE_VALUE(x) static_cast<unsigned short>((x) * 8.0)

template<typename T>
void _add_expense(char_id_t person, unsigned char type, unsigned char duration, T amount, IN(w_lock) l) noexcept {
	auto range = expense_records.equal_range(person);
	for (; range.first != range.second; ++range.first) {
		if (range.first->second.type == 0)
			break;
	}
	
	while (amount > 0) {
		expense_record r;
		r.type = type;
		r.months = duration;
		if (amount < static_cast<T>(8191.0))
			r.amount = TO_EXPENSE_VALUE(amount);
		else
			r.amount = TO_EXPENSE_VALUE(8191.0);
		if (range.first != range.second) {
			range.first->second = r;
			for (; range.first != range.second; ++range.first) {
				if (range.first->second.type == 0)
					break;
			}
		} else {
			expense_records.emplace(person, r);
		}
		amount -= static_cast<T>(8191.0);
	}
}

void add_expense(char_id_t person, unsigned char type, unsigned char duration, double amount, IN(w_lock) l) noexcept {
	_add_expense(person, type, duration, amount, l);
}

void add_expense(char_id_t person, unsigned char type, unsigned char duration, float amount, IN(w_lock) l) noexcept {
	_add_expense(person, type, duration, amount, l);
}

template<typename T>
void _remove_expense(char_id_t person, unsigned char type, T amount, IN(w_lock) l) noexcept {
	auto range = expense_records.equal_range(person);
	for (; range.first != range.second; ++range.first) {
		INOUT(auto) r = range.first->second;
		if (r.type == type) {
			if (amount >= static_cast<T>(8191.0) && r.amount == TO_EXPENSE_VALUE(8191.0)) {
				r.type = 0;
				amount -= static_cast<T>(8191.0);
			} else if (r.amount == TO_EXPENSE_VALUE(amount)) {
				r.type = 0;
				return;
			}
		}
	}
}

void remove_expense(char_id_t person, unsigned char type, double amount, IN(w_lock) l) noexcept {
	_remove_expense(person, type, amount, l);
}

void remove_expense(char_id_t person, unsigned char type, float amount, IN(w_lock) l) noexcept {
	_remove_expense(person, type, amount, l);
}

void remove_expense_by_type(char_id_t person, unsigned char type, IN(w_lock) l) noexcept {
	auto range = expense_records.equal_range(person);
	for (; range.first != range.second; ++range.first) {
		INOUT(auto) r = range.first->second;
		if (r.type == type) {
			r.type = 0;
		}
	}
}

void pay_all_expenses(IN(g_lock) l) noexcept {
	for (INOUT(auto) r : expense_records) {
		if (valid_ids(r.first) & (r.second.type != 0)) {
			with_udata(r.first, l, [&r](INOUT(udata) d) noexcept {
				if ((r.second.type & INCOME_MASK) != 0)
					d.wealth += EXPENSE_VALUE(r.second.amount);
				else
					d.wealth -= EXPENSE_VALUE(r.second.amount);
			});
			if (r.second.months != 0) {
				--r.second.months;
				if (r.second.months == 0)
					r.second.type = 0;
			}
		}
	}
}


void remove_expenses_for(char_id_t person, IN(w_lock) l) noexcept {
	const auto range = expense_records.equal_range(person);
	expense_records.erase(range.first, range.second);
}


void expenses_to_ui(IN(std::shared_ptr<uiElement>) p, int x, int &y, char_id_t person, IN(g_lock) l) noexcept {
	p->add_element<uiSimpleText>(x, y, get_simple_string(TX_EXPENSE_L), global::empty, global::standard_text);
	y += global::standard_text.csize + 2;

	const auto range = expense_records.equal_range(person);
	for (unsigned char type = 1; type != EXPENSE_MAX; ++type) {
		double total = 0.0;
		for (auto i = range.first; i != range.second; ++i) {
			if (i->second.type == type) {
				total += EXPENSE_VALUE(i->second.amount);
			}
		}
		if (total != 0.0) {
			size_t params[] = {dumb_cast<size_t>(total), static_cast<size_t>(type)};
			p->add_element<uiSimpleText>(x, y, get_p_string(TX_FORMAT_EXPENSE, params, 2), global::empty, global::standard_text);
			y += global::standard_text.csize + 2;
		}
	}
}

void ex_income_to_ui(IN(std::shared_ptr<uiElement>) p, int x, int &y, char_id_t person, IN(g_lock) l) noexcept {
	p->add_element<uiSimpleText>(x, y, get_simple_string(TX_INCOME_L), global::empty, global::standard_text);
	y += global::standard_text.csize + 2;

	const auto range = expense_records.equal_range(person);
	for (unsigned char type = INCOME_TRIBUTE; type != INCOME_MAX; ++type) {
		double total = 0.0;
		for (auto i = range.first; i != range.second; ++i) {
			if (i->second.type == type) {
				total += EXPENSE_VALUE(i->second.amount);
			}
		}
		if (total != 0.0) {
			size_t params[] = {dumb_cast<size_t>(total), static_cast<size_t>(type & ~INCOME_MASK)};
			p->add_element<uiSimpleText>(x, y, get_p_string(TX_FORMAT_EX_INCOME, params, 2), global::empty, global::standard_text);
			y += global::standard_text.csize + 2;
		}
	}
}

double current_mthly_expense(char_id_t person, IN(g_lock) l) noexcept {
	double total = 0;
	const auto range = expense_records.equal_range(person);
	for (auto i = range.first; i != range.second; ++i) {
		if (i->second.type != 0) {
			if((i->second.type & INCOME_MASK) != 0)
				total -= EXPENSE_VALUE(i->second.amount);
			else
				total += EXPENSE_VALUE(i->second.amount);
		}
	}
	return total;
}

void expense_f_generate(IN_P(sqlite3) db) noexcept {
	sqlite3_exec(db, "CREATE TABLE finances (person INTEGER, amount INTEGER, months INTEGER, type INTEGER)", nullptr, nullptr, nullptr);
}

void expense_f_load(IN_P(sqlite3) db, IN(w_lock) l) noexcept {
	stmtwrapper stmt(db, "SELECT person, amount, months, type FROM finances");
	expense_records.clear();

	while (stmt.step()) {
		expense_records.emplace(static_cast<char_id>(sqlite3_column_int64(stmt, 0)),
			expense_record{
			static_cast<unsigned short>(sqlite3_column_int64(stmt, 1)),
			static_cast<unsigned char>(sqlite3_column_int64(stmt, 2)),
			static_cast<unsigned char>(sqlite3_column_int64(stmt, 3))
		});
	}
}

void expense_f_save(IN_P(sqlite3) db, IN(r_lock) l) noexcept {
	sqlite3_exec(db, "DELETE FROM finances", nullptr, nullptr, nullptr);

	stmtwrapper add_exp(db, "INSERT INTO finances (person, amount, months, type) VALUES (?1, ?2, ?3, ?4)");

	for (IN(auto) pr : expense_records) {
		bindings bind(add_exp);
		bind(1, pr.first);
		bind(2, pr.second.amount);
		bind(3, pr.second.months);
		bind(4, pr.second.type);
		add_exp.step();
	}
}