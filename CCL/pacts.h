#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"



#define G_HONOR 1
#define G_MARRIAGE 2
#define G_TRIBUTE 3

struct guarantee {
	unsigned char type;
	union {
		struct {
			char_id a;
			char_id b;
		} marriage;
		struct {
			char_id to;
			float amount;
		} tribute;
	} data;
};

#define P_DEFENCE 1
#define P_DEFENCE_AGAINST 2
#define P_NON_AGRESSION 3


class pact_data;

namespace global {
	extern v_pool_t<pact_data, pact_id> pacts;
};

class pact_data {
public:

	static constexpr pact_id NO_PACT = pact_id_t::NONE;

	static constexpr unsigned char NONE = 0;
	static constexpr unsigned char DEFENSE = 1;
	static constexpr unsigned char DEFENSE_AGAINST = 2;
	static constexpr unsigned char NON_AGRESSION = 3;

	std::vector<guarantee> guarantees;
	
	char_id_t a;
	char_id_t b;
	char_id_t against;
	// pact_id_t pact_id;

	unsigned char pact_type = NONE;

	void construct() noexcept {}
	bool is_clear() const noexcept {
		return pact_type == pact_data::NONE;
	}
	void set_clear() noexcept {
		guarantees.clear();
		pact_type = pact_data::NONE;
		against = char_id_t();
	}
};

inline const pact_data& get_object(pact_id_t id, IN(g_lock) l) noexcept {
	return global::pacts.get(id.value, l);
}
inline pact_data& get_object(pact_id_t id, IN(w_lock) l) noexcept {
	return global::pacts.get(id.value, l);
}
inline pact_id_t get_id(IN(pact_data) obj, IN(g_lock) l) noexcept {
	return pact_id_t(global::pacts.get_index(obj, l));
}

int count_defensive_pacts(char_id_t id,  IN(g_lock) l) noexcept;
void add_pact( IN(pact_data) pact, IN(w_lock) l) noexcept;
void pact_to_ui(int x, int &y, IN(std::shared_ptr<uiElement>) parent, IN(pact_data) pact, IN(g_lock) l) noexcept;
bool extend_pact_popup(IN(pact_data) pact) noexcept;
void pact_expired_popup(IN(pact_data) pact, IN(g_lock) l) noexcept;
void pact_entered_popup(IN(pact_data) pact, IN(g_lock) l) noexcept;
void pact_declined_popup(IN(pact_data) pact, IN(g_lock) l) noexcept;
void update_pacts( INOUT(w_lock) l) noexcept;
bool has_non_agression_pact(char_id_t a, char_id_t b, IN(g_lock) l) noexcept;
bool has_defensive_pact(char_id_t a, char_id_t b, IN(g_lock) l) noexcept;
bool has_defensive_pact_against(char_id_t a, char_id_t b, char_id_t target, IN(g_lock) l) noexcept;
void vaidate_guarantees(INOUT(pact_data) pact, IN(g_lock) l) noexcept;
void free_pact(INOUT(pact_data) pact, IN(w_lock) l) noexcept;

void open_pick_guarantee(int x, int y,  IN(std::function<void(const guarantee &)>) handle_result) noexcept;
void setup_guarantee(unsigned int type, IN(pact_data) pact, IN(g_lock) l) noexcept;
void close_pick_guarantee() noexcept;
void init_pacts_ui() noexcept;

void close_offer_window() noexcept;
void open_offer_window(char_id_t from, IN(pact_data) pact,  IN(std::function<void(bool, const pact_data&)>) result) noexcept;

void break_pact(INOUT(pact_data) pact, char_id_t breaker, IN(w_lock) l) noexcept;
bool honor_loss_on_break(IN(pact_data) pact, char_id_t breaker, IN(g_lock) l) noexcept;
double honor_loss_on_break_val(IN(pact_data) pact, char_id_t breaker, IN(g_lock) l) noexcept;

float tribute_adjuistment(IN(pact_data) pact, char_id_t from, IN(g_lock) l) noexcept;

float desirability_of_defensive(char_id_t from, char_id_t other, char_id_t envoy, bool recipient, IN(g_lock) l) noexcept;
float desirability_of_t_defensive(char_id_t from, char_id_t other, char_id_t against, char_id_t envoy, bool recipient, IN(g_lock) l) noexcept;
float desirability_of_non_aggression(char_id_t from, char_id_t other, char_id_t envoy, bool recipient, IN(g_lock) l) noexcept;

float continuing_desirability_of_defensive(IN(pact_data) pact, char_id_t from, char_id_t other, IN(g_lock) l) noexcept;
float continuing_desirability_of_t_defensive(IN(pact_data) pact, char_id_t from, char_id_t other, IN(g_lock) l) noexcept;
float continuing_desirability_of_non_aggression(IN(pact_data) pact, char_id_t from, char_id_t other, IN(g_lock) l) noexcept;
float continuing_desirability_of_pact(pact_id_t pact, char_id_t from, IN(g_lock) l) noexcept;

template<typename T>
void enum_defensive_against(char_id_t target, char_id_t against, IN(g_lock) l, IN(T) func) noexcept {
	enum_pacts_for(target, l, [&func, &l, against, target](pact_id_t id) {
		const pact_data& pact = global::pacts.get(id.value, l);
		if (pact.pact_type == P_DEFENCE || (pact.pact_type == P_DEFENCE_AGAINST && pact.against == against)) {
			if (pact.a == target)
				func(pact.b, pact_id_t(id));
			else
				func(pact.a, pact_id_t(id));
		}
	});
}