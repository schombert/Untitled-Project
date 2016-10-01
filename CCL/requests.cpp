#include "globalhelpers.h"
#include "requests.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "events.h"
#include "ChPane.h"
#include "ProvPane.h"
#include "TPane.h"
#include "datamanagement.hpp"
#include "i18n.h"
#include "peace.h"
#include "actions.h"
#include "relations.h"
#include "traits.h"
#include "living_data.h"
#include "wardata.h"

template<typename LOCK>
bool _will_accept_peace(char_id_t person, IN(peace_deal) deal, INOUT(LOCK) l) noexcept {
	if (person != global::playerid) {
		return true;
	} else {
		l.unlock();
		const auto result = make_yes_no_popup(global::uicontainer, get_simple_string(TX_L_PEACE_OFFER), [&deal](const std::shared_ptr<uiScrollView>& sv) {
			size_t param = head_of_state(admin_id_t(deal.offer_from), r_lock()).value;
			{
				r_lock l;
				int y = create_tex_block(TX_PEACE_OFFER_BODY, &param, 1, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text) + 10;
				deal.to_ui(sv, 0, y, l);
			}
		}, 0, 0, accept_decline_array);
		l.lock();
		return result;
	}
}

bool will_accept_peace(char_id_t person, IN(peace_deal) deal, INOUT(r_lock) l) noexcept {
	return _will_accept_peace(person, deal, l);
}

bool will_accept_peace(char_id_t person, IN(peace_deal) deal, INOUT(w_lock) l) noexcept {
	return _will_accept_peace(person, deal, l);
}

template<typename LOCK>
bool _will_honor_def_pact(char_id_t id, char_id_t behalf, char_id_t against, IN(pact_data) pact, INOUT(LOCK) l) noexcept {
	if (id != global::playerid) {
		return with_udata(id, l, false, [&l, &pact, id, behalf, against](IN(udata) d) noexcept {
			double probadj = adjust_by_feeling(d, id, behalf, get_feeling(id, behalf, l), 0.3) - adjust_by_feeling(d, id, against, get_feeling(id, against, l), 0.3);
			if (is_at_war(id, l)) probadj -= 0.3;

			if (is_deceitful(d)) {
				probadj *= 2.0;
			} else if (is_honest(d)) {
				probadj += 0.4;
			}

			if (honor_loss_on_break(pact, id, l)) {
				probadj += d.p_honorable;
			}

			return global_store.get_double() < probadj;
		});
	} else {
		l.unlock();
		const bool result = make_yes_no_popup(global::uicontainer, get_simple_string(TX_DEF_CALL), [behalf, against](const std::shared_ptr<uiScrollView>& sv) noexcept {
			size_t params[] = {behalf.value, against.value};
			create_tex_block(TX_DEF_CALL_BODY, params, 2, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
		}, 0, with_udata(char_id_t(global::playerid), fake_lock(), 0, [](IN(udata) d) noexcept { return is_honest(d) ? 5 : 0; }), accept_decline_array);
		l.lock();
		return result;
	}
}

bool will_honor_def_pact(char_id_t id, char_id_t behalf, char_id_t against, IN(pact_data) pact, INOUT(r_lock) l) noexcept {
	return _will_honor_def_pact(id, behalf, against, pact, l);
}

bool will_honor_def_pact(char_id_t id, char_id_t behalf, char_id_t against, IN(pact_data) pact, INOUT(w_lock) l) noexcept {
	return _will_honor_def_pact(id, behalf, against, pact, l);
}

bool willraisetroops(char_id_t person, char_id_t behalfof, char_id_t target) noexcept {
	if (person != global::playerid) {
		return true;
	} else {
		const bool rvalue = make_yes_no_popup(global::uicontainer, get_simple_string(TX_L_CTOARMS), [ behalfof, target](const std::shared_ptr<uiScrollView>& sv) {
			if (valid_ids(target)) {
				size_t params[2] = {behalfof.value, target.value};
				create_tex_block(TX_CALL_TO_A1, params,2,sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
			} else {
				size_t param = behalfof.value;
				create_tex_block(TX_CALL_TO_A2, &param, 1, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
			}
		}, 0.0);
		if (!rvalue) {
			global::actionlist.add_new<relation_change>(behalfof, person, static_cast<char>(-3), false);
		}
		return rvalue;
	}
}

bool will_attend_event(char_id_t person, char_id_t host, unsigned int event_id) noexcept {
	if (person != global::playerid) {
		return true;
	} else {
		const bool rvalue = make_yes_no_popup(global::uicontainer, get_simple_string(TX_L_INVITATION),  [host, event_id](const std::shared_ptr<uiScrollView>& sv) {
			size_t params[2] = {host.value, event_template_by_id(event_id).name};
			create_tex_block(TX_INVITATION, params, 2, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
		}, 0.0);
		if (!rvalue) {
			global::actionlist.add_new<relation_change>(host, person, static_cast<char>(-1), false);
		}
		return rvalue;
	}
}

bool will_attend_event_date(char_id_t person, char_id_t host, unsigned int event_id, unsigned int date) noexcept {
	if (person != global::playerid) {
		return true;
	} else {
		const bool rvalue = make_yes_no_popup(global::uicontainer, get_simple_string(TX_L_INVITATION), [host, event_id, date](const std::shared_ptr<uiScrollView>& sv) {
			size_t params[] = {host.value, event_template_by_id(event_id).name, date};
			create_tex_block(TX_INVITATION_DATE, params, 3, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
		}, 0.0);
		if (!rvalue) {
			global::actionlist.add_new<relation_change>(host, person, static_cast<unsigned char>(-1), false);
		}
		return rvalue;
	}
}

double mayraisetroops(char_id_t person, char_id_t behalfof) noexcept {
	return 1.0;
}