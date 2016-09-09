#include "globalhelpers.h"
#include "ai_calc.h"
#include "structs.hpp"
#include "events.h"
#include "prov_control.h"
#include "actions.h"

std_fp base_value_mask::get_value(size_t category, size_t value, size_t index, char_id person) noexcept {
	switch (category) {
	case I_TITLES:
		switch (value) {
		case I_TITLES_CAPITAL: return std_fp::from_int(global::titles[index].capital);
		case I_TITLES_CULTURE: return std_fp::from_int(global::titles[index].culture);
		case I_TITLES_RELIGION: return std_fp::from_int(global::titles[index].religion);
		case I_TITLES_HOLDER: return std_fp::from_int(global::titles[index].holder);
		case I_TITLES_LEIGE: return std_fp::from_int(global::titles[index].leige);
		case I_TITLES_VTAX: return global::titles[index].vtax;
		}
		break;
	case I_PEOPLE:
		switch (value) {
		case I_PEOPLE_MONEY: return global::GetWealth(static_cast<char_id>(index));
		case I_PEOPLE_STRENGTH: return global::GetTroopStr(static_cast<char_id>(index), fake_lock());
		case I_PEOPLE_INCOME: return global::project_income(static_cast<char_id>(index), fake_lock());
		case I_PEOPLE_DEFENSIVE: return global::get_defensive_strength(static_cast<char_id>(index), fake_lock());
		//case I_PEOPLE_WARPROB: return std::min(std_fp::from_int(1), std_fp::from_int(global::people[index].blb->ai.interests.threats.size()) * std_fp::from_double(0.1) +
		//									   std_fp::from_int(count_defensive_pacts(static_cast<char_id>(index), fake_lock())) * std_fp::from_double(0.2));
		case I_PEOPLE_DEFENCE_AGAINST: return global::get_defensive_against(person, static_cast<char_id>(index), fake_lock());
		case I_PEOPLE_CULTURE: return std_fp::from_int(global::people[index].culture);
		case I_PEOPLE_RELIGION: return std_fp::from_int(global::people[index].religion);
		case I_PEOPLE_DYNASTY: return std_fp::from_int(global::people[index].dynasty);
		case I_PEOPLE_FATHER: return std_fp::from_int(global::people[index].father);
		case I_PEOPLE_MOTHER: return std_fp::from_int(global::people[index].mother);
		case I_PEOPLE_GENDER: return std_fp::from_int(global::people[index].gender);
		case I_PEOPLE_SPOUSE: return std_fp::from_int(global::people[index].spouse);
		case I_PEOPLE_PRIMET: return std_fp::from_int(global::people[index].primetitle);
		}
		break;
	case I_PROVINCES:
		switch (value) {
		case I_PROVINCES_TITLE: return std_fp::from_int(global::provinces[index].title);
		case I_PROVINCES_TAX: return global::provinces[index].tax;
		}
		break;
	case I_ARRAYS:
		switch (value) {
		//case I_ARRAYS_THREAT_PROB:
		//	if (index == 0) {
		//		return std_fp::from_int(global::people[person].blb->ai.interests.threats.size());
		//	} else if ((index - 1) < global::people[person].blb->ai.interests.threats.size()) {
		//		return std_fp::from_float(std::get<2>(global::people[person].blb->ai.interests.threats[index - 1]));
		//	} else {
		//		return std_fp::from_int(0);
		//	}
		//case I_ARRAYS_THREATS:
		//	if (index == 0) {
		//		return std_fp::from_int(global::people[person].blb->ai.interests.threats.size());
		//	} else if ((index - 1) < global::people[person].blb->ai.interests.threats.size()) {
		//		return std_fp::from_int(std::get<0>(global::people[person].blb->ai.interests.threats[index - 1]));
		//	} else {
		//		return std_fp::from_int(0);
		//	}
		}
	}
	return std_fp::from_int(0);
}



