#include "globalhelpers.h"
#include "political_action.h"
#include "structs.hpp"
#include "datamanagement.hpp"
#include "relations.h"
#include "action_opinion.h"
#include "reputation.h"
#include "i18n.h"
#include "generated_ui.h"


actionable_list_class_t<political_action, std::pair<char_id_t, char_id_t>, apply_political_action_proposal> proposals;
actionable_list_class<political_action, apply_political_action> political_actions;

multiindex<char_id_t, influence_against> saved_influence_set;

void display_political_action(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(political_action) act, IN(g_lock) l) {
	act.display_description(parent, x, y, l);
}

void save_influence(char_id_t inf_for, IN(influence_against) inf, IN(w_lock) l) {
	
	switch (inf.influence_type) {
	case influence_against::INF_BLACKMAIL:
		//TODO
		if (has_negative_counterpart(inf.type))
			saved_influence_set.emplace(inf_for, l, inf);
		return;
	case influence_against::INF_FAVOR_CALLED_IN:
		if (call_in_favor(inf_for, inf.influence_from, l) & has_negative_counterpart(inf.type))
			saved_influence_set.emplace(inf_for, l, inf);
		return;
	case influence_against::INF_FAVOR_OFFERED:
		add_favor(inf.influence_from, inf_for, l);
		if (has_negative_counterpart(inf.type))
			saved_influence_set.emplace(inf_for, l, inf);
		return;
	case influence_against::INF_FRIENDSHIP:
		if (has_negative_counterpart(inf.type))
			saved_influence_set.emplace(inf_for, l, inf);
		return;
	}
}

void do_influence_ignored(char_id_t inf_for, IN(influence_against) inf, bool ignored_favors, IN(w_lock) l) {
	switch (inf.influence_type) {
	case influence_against::INF_BLACKMAIL:
		//TODO
		return;
	case influence_against::INF_FAVOR_CALLED_IN:
		if (ignored_favors) {
			dishonor_favor(inf_for, inf.influence_from, l);
		}
		return;
	case influence_against::INF_FAVOR_OFFERED:
		return;
	case influence_against::INF_FRIENDSHIP:
		adjust_relation(inf.influence_from, inf_for, -1, l);
		return;
	}
}

