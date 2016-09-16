#pragma once
#include "globalhelpers.h"
#include "political_action.h"
#include "living_data.h"
#include "generated_ui.h"
#include "i18n.h"
#include "uielements.hpp"
#include "datamanagement.hpp"
#include "relations.h"

template<typename T>
class political_action_t : public political_action {
public:
	virtual void propose(char_id_t proposal_from, char_id_t proposal_to, INOUT(w_lock) l) const override final;
	virtual void take_action(INOUT(w_lock) l) const override final;
	
	void vote_on(INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, char_id_t proposal_from, char_id_t proposal_to, INOUT(w_lock) l) const;
	void take_action_inner(INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, char_id_t proposal_from, INOUT(w_lock) l) const;
	int get_influence(char_id_t influencer, char_id_t exclude, IN(std::vector<char_id_t>) influence_targets, IN(std::vector<influence_against>) influences_involved, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, bool is_proposal, INOUT(w_lock) l) const;
	int get_influence_single(char_id_t influencer, char_id_t influence_target, IN(std::vector<influence_against>) influences_involved, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, INOUT(w_lock) l) const;

	double evaluate_potential_political_action(char_id_t by, IN(g_lock) l) const;
};

#define POL_ACTION_CLASS(name, type) class name : public type, political_action_t<name>

template<typename T>
void political_action_t<T>::take_action(INOUT(w_lock) l) const {
	if (!((T*)this)->is_possible(l)) {
		return;
	}

	flat_multimap<char_id_t, influence_against> dummy_positive_influence;

	if (!((T*)this)->needs_vote(l)) {
		take_action_inner(dummy_positive_influence, char_id_t(), l);
	} else {
		vote_on(dummy_positive_influence, char_id_t(), char_id_t(), l);
	}
}