value_mask::value_mask(IN(value_mask) p) : v_map(p.v_map) {
}

value_mask::value_mask(value_mask &&p) : v_map(std::move(p.v_map)) {
}

const value_mask& value_mask::operator=(IN(value_mask) in) noexcept {
	v_map = in.v_map;
	return *this;
}

void value_mask::add_value(size_t category, size_t value, size_t index, std_fp val, char_id id) noexcept {
	auto it = v_map.find(TO_I(category, value, index));
	if (it == v_map.end())
		v_map[TO_I(category, value, index)] = val + base_value_mask::get_value(category, value, index, id);
	else
		it->second += val;
}

void value_mask::set_value(size_t category, size_t value, size_t index, std_fp val) noexcept {
	v_map[TO_I(category, value, index)] = val;
}

std_fp value_mask::get_value(size_t category, size_t value, size_t index, char_id id) noexcept {
	auto it = v_map.find(TO_I(category, value, index));
	if (it == v_map.end())
		if (category == I_PEOPLE && (value == I_PEOPLE_WARPROB || value == I_PEOPLE_DEFENCE_AGAINST)) {
			return v_map[TO_I(category, value, index)] = base_value_mask::get_value(category, value, index, id);
		} else {
			return base_value_mask::get_value(category, value, index, id);
		}
	else
		return it->second;
}

class apply_pact_f : public action_function {
public:
	IN(pact_data) pact;

	apply_pact_f(IN(pact_data) p) : pact(p) {};

	virtual std_fp duration(INOUT(value_mask) i, char_id id) const noexcept {
		if (pact.expiration == 0) {
			return indefinite_duration(global::currentday - global::people[id].born);
		}
		return std_fp::from_int(pact.expiration - global::currentday);
	}

	virtual std_fp probability(INOUT(value_mask) i, char_id id) const noexcept {
		std_fp prob = std_fp::from_int(0);
		for (const auto& __restrict g : pact.guarantees) {
			if (g.type == G_HONOR || g.type == G_MARRIAGE) {
				prob += std_fp::from_double(0.6);
			}
		}
		return std::min(prob, std_fp::from_int(1));
	}

	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
		o = i;
		switch (pact.pact_type) {
		case P_DEFENCE:
			o.add_value(I_PEOPLE, I_PEOPLE_DEFENSIVE, pact.a, i.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, pact.b, id), id);
			o.add_value(I_PEOPLE, I_PEOPLE_DEFENSIVE, pact.b, i.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, pact.a, id), id);
			o.add_value(I_PEOPLE, I_PEOPLE_WARPROB, id, std_fp::from_double(0.2), id);
			break;
		case P_DEFENCE_AGAINST:
			o.add_value(I_PEOPLE, I_PEOPLE_DEFENCE_AGAINST, pact.against, i.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, (pact.a == id) ? pact.b : pact.a, id), id);
			break;
		case P_NON_AGRESSION:
		{
			//make no longer a threat
			size_t threat_count = i.get_value(I_ARRAYS, I_ARRAYS_THREATS, 0, id).to_int();
			const auto othr = std_fp::from_int((pact.a == id) ? pact.b : pact.a);
			for (size_t indx = 1; indx <= threat_count; ++indx) {
				if (othr == i.get_value(I_ARRAYS, I_ARRAYS_THREATS, indx, id)) {
					//o.set_value(I_ARRAYS, I_ARRAYS_THREATS, 0, threat_count - 1);
					//o.set_value(I_ARRAYS, I_ARRAYS_THREATS, indx, i.get_value(I_ARRAYS, I_ARRAYS_THREATS, threat_count, id));
					o.set_value(I_ARRAYS, I_ARRAYS_THREATS, indx, std_fp::from_int(0));
				}
			}
			break;
		}
		}

		for (const auto& __restrict g : pact.guarantees) {
			switch (g.type) {
			case G_HONOR:
				break;
			case G_MARRIAGE:
				break;
			case G_TRIBUTE:
				o.add_value(I_PEOPLE, I_PEOPLE_INCOME, g.data.tribute.to, std_fp::from_float(g.data.tribute.amount), id);
				o.add_value(I_PEOPLE, I_PEOPLE_INCOME, (g.data.tribute.to == pact.a) ? pact.b : pact.a, std_fp::from_float(-g.data.tribute.amount), id);
				break;
			}
		}
	}
};


