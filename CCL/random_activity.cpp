#include "globalhelpers.h"
#include "random_activity.h"
#include "datamanagement.hpp"
#include "structs.hpp"
#include "wardata.h"
#include "actions.h"
#include "relations.h"
#include "traits.h"
#include "events.h"
#include "finances.h"
#include "pacts.h"
#include "envoys.h"
#include "living_data.h"
#include "spies.h"
#include "action_opinion.h"

#include <boost/mpl/list.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>


namespace detail {
	template<typename T>
	static auto test_instance(int) -> sfinae_true<decltype(&T::generate_instance)>;
	template<typename T>
	static auto test_instance(long) -> std::false_type;
}

template<typename T>
struct has_generate_instance : decltype(detail::test_instance<T>(0)){};

constexpr double days_to_zero_r = 60.0;
constexpr double min_r = -OPINION_THRESHOLD;
constexpr double max_r = 1.5;

constexpr double offset = (-min_r) * days_to_zero_r / max_r;
constexpr double factor = (-min_r) * days_to_zero_r + (-min_r) * offset;

constexpr double reluctance_adjustment(unsigned int days_since_activity) noexcept {
	return min_r + (factor / (static_cast<double>(days_since_activity) + offset));
}


template<typename this_type>
class adm_activity {
public:
	static bool _do_admin_activity(IN(administration) adm, std::true_type) noexcept {
		auto actor = this_type::get_actor(adm);

		if (actor == global::playerid)
			return false;

		auto inst = this_type::generate_instance(adm, actor);

		if (valid_ids(actor)) {
			double des = this_type::eval_desireability(adm, actor, inst);
			double reluctance = reluctance_adjustment(global::currentday - with_udata_force(actor, fake_lock(), [](IN(udata) d) noexcept { return d.activity; }));

			if ((des - reluctance) * 2.0 > global_store.get_fast_double()) {
				return this_type::do_activity(adm, actor, inst);
			}
		} else { //must be voted on
			actor = this_type::get_voter(adm);
			if (actor == global::playerid)
				return false;

			double des = this_type::eval_desireability(adm, actor, inst);
			double reluctance = reluctance_adjustment(global::currentday - with_udata_force(actor, fake_lock(), [](IN(udata) d) noexcept { return d.activity; }));

			if (des - reluctance > global_store.get_fast_double()) {
				return this_type::vote_on_activity(adm, actor, inst);
			}
		}

		return false;
	}
	static bool _do_admin_activity(IN(administration) adm, std::false_type) noexcept {
		auto actor = this_type::get_actor(adm);

		if (actor == global::playerid) {
			return false;
		} else if (valid_ids(actor)) {
			double des = this_type::eval_desireability(adm, actor);
			double reluctance = reluctance_adjustment(global::currentday - with_udata_force(actor, fake_lock(), [](IN(udata) d) noexcept { return d.activity; }));

			if (des - reluctance > global_store.get_fast_double()) {
				return this_type::do_activity(adm, actor);
			}
		} else { //must be voted on
			actor = this_type::get_voter(adm);
			if (actor == global::playerid)
				return false;

			double des = this_type::eval_desireability(adm, actor);
			double reluctance = reluctance_adjustment(global::currentday - with_udata_force(actor, fake_lock(), [](IN(udata) d) noexcept { return d.activity; }));

			if ((des - reluctance) * 2.0 > global_store.get_fast_double()) {
				return this_type::vote_on_activity(adm, actor);
			}
		}
		return false;
	}


	static bool do_admin_activity(IN(administration) adm) noexcept { 
		return _do_admin_activity(adm, has_generate_instance<this_type>());
	};
};

#define ADM_ACTIVITY_CLASS(x) class x : public adm_activity < x >

