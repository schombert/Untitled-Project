#pragma once
#include "globalhelpers.h"
#include "pacts.h"
#include "i18n.h"
#include "traits.h"

class peace_deal;

bool willraisetroops(char_id_t person, char_id_t behalfof, char_id_t target) noexcept;
double mayraisetroops(char_id_t person, char_id_t behalfof) noexcept;
bool will_attend_event(char_id_t person, char_id_t host, unsigned int event_id) noexcept;
bool will_attend_event_date(char_id_t person, char_id_t host, unsigned int event_id, unsigned int date) noexcept;

bool will_accept_peace(char_id_t person, IN(peace_deal) deal, INOUT(r_lock) l) noexcept;
bool will_accept_peace(char_id_t person, IN(peace_deal) deal, INOUT(w_lock) l) noexcept;

bool will_honor_def_pact(char_id_t id, char_id_t behalf, char_id_t against, IN(pact_data) pact, INOUT(r_lock) l) noexcept;
bool will_honor_def_pact(char_id_t id, char_id_t behalf, char_id_t against, IN(pact_data) pact, INOUT(w_lock) l) noexcept;

template<typename LOCK>
bool will_honor_def_pact(char_id_t id, char_id_t behalf, char_id_t against, IN(pact_data) pact, INOUT(LOCK) l) {
	if (id != global::playerid) {
		if (const auto d = get_udata(id, l)) {
			double probadj = adjust_by_feeling( *d, id, behalf, get_feeling(id, behalf, l), 0.3) - adjust_by_feeling(*d, id, against, get_feeling(id, against, l), 0.3);
			if (is_at_war(id.value, l)) probadj -= 0.3;

			if (is_deceitful(*d)) {
				probadj *= 2.0;
			} else if (is_honest(*d)) {
				probadj += 0.4;
			}

			if (honor_loss_on_break(pact, id, l)) {
				probadj += d->p_honorable;
			}

			return global_store.get_double() < probadj;
		}
		return false;
	} else {
		l.unlock();
		const bool result = make_yes_no_popup(global::uicontainer, get_simple_string(TX_DEF_CALL), [behalf, against](const std::shared_ptr<uiScrollView>& sv) {
			size_t params[] = {behalf.value, against.value};
			sv->subelements.push_back(create_tex_block(TX_DEF_CALL_BODY, params, 2, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text));
		}, 0, is_honest(*get_udata(global::playerid, fake_lock())) ? 5 : 0, accept_decline_array);
		l.lock();
		return result;
	}
}