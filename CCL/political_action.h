#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"
#include "living_data.h"

struct influence_against {
	union iad {
		char_id_t chid;
		iad() : chid() {};
	} data;

	char_id_t influence_from;
	unsigned char influence_type = 0;
	unsigned char type = 0;

	static constexpr unsigned char INF_FRIENDSHIP = 0;
	static constexpr unsigned char INF_FAVOR_OFFERED = 1;
	static constexpr unsigned char INF_FAVOR_CALLED_IN = 2;
	static constexpr unsigned char INF_BLACKMAIL = 3;
	static constexpr unsigned char INF_MAX_TYPE = 4;

	static constexpr unsigned char NONE = 0;
	static constexpr unsigned char WAR_WITH = 1;
	static constexpr unsigned char FIRST_NEGATIVE = 2;

	bool operator==(IN(influence_against) other) const noexcept {
		if (type != other.type)
			return false;

		switch (type) {
		case WAR_WITH:
			return data.chid == other.data.chid;
		default:
			return true;
		}
	}
	bool operator<(IN(influence_against) other) const noexcept {
		if (type < other.type) {
			return true;
		}  else if (type == other.type) {
			switch (type) {
			case WAR_WITH:
				return data.chid.value < other.data.chid.value;
			default:
				return false;
			}
		} else {
			return false;
		}
	}
	influence_against() {};
	influence_against(IN(influence_against) base, char_id_t from, unsigned char type) : influence_against(base) {
		influence_from = from;
		influence_type = type;
	}
};

inline bool has_negative_counterpart(unsigned char influence_type) noexcept {
	return influence_type >= influence_against::FIRST_NEGATIVE;
}

class political_action {
public:
	virtual void propose(char_id_t proposal_from, char_id_t proposal_to, INOUT(w_lock) l) const { abort(); };
	virtual void vote_on(INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, char_id_t proposal_from, char_id_t proposal_to, INOUT(w_lock) l) const { abort(); };
	virtual void take_action(INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, char_id_t proposal_from, INOUT(w_lock) l) const { abort(); };
	virtual void display_description(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(g_lock) l) const { abort(); };
	virtual bool is_possible(IN(g_lock) l) const { abort(); };
	// virtual void do_action(IN(w_lock) l) const = 0;
	// virtual double evaluate_action(char_id_t by, IN(g_lock) l) const = 0;
	// virtual void influences_involved(INOUT(std::vector<influence_against>) v, IN(w_lock) l) const = 0;
	// virtual void influences_involved(INOUT(cvector<influence_against>) v, IN(g_lock) l) const = 0;
	// virtual bool needs_vote(IN(g_lock) l) const = 0;
	// virtual void get_voters(INOUT(std::vector<char_id_t>) voters, IN(w_lock) l) const = 0;
	// virtual char_id_t get_executor(IN(g_lock) l) const = 0;
	// virtual admin_id_t get_admin(IN(g_lock) l) const = 0;
	virtual ~political_action() {};
};

class influence_display_data {
public:
	cvector<influence_against> infl_against;
	cvector<char_id_t> to_influence;
	bool only_positive;
};

struct apply_political_action_proposal {
	static void apply(IN(std::pair<char_id_t, char_id_t>) ex, INOUT(political_action) act) {
		if (act.is_possible(fake_lock())) {
			w_lock l;
			act.propose(ex.first, ex.second, l);
		}
	}
};

struct apply_political_action_vote {
	static void apply(INOUT(political_action) act) {
		if (act.is_possible(fake_lock())) {
			w_lock l;
			flat_multimap<char_id_t, influence_against> dummy_positive_influence;
			act.vote_on(dummy_positive_influence, char_id_t(), char_id_t(), l);
		}
	}
};

struct apply_political_action_execution {
	static void apply(INOUT(political_action) act) {
		if (act.is_possible(fake_lock())) {
			w_lock l;
			flat_multimap<char_id_t, influence_against> dummy_positive_influence;
			act.take_action(dummy_positive_influence, char_id_t(), l);
		}
	}
};

extern actionable_list_class_t<political_action, std::pair<char_id_t, char_id_t>, apply_political_action_proposal> proposals;
extern actionable_list_class<political_action, apply_political_action_vote> political_action_voting;
extern actionable_list_class<political_action, apply_political_action_proposal> political_action_execution;


extern update_record modal_influence_window;


void display_political_action(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(political_action) act, IN(g_lock) l);
void save_influence(char_id_t inf_for, IN(influence_against) inf, IN(w_lock) l);
void do_influence_ignored(char_id_t inf_for, IN(influence_against) inf, bool ignored_favors, IN(w_lock) l);
double weight_of_influence(char_id_t inf_for, IN(udata) fd, IN(influence_against) inf, IN(g_lock) l);
double total_weight_of_influence(char_id_t inf_for, IN(udata) fd, IN(flat_multimap<char_id_t, influence_against>) influence, IN(w_lock) l);
double total_weight_of_influence(char_id_t inf_for, IN(udata) fd, IN(std::vector<influence_against>) influence, IN(w_lock) l);
void do_influence_ignored_set(char_id_t inf_for, IN(std::vector<influence_against>) iv, bool ignored_favors, IN(w_lock) l);
void get_influence_against_set(char_id_t inf_for, IN(std::vector<influence_against>) base_set, INOUT(std::vector<influence_against>) result_set, IN(w_lock) l);
void get_influence_against_set(char_id_t inf_for, IN(cvector<influence_against>) base_set, INOUT(cvector<influence_against>) result_set, IN(g_lock) l);
void erase_saved_influence_set(char_id_t inf_for, IN(std::vector<influence_against>) iset, IN(w_lock) l);
bool can_apply_influence(char_id_t to_influence, char_id_t influence_from, unsigned char influence_type, IN(g_lock) l);
// double evaluate_potential_political_action(char_id_t by, IN(political_action) action, IN(g_lock) l);


// return whether the influencer was for or against
// int get_influence(char_id_t influencer, IN(std::vector<char_id_t>) influence_targets, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, bool is_proposal, INOUT(w_lock) l);
bool generate_influence(INOUT(influence_against) out, char_id_t to_influence, char_id_t influencer, IN(std::vector<influence_against>) iv, IN(g_lock) l);

int get_influence_results_from_player(IN_P(political_action) action_for, IN(std::vector<influence_against>) influences_involved, IN(std::vector<char_id_t>) influence_targets, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, bool is_proposal, INOUT(w_lock) l);
bool get_player_take_action(IN_P(political_action) act, double player_eval, IN(std::vector<std::pair<char_id_t, int>>) council_members, IN(std::vector<influence_against>) result_set, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, INOUT(w_lock) l);
bool get_player_vote(IN_P(political_action) act, double player_eval, IN(std::vector<influence_against>) result_set, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(flat_multimap<char_id_t, influence_against>) negative_influence, INOUT(w_lock) l);
bool get_player_proceed_with_proposal(IN_P(political_action) act, bool action_needs_vote, double player_eval, char_id_t proposal_from, IN(std::vector<influence_against>) result_set, INOUT(flat_multimap<char_id_t, influence_against>) positive_influence, INOUT(w_lock) l);
void display_positive_influence(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(influence_against) inf, IN(g_lock) l);
void display_negative_influence(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(influence_against) inf, IN(g_lock) l);
void display_issue(IN(std::shared_ptr<uiElement>) parent, INOUT(int) x, INOUT(int) y, IN(influence_against) inf, IN(g_lock) l);

