#pragma once
#include "globalhelpers.h"
#include "ai_actions.h"
#include "ai_execution.h"
#include "pacts.h"
#include "structs.hpp"

#define I_TITLES				0
#define I_PEOPLE				1
#define I_PROVINCES				2
#define I_ARRAYS				3

#define I_PROVINCES_TITLE		0
#define I_PROVINCES_TAX			1

#define I_TITLES_CAPITAL		0
#define I_TITLES_CULTURE		1
#define I_TITLES_RELIGION		2
#define I_TITLES_HOLDER			3
#define I_TITLES_LEIGE			4
#define I_TITLES_VTAX			5

#define I_PEOPLE_CULTURE		0
#define I_PEOPLE_RELIGION		1
#define I_PEOPLE_DYNASTY		2
#define I_PEOPLE_FATHER			3
#define I_PEOPLE_MOTHER			4
#define I_PEOPLE_GENDER			5
#define I_PEOPLE_SPOUSE			6
#define I_PEOPLE_PRIMET			7
#define I_PEOPLE_MONEY			8
#define I_PEOPLE_STRENGTH		9
#define I_PEOPLE_INCOME			10
#define I_PEOPLE_DEFENSIVE		11
#define I_PEOPLE_WARPROB		12
#define I_PEOPLE_DEFENCE_AGAINST 13

#define I_ARRAYS_THREATS		0
#define I_ARRAYS_THREAT_PROB	1

class base_value_mask {
public:
	static std_fp get_value(size_t category, size_t value, size_t index, char_id person) noexcept;
};

class temporary_mask;


#define I_CATEGORY(x) (x & 0x03)
#define I_VALUE(x) ((x >> 2) & 0x0F)
#define I_INDEX(x) (x >> 6)
#define TO_I(i,v,c) (((i << 4) | v) << 2) | c

class value_mask {
public:
	flat_map<size_t, std_fp, std::less<size_t>, concurrent_allocator<std::pair<size_t, std_fp>>> v_map;

	value_mask() {};
	value_mask(IN(value_mask) p);
	value_mask(value_mask &&p);

	const value_mask& operator=(IN(value_mask) in) noexcept;
	void add_value(size_t category, size_t value, size_t index, std_fp val, char_id id) noexcept;
	void set_value(size_t category, size_t value, size_t index, std_fp val) noexcept;
	std_fp get_value(size_t category, size_t value, size_t index, char_id id) noexcept;
};

constexpr const std_fp indefinite_duration(int current_age) noexcept {
	return std_fp::from_int(std::max(10*365, (65 * 365) - current_age));
}

std_fp fitness_function(char_id id, INOUT(value_mask) data, IN(ai_action_prefs) prefs) noexcept;

class action_function;

std_fp percent_difference(INOUT(value_mask) newstate, IN(value_mask) target, INOUT(value_mask) base, char_id id);

class action_function {
public:
	virtual std_fp probability(INOUT(value_mask) i, char_id id) const noexcept {
		return std_fp::from_int(1);
	}
	virtual std_fp time(INOUT(value_mask) i, char_id id) const noexcept {
		return std_fp::from_int(1);
	}
	virtual std_fp duration(INOUT(value_mask) i, char_id id) const noexcept;
	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
		o = i;
	}
	virtual void prereq(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
		o = i;
	}

	std_fp value(INOUT(value_mask) state, char_id id, std_fp current_fitness, IN(ai_action_prefs) prefs) const noexcept;
	std_fp instrumental_value(INOUT(value_mask) state, char_id id, IN(value_mask) target_state, std_fp target_value) const noexcept;
	virtual ~action_function() {}
};


class change_money_f : public action_function {
public:
	const std_fp amount;
	change_money_f(double a) : amount(std_fp::from_double(a)) {};

	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept;
};

class action_group_f : public action_function {
public:
	std::vector<concurrent_uniq<action_function>> lst;

	action_group_f() {};
	action_group_f(IN(std::vector<std::unique_ptr<actionbase>>) v);
	virtual std_fp probability(INOUT(value_mask) i, char_id id) const noexcept;
	virtual std_fp time(INOUT(value_mask) i, char_id id) const noexcept;
	virtual std_fp duration(INOUT(value_mask) i, char_id id) const noexcept;
	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept;
	virtual void prereq(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept;
};

class transfer_land_f : public action_function {
public:
	IN(std::vector<prov_id>) land;
	char_id to;

	transfer_land_f(IN(std::vector<prov_id>) l, char_id t) : land(l), to(t) {};
	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept;
};

class host_event_f : public action_function {
public:
	IN(std::vector<char_id>) guests;
	unsigned int event_id;

	host_event_f(IN(std::vector<char_id>) g, unsigned int i) : guests(g), event_id(i) {};

	virtual std_fp time(INOUT(value_mask) i, char_id id) const noexcept;
	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept;
	virtual void prereq(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept;
};