class remove_pact_f : public action_function {
public:
	IN(pact_data) pact;

	remove_pact_f(IN(pact_data) p) : pact(p) {};

	virtual std_fp duration(INOUT(value_mask) i, char_id id) const noexcept {
		return indefinite_duration(global::currentday - global::people[id].born);
	}

	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
		o = i;
		switch (pact.pact_type) {
		case P_DEFENCE:
			o.add_value(I_PEOPLE, I_PEOPLE_DEFENSIVE, pact.a, -i.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, pact.b, id), id);
			o.add_value(I_PEOPLE, I_PEOPLE_DEFENSIVE, pact.b, -i.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, pact.a, id), id);
			o.add_value(I_PEOPLE, I_PEOPLE_WARPROB, id, std_fp::from_double(-0.2), id);
			break;
		case P_DEFENCE_AGAINST:
			if (pact.a == id || pact.b == id)
				o.add_value(I_PEOPLE, I_PEOPLE_DEFENCE_AGAINST, pact.against, -i.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, (pact.a == id) ? pact.b : pact.a, id), id);
			break;
		case P_NON_AGRESSION:
		{
			const auto threat_count = i.get_value(I_ARRAYS, I_ARRAYS_THREATS, 0, id);
			o.set_value(I_ARRAYS, I_ARRAYS_THREATS, 0, threat_count + std_fp::from_int(1));
			o.set_value(I_ARRAYS, I_ARRAYS_THREATS, threat_count.to_int() + 1, std_fp::from_int((pact.a == id) ? pact.b : pact.a));
			o.set_value(I_ARRAYS, I_ARRAYS_THREATS, threat_count.to_int() + 1, std_fp::from_int(1));
			break;
		}
		}

		for (const auto& __restrict g : pact.guarantees) {
			switch (g.type) {
			case G_HONOR:
				break;
			case G_MARRIAGE:
				break;
			case G_TRIBUTE:
				o.add_value(I_PEOPLE, I_PEOPLE_INCOME, g.data.tribute.to, std_fp::from_float(-g.data.tribute.amount), id);
				o.add_value(I_PEOPLE, I_PEOPLE_INCOME, (g.data.tribute.to == pact.a) ? pact.b : pact.a, std_fp::from_float(g.data.tribute.amount), id);
				break;
			}
		}
	}
};

class save_money_f : public action_function {
public:
	const std_fp amount;

	save_money_f(double a) : amount(std_fp::from_double(a)) {};

	virtual std_fp time(INOUT(value_mask) i, char_id id) const noexcept {
		return ((amount - i.get_value(I_PEOPLE, I_PEOPLE_MONEY, id, id))  * std_fp::from_int(30)) / i.get_value(I_PEOPLE, I_PEOPLE_INCOME, id, id);
	}

	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
		o = i;
		o.add_value(I_PEOPLE, I_PEOPLE_MONEY, id, amount, id);
	}
};

class prosecute_war_f : public action_function {
public:
	char_id against;

	prosecute_war_f(char_id a) : against(a) {};

	virtual void apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
		o = i;
		o.add_value(I_PEOPLE, I_PEOPLE_INCOME, id, -troopCost(i.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, id, id).to_int(), 0), id);
		//TODO
	}
};