ADM_ACTIVITY_CLASS(adm_a_conquest_war) {
public:
	static int get_class_status(IN(administration) adm) noexcept {
		if (wars_involved_in.count(admin_id_t(admin_pool.get_index(adm, fake_lock())), fake_lock()) == 0)
			return 1;
		return 0;
	};
	static char_id_t get_actor(IN(administration) adm) noexcept {
		return get_diplo_decider(adm, fake_lock()); 
	};
	static char_id_t get_voter(IN(administration) adm) noexcept {
		return char_id_t();
	};
	static char_id_t generate_instance(IN(administration) adm, char_id_t actor) noexcept {
		return get_random_target(head_of_state(adm), fake_lock());
	};
	static double eval_desireability(IN(administration) adm, char_id_t actor, char_id_t target) noexcept {
		return attack_opinion(actor, head_of_state(adm), target, make_wg_conquest(), fake_lock());
	};
	static bool do_activity(IN(administration) adm, char_id_t actor, char_id_t target) noexcept {
		if (!valid_ids(target))
			return false;
		const auto hos = head_of_state(adm);
		if (with_udata(hos, fake_lock(), true, [tc = static_cast<double>(troop_cost_est(mu_estimate(hos, fake_lock())))](IN(udata) d) noexcept { return d.wealth / 12.0 < tc; }))
			return false;
		global::actionlist.add_new<nocbwar>(head_of_state(adm), target);
		return true;
	};
	static bool vote_on_activity(IN(administration) adm, char_id_t actor, char_id_t target) noexcept {
		return false;
	};
};

ADM_ACTIVITY_CLASS(adm_a_defensive_pact) {
public:
	static int get_class_status(IN(administration) adm) noexcept {
		return 1;
	};
	static char_id_t get_actor(IN(administration) adm) noexcept {
		return get_diplo_decider(adm, fake_lock());
	};
	static char_id_t get_voter(IN(administration) adm) noexcept {
		return char_id_t();
	};
	
	static double eval_desireability(IN(administration) adm, char_id_t actor) noexcept {
		return defensive_envoy_opinion(actor, head_of_state(adm), fake_lock());
	};
	static bool do_activity(IN(administration) adm, char_id_t actor) noexcept {
		fake_lock l;
		if (can_add_defensive_mission_a(get_id(adm, l), l)) {
			add_defensive_pact_mission_a(get_id(adm, l), actor, l);
			return true;
		}
		return false;
	};
	static bool vote_on_activity(IN(administration) adm, char_id_t actor) noexcept {
		return false;
	};
};

ADM_ACTIVITY_CLASS(adm_a_defensive_against_pact) {
public:
	static int get_class_status(IN(administration) adm) noexcept {
		return 1;
	};
	static char_id_t get_actor(IN(administration) adm) noexcept {
		return get_diplo_decider(adm, fake_lock());
	};
	static char_id_t get_voter(IN(administration) adm) noexcept {
		return char_id_t();
	};
	static char_id_t generate_instance(IN(administration) adm, char_id_t actor) noexcept {
		return get_random_threat(char_id_t(head_of_state(adm)), fake_lock());
	};
	static double eval_desireability(IN(administration) adm, char_id_t actor, char_id_t against) noexcept {
		return defensive_against_envoy_opinion(char_id_t(actor), char_id_t(head_of_state(adm)), against, fake_lock());
	};
	static bool do_activity(IN(administration) adm, char_id_t actor, char_id_t against) noexcept {
		fake_lock l;
		if (valid_ids(against) && can_add_defensive_against_mission_a(admin_id_t(admin_pool.get_index(adm, l)), against, l)) {
			add_defensive_against_mission_a(admin_id_t(admin_pool.get_index(adm, l)), actor, against, l);
			return true;
		}
		return false;
	};
	static bool vote_on_activity(IN(administration) adm, char_id_t actor, char_id_t against) noexcept {
		return false;
	};
};

ADM_ACTIVITY_CLASS(adm_a_non_aggression_pact) {
public:
	static int get_class_status(IN(administration) adm) noexcept {
		return 1;
	};
	static char_id_t get_actor(IN(administration) adm) noexcept {
		return get_diplo_decider(adm, fake_lock());
	};
	static char_id_t get_voter(IN(administration) adm) noexcept {
		return char_id_t();
	};
	static char_id_t generate_instance(IN(administration) adm, char_id_t actor) noexcept {
		return get_random_threat(head_of_state(adm), fake_lock());
	};
	static double eval_desireability(IN(administration) adm, char_id_t actor, char_id_t with) noexcept {
		return non_aggression_envoy_opinion(actor, head_of_state(adm), with, fake_lock());
	};
	static bool do_activity(IN(administration) adm, char_id_t actor, char_id_t with) noexcept {
		fake_lock l;
		if (valid_ids(with) && can_add_non_aggression_mission_a(admin_id_t(admin_pool.get_index(adm, l)), with, l)) {
			add_nonagression_mission_a(admin_id_t(admin_pool.get_index(adm, l)), actor, with, l);
			return true;
		}
		return false;
	};
	static bool vote_on_activity(IN(administration) adm, char_id_t actor, char_id_t with) noexcept {
		return false;
	};
};