template<typename T>
void political_action_t<T>::propose(char_id_t proposal_from, char_id_t proposal_to, INOUT(w_lock) l) const {
	if (!((T*)this)->is_possible(l)) {
		return;
	}

	flat_multimap<char_id_t, influence_against> positive_influence;

	std::vector<influence_against> iv;
	((T*)this)->influences_involved(iv, l);


	if (proposal_from != global::playerid) {
		influence_against out;
		if (generate_influence(out, proposal_to, proposal_from, iv, l)) {
			positive_influence.emplace(proposal_to, out);
		}
	} else {
		flat_multimap<char_id_t, influence_against> dummy_negative;
		std::vector<char_id_t> influence_targets(1);
		influence_targets.push_back(proposal_to);
		get_influence(global::playerid, char_id_t(), influence_targets, iv, positive_influence, dummy_negative, true, l);
	}

	bool do_proposal = false;


	std::vector<influence_against> result_set;
	get_influence_against_set(proposal_to, iv, result_set, l);

	int favor_sum = 0;
	{
		flat_multimap<char_id_t, influence_against> dummy_negative;
		favor_sum = sum_of_favors(proposal_to, positive_influence, dummy_negative, result_set);
	}

	if (proposal_to == global::playerid) {
		const double bias = with_udata(proposal_to, l, 0.0, [th = ((T*)this), proposal_to, &l, &result_set, &positive_influence, favor_sum](IN(udata) d) {
			return value_of_honor(sum_total_of_favors(favor_sum, d.p_honorable), d) + th->evaluate_action(proposal_to, l) + total_weight_of_influence(proposal_to, d, positive_influence, l) - total_weight_of_influence(proposal_to, d, result_set, l);
		});
		do_proposal = get_player_proceed_with_proposal(this, ((T*)this)->needs_vote(l), bias, proposal_from, result_set, positive_influence, l);
	} else {
		do_proposal = with_udata(proposal_to, l, false, [th = ((T*)this), proposal_to, &l, &result_set, &positive_influence, favor_sum](IN(udata) d) {
			return value_of_honor(sum_total_of_favors(favor_sum, d.p_honorable), d) + th->evaluate_action(proposal_to, l) + total_weight_of_influence(proposal_to, d, positive_influence, l) - total_weight_of_influence(proposal_to, d, result_set, l) > 0.0;
		});
	}

	if (do_proposal) {
		if (!((T*)this)->needs_vote(l)) {
			if (proposal_from == global::playerid) {
				message_popup(global::uicontainer, get_simple_string(TX_PROPOSAL), [proposal_to, th = ((T*)this), &l](IN(std::shared_ptr<uiScrollView>) sv) {
					size_t param = proposal_to.value;
					const auto tb = create_tex_block(TX_PROPOSAL_ACCEPTED_EXEC, &param, 1, sv, 1, 1, sv->pos.width - 5, global::empty, global::standard_text);
					sv->subelements.push_back(tb);
					int y = tb->pos.height + 10;
					int x = 1;
					th->display_description(sv, x, y, l);
				});
			}

			take_action_inner(positive_influence, proposal_from, l);
		} else {
			if (proposal_from == global::playerid) {
				message_popup(global::uicontainer, get_simple_string(TX_PROPOSAL), [proposal_to, th = ((T*)this), &l](IN(std::shared_ptr<uiScrollView>) sv) {
					size_t param = proposal_to.value;
					const auto tb = create_tex_block(TX_PROPOSAL_ACCEPTED_VOTE, &param, 1, sv, 1, 1, sv->pos.width - 5, global::empty, global::standard_text);
					sv->subelements.push_back(tb);
					int y = tb->pos.height + 10;
					int x = 1;
					th->display_description(sv, x, y, l);
				});
			}

			vote_on(positive_influence, proposal_from, proposal_to, l);
		}
	} else {
		if (proposal_from == global::playerid) {
			message_popup(global::uicontainer, get_simple_string(TX_PROPOSAL), [proposal_to, th = ((T*)this), &l](IN(std::shared_ptr<uiScrollView>) sv) {
				size_t param = proposal_to.value;
				const auto tb = create_tex_block(TX_PROPOSAL_REFUSED, &param, 1, sv, 1, 1, sv->pos.width - 5, global::empty, global::standard_text);
				sv->subelements.push_back(tb);
				int y = tb->pos.height + 10;
				int x = 1;
				th->display_description(sv, x, y, l);
			});
		}
		for (IN(auto) pr : positive_influence) {
			do_influence_ignored(pr.first, pr.second, favor_sum > 0, l);
		}
	}
}