std_fp fitness_function(char_id id, INOUT(value_mask) data, IN(ai_action_prefs) prefs) noexcept {
	const auto& __restrict b = global::people[id].blb;
	if (b) {
		const std_fp strength = data.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, id, id);
		const std_fp income = std::max(std_fp::from_int(1),
									   data.get_value(I_PEOPLE, I_PEOPLE_INCOME, id, id)
									   + (data.get_value(I_PEOPLE, I_PEOPLE_MONEY, id, id) * std_fp::from_int(30) / indefinite_duration(global::currentday - global::people[id].born))
									   - data.get_value(I_PEOPLE, I_PEOPLE_WARPROB, id, id) * troopCost(strength.to_int(), 0));
		const std_fp d_strength = data.get_value(I_PEOPLE, I_PEOPLE_DEFENSIVE, id, id) / std_fp::from_int(1000);

		std_fp fitness = income*strength/std_fp::from_int(1000);
		size_t threat_count = data.get_value(I_ARRAYS, I_ARRAYS_THREATS, 0, id).to_int();
		for (size_t i = 1; i <= threat_count; ++i) {
			const auto tid = data.get_value(I_ARRAYS, I_ARRAYS_THREATS, i, id);
			fitness -= std::max(std_fp::from_int(0), data.get_value(I_ARRAYS, I_ARRAYS_THREAT_PROB, i, id) *
								data.get_value(I_PEOPLE, I_PEOPLE_INCOME, tid.to_int(), id) * (((data.get_value(I_PEOPLE, I_PEOPLE_STRENGTH, tid.to_int(), id) - data.get_value(I_PEOPLE, I_PEOPLE_DEFENCE_AGAINST, tid.to_int(), id)) / std_fp::from_int(1000)) - (strength + d_strength)));
		}
		return fitness;
	}
	return std_fp::from_int(0);
}

std_fp percent_difference(INOUT(value_mask) newstate, IN(value_mask) target, INOUT(value_mask) base, char_id id) {
	__int64 cnt = 0;
	std_fp percent = std_fp::from_int(0);
	for (IN(auto) pr : target.v_map) {
		const auto base_dif = base.get_value(I_CATEGORY(pr.first), I_VALUE(pr.first), I_INDEX(pr.first), id) - pr.second;
		if (base_dif != std_fp::from_int(0)) {
			const auto dif = newstate.get_value(I_CATEGORY(pr.first), I_VALUE(pr.first), I_INDEX(pr.first), id) - pr.second;
			percent += (dif / base_dif)*(dif / base_dif);
			cnt += 1;
		}
	}
	/*for (const auto& __restrict pr : target.i_map) {
		if (base.get_value(I_CATEGORY(pr.first), I_VALUE(pr.first), I_INDEX(pr.first), id) != pr.second) {
			if (newstate.get_value(I_CATEGORY(pr.first), I_VALUE(pr.first), I_INDEX(pr.first), id) != pr.second)
				percent += std_fp::from_int(1);
			cnt += 1;
		}
	}/**/

	return std_fp::from_int(1) - (percent / std_fp::from_int(cnt));
}

std_fp action_function::duration(INOUT(value_mask) i, char_id id) const noexcept {
	return indefinite_duration(global::currentday - global::people[id].born);
}

std_fp action_function::value(INOUT(value_mask) state, char_id id, std_fp current_fitness, IN(ai_action_prefs) prefs) const noexcept {
	value_mask newstate;
	apply(newstate, state, id);
	return probability(state, id) * (fitness_function(id, newstate, prefs) - current_fitness) * duration(state, id) / time(state, id);
}

std_fp action_function::instrumental_value(INOUT(value_mask) state, char_id id, IN(value_mask) target_state, std_fp target_value) const noexcept {
	value_mask newstate;
	apply(newstate, state, id);
	return probability(state, id) *  percent_difference(newstate, target_state, state, id) * target_value / time(state, id);
}