ADM_ACTIVITY_CLASS(adm_a_cancel_pact) {
public:
	static int get_class_status(IN(administration) adm) noexcept {
		return (global_store.get_fast_int() % 4) == 0 ? 1 : 0;
	};
	static char_id_t get_actor(IN(administration) adm) noexcept {
		return get_diplo_decider(adm, fake_lock());
	};
	static char_id_t get_voter(IN(administration) adm) noexcept {
		return char_id_t();
	};
	static pact_id_t generate_instance(IN(administration) adm, char_id_t actor) noexcept {
		small_vector<pact_id_t, 8, concurrent_allocator<pact_id_t>> pctlst;
		enum_pacts_for(head_of_state(adm), fake_lock(), [&pctlst](pact_id_t p) { pctlst.push_back(p); });
		if (pctlst.size() != 0)
			return pctlst[global_store.get_fast_int() % pctlst.size()];
		return pact_id_t();
	};
	static double eval_desireability(IN(administration) adm, char_id_t actor, pact_id_t p) noexcept {
		if(valid_ids(p))
			return break_pact_opinion(actor, head_of_state(adm), p, fake_lock());
		return 0.0;
	};
	static bool do_activity(IN(administration) adm, char_id_t actor, pact_id_t p) noexcept {
		if (valid_ids(p)) {
			global::actionlist.add_new<break_pact_a>(actor, p);
			return true;
		}
		return false;
	};
	static bool vote_on_activity(IN(administration) adm, char_id_t actor, pact_id_t p) noexcept {
		return false;
	};
};

ADM_ACTIVITY_CLASS(adm_a_hostile_plot) {
public:
	struct instance {
		char_id_t against;
		unsigned char type;
		instance(char_id_t c, unsigned char t) noexcept : against(c), type(t) {}
	};

	static int get_class_status(IN(administration) adm) noexcept {
		return 1;
	};
	static char_id_t get_actor(IN(administration) adm) noexcept {
		return get_spy_decider(adm, fake_lock());
	};
	static char_id_t get_voter(IN(administration) adm) noexcept {
		return char_id_t();
	};
	static instance generate_instance(IN(administration) adm, char_id_t actor) noexcept {
		return instance(get_random_threat(head_of_state(adm), fake_lock()), (global_store.get_fast_int() % 2 == 0) ? PLOT_AGGRESSION : PLOT_TYRANNY);
	};
	static double eval_desireability(IN(administration) adm, char_id_t actor, IN(instance) i) noexcept {
		return deception_plot_opinion(actor, head_of_state(adm), i.against, fake_lock());
	};
	static bool do_activity(IN(administration) adm, char_id_t actor, IN(instance) i) noexcept {
		if (!valid_ids(i.against))
			return false;
		fake_lock l;
		const auto ad_id = admin_id_t(admin_pool.get_index(adm, l));
		const auto hos = head_of_state(adm);
		if ((global::project_income(hos, l) - current_mthly_expense(hos, l)) <= plot_by_type(i.type).monthly_cost ||
			!new_spy_mission_possible(ad_id, i.against, i.type, l))
			return false;
		global::actionlist.add_new<setup_spy_mission>(actor, ad_id, i.against, char_id_t(), i.type);
		return true;
	};
	static bool vote_on_activity(IN(administration) adm, char_id_t actor, IN(instance) i) noexcept {
		return false;
	};
};

ADM_ACTIVITY_CLASS(adm_a_enemy_plot) {
public:
	static int get_class_status(IN(administration) adm) noexcept {
		return 1;
	}
	static char_id_t get_actor(IN(administration) adm) noexcept {
		return get_spy_decider(adm, fake_lock());
	};
	static char_id_t get_voter(IN(administration) adm) noexcept {
		return char_id_t();
	};
	static char_id_t generate_instance(IN(administration) adm, char_id_t actor) noexcept {
		return get_random_hated_or_enemy(actor, fake_lock());
	}
	static double eval_desireability(IN(administration) adm, char_id_t actor, char_id_t target) noexcept {
		return deception_plot_opinion(actor, head_of_state(adm), target, fake_lock());
	}
	static bool do_activity(IN(administration) adm, char_id_t actor, char_id_t target) noexcept {
		if (!valid_ids(target))
			return false;
		fake_lock l;
		const auto ad_id = admin_id_t(admin_pool.get_index(adm, l));
		const auto hos = head_of_state(adm);
		if ((global::project_income(hos, l) - current_mthly_expense(hos, l)) <= plot_by_type(PLOT_DISHONORERABLE).monthly_cost ||
			!new_spy_mission_possible(ad_id, target, PLOT_DISHONORERABLE, l))
			return false;
		global::actionlist.add_new<setup_spy_mission>(actor, ad_id, target, char_id_t(), static_cast<unsigned char>(PLOT_DISHONORERABLE));
		return true;
	}
	static bool vote_on_activity(IN(administration) adm, char_id_t actor, char_id_t target) noexcept {
		return false;
	};
};