template<typename T>
void political_action_t<T>::vote_on(INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, char_id_t proposal_from, char_id_t proposal_to, INOUT(w_lock) l) const {
	std::vector<char_id_t> voters;
	((T*)this)->get_voters(voters, l);

	std::vector<char_id_t> council_members;
	global::enum_council(((T*)this)->get_admin(l), l, [&voters, &council_members](char_id_t id) {
		const auto iend = voters.end();
		const auto mend = council_members.end();
		if (std::find(voters.begin(), iend, id) == iend &&
			std::find(council_members.begin(), mend, id) == mend) {
			council_members.emplace_back(id);
		}
	});

	flat_multimap<char_id_t, influence_against> negative_influence;
	std::vector<influence_against> iv;
	((T*)this)->influences_involved(iv, l);

	for (IN(auto) interested : voters) {
		get_influence(interested, interested == proposal_from ? proposal_to : char_id_t(), voters, iv, positive_influence, negative_influence, false, l);
	}
	for (const auto interested : council_members) {
		get_influence(interested, interested == proposal_from ? proposal_to : char_id_t(), voters, iv, positive_influence, negative_influence, false, l);
	}
	int vote_total = 0;

	
	std::vector<influence_against> result_set;

	for (INOUT(auto) voter : voters) {
		result_set.clear();
		get_influence_against_set(voter, iv, result_set, l);

		const int favor_sum = sum_of_favors(voter, positive_influence, negative_influence, result_set);

		bool vote_for = false;
		if (voter == global::playerid) {
			const double positive_bias = with_udata(global::playerid, l, 0.0, [th = ((T*)this), &l, &positive_influence, &negative_influence, &result_set, favor_sum](IN(udata) d) {
				return value_of_honor(sum_total_of_favors(favor_sum, d.p_honorable), d) + th->evaluate_action(global::playerid, l) + total_weight_of_influence(global::playerid, d, positive_influence, l) - total_weight_of_influence(global::playerid, d, result_set, l) - total_weight_of_influence(global::playerid, d, negative_influence, l);
			});;
			vote_for = get_player_vote(this, positive_bias, result_set, positive_influence, negative_influence, l);
		} else {
			vote_for = with_udata(voter, l, false, [th = ((T*)this), voter, &l, &positive_influence, &negative_influence, &result_set, favor_sum](IN(udata) d) {
				return value_of_honor(sum_total_of_favors(favor_sum, d.p_honorable), d) + th->evaluate_action(voter, l) + total_weight_of_influence(voter, d, positive_influence, l) - total_weight_of_influence(voter, d, result_set, l) - total_weight_of_influence(voter, d, negative_influence, l) > 0.0;
			});
		}

		if (vote_for) {
			++vote_total;
			for (auto pr = negative_influence.equal_range(voter); pr.first != pr.second; ++pr.first) {
				do_influence_ignored(voter, pr.first->second, favor_sum < 0, l);
			}
			for (auto pr = positive_influence.equal_range(voter); pr.first != pr.second; ++pr.first) {
				save_influence(voter, pr.first->second, l);
			}
			do_influence_ignored_set(voter, result_set, favor_sum < 0, l);
			erase_saved_influence_set(voter, iv, l);
		} else {
			--vote_total;
			for (auto pr = positive_influence.equal_range(voter); pr.first != pr.second; ++pr.first) {
				do_influence_ignored(voter, pr.first->second, favor_sum > 0, l);
			}
			for (auto pr = negative_influence.equal_range(voter); pr.first != pr.second; ++pr.first) {
				save_influence(voter, pr.first->second, l);
			}
		}
	}

	if (vote_total > 0) {
		((T*)this)->do_action(l);
	}
}

template<typename T>
void political_action_t<T>::take_action_inner(INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, char_id_t proposal_from, INOUT(w_lock) l) const {
	const auto executor = ((T*)this)->get_executor(l);

	std::vector<std::pair<char_id_t, int>> council_members;
	global::enum_council(((T*)this)->get_admin(l), l, [executor, &council_members](char_id_t id) {
		const auto mend = council_members.end();
		if ((id != executor) & (std::find(council_members.begin(), mend, std::make_pair(id, 0)) == mend)) {
			council_members.emplace_back(id, 0);
		}
	});


	std::vector<influence_against> iv;
	((T*)this)->influences_involved(iv, l);
	flat_multimap<char_id_t, influence_against> negative_influence;

	for (INOUT(auto) interested : council_members) {
		if ((interested.first != proposal_from) | (interested.first == global::playerid)) {
			interested.second = get_influence_single(interested.first, executor, iv, positive_influence, negative_influence, l);
		}
	}

	
	std::vector<influence_against> result_set(iv.size());
	get_influence_against_set(executor, iv, result_set, l);


	const int favor_sum = sum_of_favors(executor, positive_influence, negative_influence, result_set);

	bool do_action = false;
	if (executor == global::playerid) {
		const double positive_bias = with_udata(global::playerid, l, 0.0, [act = (T*)this, &l, &positive_influence, &negative_influence, &result_set, favor_sum](IN(udata) d) {
			return value_of_honor(sum_total_of_favors(favor_sum, d.p_honorable), d) + act->evaluate_action(global::playerid, l) + total_weight_of_influence(global::playerid, d, positive_influence, l) - total_weight_of_influence(global::playerid, d, result_set, l) - total_weight_of_influence(global::playerid, d, negative_influence, l);
		});
		do_action = get_player_take_action(this, positive_bias, council_members, result_set, positive_influence, negative_influence, l);
	} else {
		do_action = with_udata(executor, l, false, [th = ((T*)this), executor, &l, &positive_influence, &negative_influence, &result_set, favor_sum](IN(udata) d) {
			return value_of_honor(sum_total_of_favors(favor_sum, d.p_honorable), d) + th->evaluate_action(executor, l) + total_weight_of_influence(executor, d, positive_influence, l) - total_weight_of_influence(executor, d, result_set, l) - total_weight_of_influence(executor, d, negative_influence, l) > 0.0;
		});
	}

	if (do_action) {
		for (IN(auto) pr : negative_influence) {
			do_influence_ignored(executor, pr.second, favor_sum < 0, l);
		}
		for (IN(auto) pr : positive_influence) {
			save_influence(executor, pr.second, l);
		}
		do_influence_ignored_set(executor, result_set, favor_sum < 0, l);
		for (IN(auto) cm : council_members) {
			if (cm.second > 0)
				adjust_relation(cm.first, executor, 1, l);
			else if (cm.second < 0)
				adjust_relation(cm.first, executor, -1, l);
		}
		erase_saved_influence_set(executor, iv, l);
		((T*)this)->do_action(l);
	} else {
		for (IN(auto) pr : positive_influence) {
			do_influence_ignored(executor, pr.second, favor_sum > 0, l);
		}
		for (IN(auto) pr : negative_influence) {
			save_influence(executor, pr.second, l);
		}
	}
}