action_group_f::action_group_f(IN(std::vector<std::unique_ptr<actionbase>>) v) {
	size_t sz = v.size();
	lst.resize(sz);
	for (size_t i = 0; i < sz; ++i) {
		lst[i] = v[i]->return_function();
	}
}

std_fp action_group_f::probability(INOUT(value_mask) i, char_id id) const noexcept {
	return std::accumulate(lst.begin(), lst.end(), std_fp::from_int(1), [&i, id](std_fp in, IN(concurrent_uniq<action_function>) f) { return std::min(in, f->probability(i, id)); });
}

std_fp action_group_f::time(INOUT(value_mask) i, char_id id) const noexcept {
	return std::accumulate(lst.begin(), lst.end(), std_fp::from_int(1), [&i, id](std_fp in, IN(concurrent_uniq<action_function>) f) { return std::max(in, f->probability(i, id)); });
}

std_fp action_group_f::duration(INOUT(value_mask) i, char_id id) const noexcept {
	return std::accumulate(lst.begin(), lst.end(), std_fp::from_int(1), [&i, id](std_fp in, IN(concurrent_uniq<action_function>) f) { return std::min(in, f->probability(i, id)); });
}

void action_group_f::apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
	for (IN(auto) l : lst) {
		l->apply(o, i, id);
	}
}

void action_group_f::prereq(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
	for (IN(auto) l : lst) {
		l->apply(o, i, id);
	}
}

void change_money_f::apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
	o = i;
	o.add_value(I_PEOPLE, I_PEOPLE_MONEY, id, amount, id);
}

void transfer_land_f::apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
	o = i;
	fake_lock l;
	for (const auto p : land) {
		global::enum_control_by_prov(p,l, [id, &o](INOUT(controlrecord) rec) {
			o.add_value(I_PEOPLE, I_PEOPLE_INCOME, rec.controller, -rec.tax * global::provinces[rec.province].tax, id);
			o.add_value(I_PEOPLE, I_PEOPLE_STRENGTH, rec.controller, -rec.tax * global::provinces[rec.province].tax * std_fp::from_int(1000), id);
		});
		o.add_value(I_PEOPLE, I_PEOPLE_INCOME, to, global::provinces[p].tax, id);
		o.add_value(I_PEOPLE, I_PEOPLE_STRENGTH, to, global::provinces[p].tax * std_fp::from_int(1000), id);
	}
}

std_fp host_event_f::time(INOUT(value_mask) i, char_id id) const noexcept {
	return std_fp::from_int(1 + std::max(global::currentday - (global::people[id].blb->last_event + 30), 0u));
}

void host_event_f::apply(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
	o = i;
	o.add_value(I_PEOPLE, I_PEOPLE_MONEY, id, -event_cost(event_id, guests, id), id);

	IN(auto) ev = event_template_by_id(event_id);

	//adjust threat magnitude by expected relationship changes
	size_t threat_count = i.get_value(I_ARRAYS, I_ARRAYS_THREATS, 0, id).to_int();
	for (size_t indx = 1; indx <= threat_count; ++indx) {
		const char_id threat = static_cast<char_id>(i.get_value(I_ARRAYS, I_ARRAYS_THREATS, indx, id).to_int());
		if (std::find(guests.cbegin(), guests.cend(), threat) != guests.cend()) {
			int rchange = 0;
			relation_change(rchange, ev, id, threat);
			o.add_value(I_ARRAYS, I_ARRAYS_THREAT_PROB, indx, std_fp::from_int(-rchange) / std_fp::from_int(1000), id);
		}
	}
}

void host_event_f::prereq(INOUT(value_mask) o, INOUT(value_mask) i, char_id id) const noexcept {
	o = i;
	const auto cost = event_cost(event_id, guests, id);
	const auto wealth = i.get_value(I_PEOPLE, I_PEOPLE_MONEY, id, id);
	if (wealth < cost) {
		o.set_value(I_PEOPLE, I_PEOPLE_MONEY, id, cost);
	}
}