void display_positive_influence(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(influence_against) inf, IN(g_lock) l) {
	switch (inf.type) {
	case influence_against::WAR_WITH:
	{
		size_t params[4] = {inf.influence_from.value, inf.influence_type, TX_INFLUENCE_WAR_WITH, inf.data.chid.value};
		x = get_linear_ui(TX_INFLUENCE_POS_FULL, params, 4, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 4;
		return;
	}
	case influence_against::NONE:
	{
		size_t params[3] = {inf.influence_from.value, inf.influence_type, TX_L_ERROR};
		x = get_linear_ui(TX_INFLUENCE_POS_FULL, params, 3, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 4;
		return;
	}
	}
}

void display_negative_influence(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(influence_against) inf, IN(g_lock) l) {
	switch (inf.type) {
	case influence_against::WAR_WITH:
	{
		size_t params[4] = {inf.influence_from.value, inf.influence_type, TX_INFLUENCE_WAR_WITH, inf.data.chid.value};
		x = get_linear_ui(TX_INFLUENCE_NEG_FULL, params, 4, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 4;
		return;
	}
	case influence_against::NONE:
	{
		size_t params[3] = {inf.influence_from.value, inf.influence_type, TX_L_ERROR};
		x = get_linear_ui(TX_INFLUENCE_NEG_FULL, params, 3, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 4;
		return;
	}
	}
}

void display_issue(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(influence_against) inf, IN(g_lock) l) {
	switch (inf.type) {
	case influence_against::WAR_WITH:
	{
		size_t params[4] = {0, 0, 0, inf.data.chid.value};
		x = get_linear_ui(TX_INFLUENCE_WAR_WITH, params, 4, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 4;
		return;
	}
	case influence_against::NONE:
	{
		x = get_linear_ui(TX_L_ERROR, parent, x, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 4;
		return;
	}
	}
}

void erase_saved_influence_set(char_id_t inf_for, IN(std::vector<influence_against>) iset, IN(w_lock) l) {
	saved_influence_set.range_erase_if(inf_for, l, [&iset, isetend = iset.end()](IN(std::pair<char_id_t,influence_against>) i) {
		return std::find(iset.begin(), isetend, i.second) != isetend;
	});
}

int sum_of_favors(char_id_t ch_for, IN(flat_multimap<char_id_t, influence_against>) positive_influence, IN(flat_multimap<char_id_t, influence_against>) negative_influence, IN(std::vector<influence_against>) existing_negative) {
	int sum = 0;
	for (auto pr = positive_influence.equal_range(ch_for); pr.first != pr.second; ++pr.first) {
		if (pr.first->second.influence_type == influence_against::INF_FAVOR_CALLED_IN)
			++sum;
	}
	for (auto pr = negative_influence.equal_range(ch_for); pr.first != pr.second; ++pr.first) {
		if (pr.first->second.influence_type == influence_against::INF_FAVOR_CALLED_IN)
			--sum;
	}
	for (IN(auto) i : existing_negative) {
		if (i.influence_type == influence_against::INF_FAVOR_CALLED_IN)
			--sum;
	}
	return sum;
}

double weight_of_influence(char_id_t inf_for, IN(udata) fd, IN(influence_against) inf, IN(g_lock) l) {
	switch (inf.influence_type) {
	case influence_against::INF_BLACKMAIL:
		//TODO
		return 0.0;
	case influence_against::INF_FAVOR_CALLED_IN:
		// nothing, weighted as a whole, not individually
		return 0.0;
	case influence_against::INF_FAVOR_OFFERED:
		return value_of_favor_from(inf_for, fd, inf.influence_from, l);
	case influence_against::INF_FRIENDSHIP:
		return 0.1 * std::max(0.0, static_cast<double>(opinion(inf_for, fd, inf.influence_from, l)));
	}
	return 0.0;
}

bool can_apply_influence(char_id_t to_influence, char_id_t influence_from, unsigned char influence_type, IN(g_lock) l) {
	switch (influence_type) {
	case influence_against::INF_BLACKMAIL:
		//TODO
		return false;
	case influence_against::INF_FAVOR_CALLED_IN:
		return owed_favors(to_influence, influence_from, l) > 0;
	case influence_against::INF_FAVOR_OFFERED:
		return owed_favors(to_influence, influence_from, l) <= 0;
	case influence_against::INF_FRIENDSHIP:
		return get_mutual_feeling(to_influence, influence_from, l) == 1;
	}
	return false;
}

double sum_total_of_favors(int number_of_favors, double starting_honor) {
	double after = starting_honor;
	for (int i = abs(number_of_favors); i != 0; --i) {
		after = update_reputation(reputation::p_dishonor_favor_reliable, reputation::p_dishonor_favor_unreliable, after);
	}
	return copysign(starting_honor - after, number_of_favors > 0 ? 1.0 : -1.0);
}

double total_weight_of_influence(char_id_t inf_for, IN(udata) fd, IN(flat_multimap<char_id_t, influence_against>) influence, IN(w_lock) l) {
	double total = 0.0;
	for (auto pr = influence.equal_range(inf_for); pr.first != pr.second; ++pr.first)
		total += weight_of_influence(inf_for, fd, pr.first->second, l);
	return total;
}

double total_weight_of_influence(char_id_t inf_for, IN(udata) fd, IN(std::vector<influence_against>) influence, IN(w_lock) l) {
	double total = 0.0;
	for (IN(auto) i : influence)
		total += weight_of_influence(inf_for, fd, i, l);
	return total;
}

double total_weight_of_influence(char_id_t inf_for, IN(udata) fd, IN(cvector<influence_against>) influence, IN(g_lock) l) {
	double total = 0.0;
	for (IN(auto) i : influence)
		total += weight_of_influence(inf_for, fd, i, l);
	return total;
}

template<typename T, typename L>
void _get_influence_against_set(char_id_t inf_for, IN(std::vector<influence_against, T>) base_set, INOUT(std::vector<influence_against, T>) result_set, IN(L) l) {
	saved_influence_set.for_each(inf_for, l, [&base_set, &result_set, bsetend = base_set.end()](IN(influence_against) i) {
		for (auto it = base_set.begin(); it != bsetend; ++it) {
			if (*it == i)
				result_set.push_back(i);
		}
	});
}

void get_influence_against_set(char_id_t inf_for, IN(cvector<influence_against>) base_set, INOUT(cvector<influence_against>) result_set, IN(g_lock) l) {
	_get_influence_against_set(inf_for, base_set, result_set, l);
}
void get_influence_against_set(char_id_t inf_for, IN(std::vector<influence_against>) base_set, INOUT(std::vector<influence_against>) result_set, IN(w_lock) l) {
	_get_influence_against_set(inf_for, base_set, result_set, l);
}

void do_influence_ignored_set(char_id_t inf_for, IN(std::vector<influence_against>) iv, bool ignored_favors, IN(w_lock) l) {
	for (IN(auto) i : iv)
		do_influence_ignored(inf_for, i, ignored_favors, l);
}


double evaluation_to_influence_probability(double eval) {
	return copysign(eval, 1.0) > OPINION_THRESHOLD ? eval * eval : 0.0;
}

const unsigned char inf_permutations[24][4] = {
	{0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}, {0, 3, 2, 1},
	{1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0}, {1, 3, 0, 2}, {1, 3, 2, 0},
	{2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
	{3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0}
};

bool generate_influence(INOUT(influence_against) out, char_id_t to_influence, char_id_t influencer, IN(std::vector<influence_against>) iv, IN(g_lock) l) {
	const auto irng = saved_influence_set.range(to_influence, l);
	const auto ivsz = iv.size();

	const unsigned char* const random_permutation = inf_permutations[random_store::get_fast_int() % 24];
	for (size_t ti = 0; ti != influence_against::INF_MAX_TYPE; ++ti) {
		const unsigned char this_type = random_permutation[ti];

		if (can_apply_influence(to_influence, influencer, this_type, l)) {
			const size_t iv_offset = random_store::get_fast_int() % ivsz;

			for (size_t ivi = 0; ivi != ivsz; ++ivi) {
				IN(auto) base_influence = iv[(ivi + iv_offset) % ivsz];
				bool is_duplicate = false;

				for (auto pr = irng.first; pr != irng.second; ++pr) {
					if ((pr->second == base_influence) & (pr->second.influence_type == this_type) & (pr->second.influence_from == influencer)) {
						is_duplicate = true;
						break;
					}
				}
				if (!is_duplicate) {
					out = influence_against(base_influence, influencer, this_type);
					return true;
				}
			}
		}
	}

	return false;
}

bool get_player_proceed_with_proposal(const IN_P(political_action) act, bool action_needs_vote, double player_eval, char_id_t proposal_from, IN(std::vector<influence_against>) result_set, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(w_lock) l) {
	l.unlock();
	const bool do_proposal = make_yes_no_popup(global::uicontainer, get_simple_string(TX_PROPOSAL), [action_needs_vote, proposal_from, &positive_influence, &result_set, act](IN(std::shared_ptr<uiScrollView>) sv) {
		int y = 1;
		int x = 1;
		size_t param = proposal_from.value;
		const auto tb = create_tex_block(TX_PROPOSAL_MADE_INTRO, &param, 1, sv, x, y, sv->pos.width - 5, global::empty, global::standard_text);
		sv->subelements.push_back(tb);
		y += tb->pos.height;

		r_lock l;
		act->display_description(sv, x, y, l);
		x = 1;

		if ((positive_influence.size() != 0) | (result_set.size() != 0)) {
			y += 6;
			get_linear_ui(TX_RELEVENT_INFLUENCES, sv, x, y, global::empty, global::standard_text);
			y += global::standard_text.csize + 4;
			for (IN(auto) inf : positive_influence) {
				x = 10;
				display_positive_influence(sv, x, y, inf.second, l);
			}
			for (IN(auto) inf : result_set) {
				x = 10;
				display_negative_influence(sv, x, y, inf, l);
			}
		}

		x = 1;
		if (action_needs_vote) {
			const auto tb2 = create_tex_block(TX_PROPOSAL_MADE_VOTE, sv, x, y, sv->pos.width - 5, global::empty, global::standard_text);
			sv->subelements.push_back(tb2);
		} else {
			const auto tb2 = create_tex_block(TX_PROPOSAL_MADE_EXEC, sv, x, y, sv->pos.width - 5, global::empty, global::standard_text);
			sv->subelements.push_back(tb2);
		}
	}, player_eval);
	l.lock();

	return do_proposal;
}

bool get_player_vote(const IN_P(political_action) act, double player_eval, IN(std::vector<influence_against>) result_set, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, INOUT(w_lock) l) {
	l.unlock();
	const bool vote_for = make_yes_no_popup(global::uicontainer, get_simple_string(TX_VOTE), [act, &positive_influence, &negative_influence, &result_set](IN(std::shared_ptr<uiScrollView>) sv) {
		int y = 1;
		int x = 1;
		const auto tb = create_tex_block(TX_VOTE_INTRO, sv, x, y, sv->pos.width - 5, global::empty, global::standard_text);
		sv->subelements.push_back(tb);
		y += tb->pos.height;

		r_lock l;
		act->display_description(sv, x, y, l);
		x = 1;

		if ((positive_influence.size() != 0) | (result_set.size() != 0) | (negative_influence.size() != 0)) {
			y += 6;
			get_linear_ui(TX_RELEVENT_INFLUENCES, sv, x, y, global::empty, global::standard_text);
			y += global::standard_text.csize + 4;
			for (IN(auto) inf : positive_influence) {
				x = 10;
				display_positive_influence(sv, x, y, inf.second, l);
			}
			for (IN(auto) inf : negative_influence) {
				x = 10;
				display_negative_influence(sv, x, y, inf.second, l);
			}
			for (IN(auto) inf : result_set) {
				x = 10;
				display_negative_influence(sv, x, y, inf, l);
			}
		}

		x = 1;
		const auto tb2 = create_tex_block(TX_VOTE_END, sv, x, y, sv->pos.width - 5, global::empty, global::standard_text);
		sv->subelements.push_back(tb2);
	}, player_eval);
	l.lock();

	return vote_for;
}

bool get_player_take_action(const IN_P(political_action) act, double player_eval, IN(std::vector<std::pair<char_id_t, int>>) council_members, IN(std::vector<influence_against>) result_set, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, INOUT(w_lock) l) {
	l.unlock();
	const bool do_action = make_yes_no_popup(global::uicontainer, get_simple_string(TX_PROPOSAL), [&council_members, act, &positive_influence, &negative_influence, &result_set](IN(std::shared_ptr<uiScrollView>) sv) {
		int y = 1;
		int x = 1;
		const auto tb = create_tex_block(TX_ACTION_INTRO, sv, x, y, sv->pos.width - 5, global::empty, global::standard_text);
		sv->subelements.push_back(tb);
		y += tb->pos.height;

		r_lock l;
		act->display_description(sv, x, y, l);
		x = 1;

		get_linear_ui(TX_C_OPPOSED, sv, 1, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 4;

		bool found = false;
		for (IN(auto) p : council_members) {
			if (p.second == -1) {
				sv->add_element<ui_ch_hlink>(10, y, p.first.value);
				y += global::standard_text.csize + 4;
				found = true;
			}
		}
		if (!found) {
			get_linear_ui(TX_NONE, sv, 10, y, global::empty, global::standard_text);
			y += global::standard_text.csize + 4;
		}

		get_linear_ui(TX_C_INFAVOR, sv, 1, y, global::empty, global::standard_text);
		y += global::standard_text.csize + 4;

		found = false;
		for (IN(auto) p : council_members) {
			if (p.second == 1) {
				sv->add_element<ui_ch_hlink>(10, y, p.first.value);
				y += global::standard_text.csize + 4;
				found = true;
			}
		}

		if (!found) {
			get_linear_ui(TX_NONE, sv, 10, y, global::empty, global::standard_text);
			y += global::standard_text.csize + 4;
		}

		if ((positive_influence.size() != 0) | (result_set.size() != 0) | (negative_influence.size() != 0)) {
			y += 6;
			get_linear_ui(TX_RELEVENT_INFLUENCES, sv, x, y, global::empty, global::standard_text);
			y += global::standard_text.csize + 4;
			for (IN(auto) inf : positive_influence) {
				x = 10;
				display_positive_influence(sv, x, y, inf.second, l);
			}
			for (IN(auto) inf : negative_influence) {
				x = 10;
				display_negative_influence(sv, x, y, inf.second, l);
			}
			for (IN(auto) inf : result_set) {
				x = 10;
				display_negative_influence(sv, x, y, inf, l);
			}
		}

		x = 1;
		const auto tb2 = create_tex_block(TX_ACTION_END, sv, x, y, sv->pos.width - 5, global::empty, global::standard_text);
		sv->subelements.push_back(tb2);
	}, player_eval);
	l.lock();

	return do_action;
}

flat_multimap<char_id_t, influence_against> inf_to_add_set;
int inf_to_add_stance;
const political_action* global_action_for = nullptr;
influence_display_data inf_disp_dat;

int get_influence_results_from_player(const IN_P(political_action) action_for, IN(std::vector<influence_against>) influences_involved, IN(std::vector<char_id_t>) influence_targets, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, bool is_proposal, INOUT(w_lock) l) {
	inf_to_add_stance = is_proposal ? 1 : 0;
	inf_to_add_set.clear();
	global_action_for = action_for;


	inf_disp_dat.only_positive = is_proposal;
	inf_disp_dat.to_influence.clear();
	inf_disp_dat.to_influence.insert(inf_disp_dat.to_influence.end(), influence_targets.begin(), influence_targets.end());
	inf_disp_dat.infl_against.clear();
	inf_disp_dat.infl_against.insert(inf_disp_dat.infl_against.end(), influences_involved.begin(), influences_involved.end());

	if (!is_proposal) {
		vector_erase_if(inf_disp_dat.to_influence, [&positive_influence](char_id_t id) { //remove anyone who was influenced in the proposal stage
			for (IN(auto) pr : positive_influence) {
				if ((pr.second.influence_from == global::playerid) & (pr.first == id))
					return true;
			}
			return false;
		});
	}

	l.unlock();
	multi_influence::open(*(action_for), inf_disp_dat);
	l.lock();

	if (inf_to_add_stance == 1) {
		positive_influence.insert(boost::container::ordered_range, inf_to_add_set.begin(), inf_to_add_set.end());
	} else if (inf_to_add_stance == -1) {
		negative_influence.insert(boost::container::ordered_range, inf_to_add_set.begin(), inf_to_add_set.end());
	}
	return inf_to_add_stance;
}

update_record modal_influence_window([] {
	if (multi_influence::multi_influence_window->gVisible()) {
		r_lock l;
		multi_influence::update(*global_action_for, inf_disp_dat, l);
	}
});

void multi_influence::stance_options(IN(std::shared_ptr<uiDropDown>) dd) {
	dd->add_option(get_simple_string(TX_APPROVE), []() {
		inf_to_add_stance = 1;
	});
	dd->add_option(get_simple_string(TX_ABSTAIN), []() {
		inf_to_add_stance = 0;
	});
	dd->add_option(get_simple_string(TX_OPPOSE), []() {
		inf_to_add_stance = -1;
	});
	switch (inf_to_add_stance) {
	case 1:
		dd->text.setString(get_simple_string(TX_APPROVE));
		inf_to_add_set.clear();
		modal_influence_window.needs_update = true;
		break;
	case 0:
		dd->text.setString(get_simple_string(TX_ABSTAIN));
		inf_to_add_set.clear();
		modal_influence_window.needs_update = true;
		break;
	case -1:
		dd->text.setString(get_simple_string(TX_OPPOSE));
		inf_to_add_set.clear();
		modal_influence_window.needs_update = true;
		break;
	}
}

std::function<void(uiButton*)> multi_influence::done_button_action(IN(political_action) obj, IN(influence_display_data) p1, INOUT(event) signal, IN(g_lock) l) {
	multi_influence::multi_influence_window->setVisible(false);
	signal.set();
}


std::function<void(uiCheckBox*)> disp_influence_by_issue::call_favor_action(IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	return  [p1, obj](uiCheckBox* cb) {
		if (cb->toggled) {
			inf_to_add_set.emplace(p1, influence_against(obj, global::playerid, influence_against::INF_FAVOR_CALLED_IN));
		} else {
			auto rng = inf_to_add_set.equal_range(p1);
			for (; rng.first != rng.second; ++rng.first) {
				if (rng.first->second == obj && rng.first->second.influence_type == influence_against::INF_FAVOR_CALLED_IN) {
					inf_to_add_set.erase(rng.first);
					break;
				}
			}
		}
	};
}

std::function<void(uiCheckBox*)> disp_influence_by_issue::blackmail_action(IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	return  [p1, obj](uiCheckBox* cb) {
		if (cb->toggled) {
			inf_to_add_set.emplace(p1, influence_against(obj, global::playerid, influence_against::INF_BLACKMAIL));
		} else {
			auto rng = inf_to_add_set.equal_range(p1);
			for (; rng.first != rng.second; ++rng.first) {
				if (rng.first->second == obj && rng.first->second.influence_type == influence_against::INF_BLACKMAIL) {
					inf_to_add_set.erase(rng.first);
					break;
				}
			}
		}
	};
}

std::function<void(uiCheckBox*)> disp_influence_by_issue::friendship_action(IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	return  [p1, obj](uiCheckBox* cb) {
		if (cb->toggled) {
			inf_to_add_set.emplace(p1, influence_against(obj, global::playerid, influence_against::INF_FRIENDSHIP));
		} else {
			auto rng = inf_to_add_set.equal_range(p1);
			for (; rng.first != rng.second; ++rng.first) {
				if (rng.first->second == obj && rng.first->second.influence_type == influence_against::INF_FRIENDSHIP) {
					inf_to_add_set.erase(rng.first);
					break;
				}
			}
		}
	};
}

std::function<void(uiCheckBox*)> disp_influence_by_issue::offer_favor_action(IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	return  [p1, obj](uiCheckBox* cb) {
		if (cb->toggled) {
			inf_to_add_set.emplace(p1, influence_against(obj, global::playerid, influence_against::INF_FAVOR_OFFERED));
		} else {
			auto rng = inf_to_add_set.equal_range(p1);
			for (; rng.first != rng.second; ++rng.first) {
				if (rng.first->second == obj && rng.first->second.influence_type == influence_against::INF_FAVOR_OFFERED) {
					inf_to_add_set.erase(rng.first);
					break;
				}
			}
		}
	};
}

void disp_influence_by_issue::disable_call_favor(IN(std::shared_ptr<uiCheckBox>) element, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	if (inf_to_add_stance == 0) {
		element->disable(get_simple_string(TX_SELECT_STANCE));
	} else if (!can_apply_influence(p1, global::playerid, influence_against::INF_FAVOR_CALLED_IN, l)) {
		element->disable(get_simple_string(TX_INFLUENCE_NO_FAVORS));
	} else {
		for (auto pr = saved_influence_set.range(p1, l); pr.first != pr.second; ++pr.first) {
			if ((pr.first->second == obj) && (pr.first->second.influence_type == influence_against::INF_FAVOR_CALLED_IN) && (pr.first->second.influence_from == global::playerid)) {
				element->disable(get_simple_string(TX_INFLUENCE_ALREADY_APPLIED));
				break;
			}
		}
	}
}

void disp_influence_by_issue::disable_offer_favor(IN(std::shared_ptr<uiCheckBox>) element, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	if (inf_to_add_stance == 0) {
		element->disable(get_simple_string(TX_SELECT_STANCE));
	} else if (!can_apply_influence(p1, global::playerid, influence_against::INF_FAVOR_OFFERED, l)) {
		element->disable(get_simple_string(TX_INFLUENCE_FAV_OWED));
	} else {
		for (auto pr = saved_influence_set.range(p1, l); pr.first != pr.second; ++pr.first) {
			if ((pr.first->second == obj) && (pr.first->second.influence_type == influence_against::INF_FAVOR_OFFERED) && (pr.first->second.influence_from == global::playerid)) {
				element->disable(get_simple_string(TX_INFLUENCE_ALREADY_APPLIED));
				break;
			}
		}
	}
}

void disp_influence_by_issue::disable_friendship(IN(std::shared_ptr<uiCheckBox>) element, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	if (inf_to_add_stance == 0) {
		element->disable(get_simple_string(TX_SELECT_STANCE));
	} else if (!can_apply_influence(p1, global::playerid, influence_against::INF_FRIENDSHIP, l)) {
		element->disable(get_simple_string(TX_INFLUENCE_NOT_FR));
	} else {
		for (auto pr = saved_influence_set.range(p1, l); pr.first != pr.second; ++pr.first) {
			if ((pr.first->second == obj) && (pr.first->second.influence_type == influence_against::INF_FRIENDSHIP) && (pr.first->second.influence_from == global::playerid)) {
				element->disable(get_simple_string(TX_INFLUENCE_ALREADY_APPLIED));
				break;
			}
		}
	}
}

void disp_influence_by_issue::disable_blackmail(IN(std::shared_ptr<uiCheckBox>) element, IN(influence_against) obj, IN(char_id_t) p1, IN(influence_display_data) p2, IN(g_lock) l) {
	if (inf_to_add_stance == 0) {
		element->disable(get_simple_string(TX_SELECT_STANCE));
	} else if (!can_apply_influence(p1, global::playerid, influence_against::INF_BLACKMAIL, l)) {
		element->disable(get_simple_string(TX_INFLUENCE_NO_BLACKMAIL));
	} else {
		for (auto pr = saved_influence_set.range(p1, l); pr.first != pr.second; ++pr.first) {
			if ((pr.first->second == obj) && (pr.first->second.influence_type == influence_against::INF_BLACKMAIL) && (pr.first->second.influence_from == global::playerid)) {
				element->disable(get_simple_string(TX_INFLUENCE_ALREADY_APPLIED));
				break;
			}
		}
	}
}