template<typename T>
double political_action_t<T>::evaluate_potential_political_action(char_id_t by, IN(g_lock) l) const {
	cvector<influence_against> iv;
	((T*)this)->influences_involved(iv, l);
	cvector<influence_against> result_set;
	get_influence_against_set(by, iv, result_set, l);
	int favors = 0;
	for (IN(auto) inf : result_set) {
		if (inf.influence_type == influence_against::INF_FAVOR_CALLED_IN) {
			++favors;
		}
	}
	return with_udata(by, l, 0.0, [th = ((T*)this), by, &l, &result_set, favors](IN(udata) d) {
		return th->evaluate_action(by, l) - total_weight_of_influence(by, d, result_set, l) - value_of_honor(sum_total_of_favors(favors, d.p_honorable), d);
	});
}

template<typename T>
int political_action_t<T>::get_influence_single(char_id_t influencer, char_id_t influence_target, IN(std::vector<influence_against>) influences_involved, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, INOUT(w_lock) l) const {
	const double eval = ((T*)this)->evaluate_action(influencer, l);
	if (influencer == global::playerid) {
		std::vector<char_id_t> influence_targets(1);
		influence_targets.push_back(influence_target);
		return get_influence_results_from_player(this, influences_involved, influence_targets, positive_influence, negative_influence, false, l);
	} else {
		const double prob = evaluation_to_influence_probability(eval);

		if (random_store::get_fast_double() < prob) {
			influence_against out;
			if (generate_influence(out, influence_target, influencer, influences_involved, l)) {
				if (eval > 0) {
					positive_influence.emplace(influence_target, out);
				} else {
					negative_influence.emplace(influence_target, out);
				}
			}
		}
		
		return (eval > OPINION_THRESHOLD) - (eval < -OPINION_THRESHOLD);
	}
}

template<typename T>
int political_action_t<T>::get_influence(char_id_t influencer, char_id_t exclude, IN(std::vector<char_id_t>) influence_targets, IN(std::vector<influence_against>) influences_involved, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, bool is_proposal, INOUT(w_lock) l) const {
	const double eval = ((T*)this)->evaluate_action(influencer, l);

	if (influencer == global::playerid) {
		return get_influence_results_from_player(this, influences_involved, influence_targets, positive_influence, negative_influence, is_proposal, l);
	} else {
		const double prob = evaluation_to_influence_probability(eval);

		for (const auto it : influence_targets) {
			if (it == exclude)
				continue;
			if (random_store::get_fast_double() >= prob)
				continue;
			influence_against out;
			if (generate_influence(out, it, influencer, influences_involved, l)) {
				if (eval > 0) {
					positive_influence.emplace(it, out);
				} else {
					negative_influence.emplace(it, out);
				}
			}
		}
		return (eval > OPINION_THRESHOLD) - (eval < -OPINION_THRESHOLD);
	}
}