ADM_ACTIVITY_CLASS(adm_a_host_mass_event) {
public:
	static int get_class_status(IN(administration) adm) noexcept {
		if (wars_involved_in.count(admin_id_t(admin_pool.get_index(adm, fake_lock())), fake_lock()) == 0)
			return 1;
		return 0;
	};
	static char_id_t get_actor(IN(administration) adm) noexcept {
		return get_local_decider(adm, fake_lock());
	};
	static char_id_t get_voter(IN(administration) adm) noexcept {
		return char_id_t();
	};
	static unsigned int generate_instance(IN(administration) adm, char_id_t actor) noexcept {
		static const unsigned int types[3] = {EVENT_FEAST, EVENT_THEATER, EVENT_TOURNAMENT};
		return types[global_store.get_fast_int() % 3];
	};
	static double eval_desireability(IN(administration) adm, char_id_t actor, unsigned int type) noexcept {
		return host_mass_event_opinion(actor, head_of_state(adm), type, fake_lock());
	};
	static bool do_activity(IN(administration) adm, char_id_t actor, unsigned int type) noexcept {
		const auto hos = head_of_state(adm);
		return with_udata(hos, fake_lock(), false, [hos, type](IN(udata) d) noexcept {
			if ((d.flags & P_FLAG_PERPARING_EVENT) == 0 && d.wealth > 0.0) {
				global::actionlist.add_new<planned_event>(hos, type);
				return true;
			}
			return false;
		});
	};
	static bool vote_on_activity(IN(administration) adm, char_id_t actor, unsigned int type) noexcept {
		return false;
	};
};

typedef boost::mpl::vector<adm_a_conquest_war, adm_a_defensive_pact, adm_a_defensive_against_pact, adm_a_non_aggression_pact, adm_a_cancel_pact,
	adm_a_hostile_plot, adm_a_enemy_plot, adm_a_host_mass_event>
	admin_action_classes;
constexpr int num_adm_classes = boost::mpl::size<admin_action_classes>::value;

using admin_possibility_vector = small_vector<unsigned short, num_adm_classes, concurrent_allocator<unsigned short>>;



template <long long cnt>
void admin_class_distributions_it(IN(administration) adm, INOUT(admin_possibility_vector) possible) noexcept {
	auto res = boost::mpl::at<admin_action_classes, boost::mpl::int_<cnt>>::type::get_class_status(adm);
	for (; res != 0; --res) {
		possible.push_back(cnt);
	} 
	admin_class_distributions_it<cnt - 1>(adm, possible);
}

template <>
void admin_class_distributions_it<-1>(IN(administration) adm, INOUT(admin_possibility_vector) possible) noexcept {
} 

class adm_dispatch_it_class {
public:
	typedef bool(*eltype)(IN(administration));

	template <int cnt>
	static eltype nth_element() {
		return &(boost::mpl::at<admin_action_classes, boost::mpl::int_<cnt>>::type::do_admin_activity);
	}
};

static const auto adm_dispatch_table = generate_array<adm_dispatch_it_class, num_adm_classes>();

void do_admin_step(IN(administration) adm) noexcept {
	admin_possibility_vector possible;
	admin_class_distributions_it<num_adm_classes - 1>(adm, possible);
	
	for (int ii = 0; ii != 2; ++ii) { // try up to 2 options, selecting first from the priority list
		auto pz = possible.size();
		if (pz != 0) {
			const auto selection = possible[global_store.get_fast_int() % pz];
			if (adm_dispatch_table[selection](adm))
				return;
			for (--pz; pz != SIZE_MAX; --pz) {
				if (possible[pz] == selection) {
					possible[pz] = std::move(possible.back());
					possible.pop_back();
				}
			}
		}
	}

}