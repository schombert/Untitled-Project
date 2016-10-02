#include "globalhelpers.h"
#include "envoys.h"
#include "structs.hpp"
#include "actions.h"
#include "ChPane.h"
#include "i18n.h"
#include "pacts.h"
#include "living_data.h"
#include "relations.h"
#include "datamanagement.hpp"
#include "events.h"
#include "relations.h"
#include "court.h"
#include "generated_ui.h"
#include "finances.h"
#include "action_opinion.h"
#include "laws.h"
#include "schedule.h"

emission_selection mission_selection;
admin_id_t emissions_for_a;

void display_individual_mission(IN(std::shared_ptr<uiElement>) p, INOUT(int) x, INOUT(int) y, IN(std::pair<admin_id_t, concurrent_uniq<envoy_mission>>) obj, admin_id_t source, IN(g_lock) l) noexcept {
	obj.second->display_mission_a(x, y, p, source, l);
}

/*struct {
	std::shared_ptr<uiDragRect> window;
	std::shared_ptr<uiScrollView> contents;
	std::shared_ptr<uiHLink> name_link;
	std::shared_ptr<ui_button_disable> new_mission;
} missions_display;*/

#define M_DISPLAY_WIDTH 400
#define M_DISPLAY_HEIGHT 700

#define M_SELECTION_WIDTH 400
#define M_SELECTION_HEIGHT 400

size_t count_missions(admin_id_t a, IN(g_lock) l) noexcept {
	return envoy_missions.count(a, l);
}

unsigned char first_free_e_index(admin_id_t a, IN(g_lock) l) noexcept {
	bool taken[8] = {false, false, false, false, false, false, false, false};
	envoy_missions.for_each(a, l, [&taken](IN(concurrent_uniq<envoy_mission>) m) {
		taken[m->index] = true;
	});
	for (unsigned char i = 0; i != 8; ++i) {
		if (!taken[i])
			return i;
	}
	return 8;
}

envoy_mission* get_envoy_mission(admin_id_t a, unsigned char index, IN(g_lock) l) noexcept {
	envoy_mission* result = nullptr;
	envoy_missions.for_each_breakable(a, l, [index, &result](IN(concurrent_uniq<envoy_mission>) m) {
		if (m->index == index) {
			result = m.get();
			return true;
		}
		return false;
	});
	return result;
}

void remove_envoy_mision(admin_id_t a, unsigned char index, IN(w_lock) l) noexcept {
	envoy_missions.range_erase_if(a, l, [a, &l, index](IN(std::pair<admin_id_t, concurrent_uniq<envoy_mission>>) pr) {
		if (pr.second->index == index) {
			pr.second->expire_mission(a, l);
			return true;
		}
		return false;
	});
}

class pact_mission : public envoy_mission {
public:
	char_id_t target;
	pact_mission(unsigned char t) noexcept : envoy_mission(t) {};
	cvector<pact_data> offers;
	cvector<std::pair<char_id_t, unsigned int>> offers_to_make;

	virtual void display_mission_a(int x, INOUT(int) y, IN(std::shared_ptr<uiElement>) parent, admin_id_t source, IN(g_lock) l) noexcept;
	virtual bool accept_offer(admin_id_t source, char_id_t actor, size_t indx, INOUT(w_lock) l) noexcept {
		if (!pact_reaction_a(source, actor, offers[indx], l))
			return false;

		add_pact(offers[indx], l);
		global::flag_ch_update(offers[indx].a.value);
		global::flag_ch_update(offers[indx].b.value);
		const auto other = (offers[indx].a == head_of_state(source, l)) ? offers[indx].b : offers[indx].a;
		if (global::interested.in_set(offers[indx].b.value) || global::interested.in_set(offers[indx].a.value)) {
			pact_entered_popup(offers[indx], l);
		}
		offers[indx].pact_type = 0;
		for (size_t i = offers.size() - 1; i != SIZE_MAX; --i) {
			if (offers[i].pact_type != 0) {
				vaidate_guarantees(offers[i], l);
				if (offers[i].guarantees.size() == 0)
					offers[i].pact_type = 0;
			}
		}
		if(actor == global::playerid)
			emission_display_rec.needs_update = true;
		return true;
	};
	virtual void decline_offer(admin_id_t source, char_id_t actor, size_t indx, IN(g_lock) l) noexcept;
	virtual void expire_mission(admin_id_t source, IN(g_lock) l) noexcept {
		for (size_t i = offers.size() - 1; i != SIZE_MAX; --i) {
			const auto other = (offers[i].a == head_of_state(source, l)) ? offers[i].b : offers[i].a;
			if (other == global::playerid && offers[i].pact_type != 0) {
				pact_declined_popup(offers[i], l);
			}
		}
		offers.clear();
		offers.shrink_to_fit();
		envoy_mission::expire_mission(source, l);
	}
	virtual void make_offer(admin_id_t source, char_id_t person, char_id_t target, INOUT(w_lock) l) noexcept;
	virtual void get_target_list(admin_id_t source, IN(g_lock) l) noexcept;
	virtual unsigned int days_until_offers(char_id_t person, IN(g_lock) l) noexcept;
	virtual bool offer_available(size_t indx) noexcept {
		return offers[indx].pact_type != 0;
	}
	virtual size_t total_offers() noexcept {
		return offers.size();
	}
	virtual bool identical(IN(envoy_mission) m) const noexcept;
	virtual void start_mission(char_id_t actor, admin_id_t source, unsigned int on_day, IN(w_lock) l) noexcept;
	virtual void setup_next_offer(char_id_t actor, admin_id_t source, unsigned int on_day, IN(w_lock) l) noexcept;
};

void envoy_mission::display_mission_a(int x, INOUT(int) y, IN(std::shared_ptr<uiElement>) parent, admin_id_t source, IN(g_lock) l) noexcept {
	size_t param = envoy.value;
	get_linear_ui(TX_EV_LABEL, &param, 1, parent, 5, 0, global::empty, global::standard_text);
	y += global::standard_text.csize + 2;
	if (global::currentday < return_date) {
		parent->add_element<uiCountDown>(x, y, parent->pos.width - x - 15, 22, start_date, return_date, global::bar_fill, get_simple_string(TX_ENVOY_RET), global::standard_text, [](uiCountDown* c) {
			global::uiTasks.run([] {
				global::end_of_day.wait();
				emission_display_rec.needs_update = true;
			});
		});
	} else {
		parent->add_element<uiCountDown>(x, y, parent->pos.width - x - 15, 22, return_date, expiration_date, global::bar_fill, get_simple_string(TX_OFFER_EXPIRE), global::standard_text, [](uiCountDown* c) {
			global::uiTasks.run([] {
				global::end_of_day.wait();
				emission_display_rec.needs_update = true;
			});
		});
	}
	parent->add_element<uiButton>(x + 10, y+26, 100, 20, get_simple_string(TX_CANCEL_MISSION), global::solid_border, global::standard_text, [&l, source, i = this->index](uiButton* b) {
		r_lock il;
		if (auto tm = get_envoy_mission(source, i, il)) {
			tm->active = false;
			global::actionlist.add_new<cancel_envoy_mission>(char_id_t(global::playerid), source, i);
			emission_display_rec.needs_update = true;
		}
	});

	y += 48;
}

void pact_mission::display_mission_a(int x, INOUT(int) y, IN(std::shared_ptr<uiElement>) parent, admin_id_t source, IN(g_lock) l) noexcept {
	envoy_mission::display_mission_a(x, y, parent, source, l);
	
	const size_t sz = offers.size();
	for (size_t indx =  0; indx != sz; ++indx) {
		if (offers[indx].pact_type != 0) {
			pact_to_ui(x + 10, y, parent, offers[indx], l);
			y += 4;

			parent->add_element<uiButton>(x + 10, y, 100, 20, get_simple_string(TX_ACCEPT_OFFER), global::solid_border, global::standard_text, [indx, source, i = this->index](uiButton* b) {
				global::actionlist.add_new<accept_envoy_offer>(char_id_t(global::playerid), source, i, static_cast<unsigned int>(indx));
			});
			parent->add_element<uiButton>(x + 10 + 100 + 5, y, 100, 20, get_simple_string(TX_DECLINE_OFFER), global::solid_border, global::standard_text, [self = this, indx, source](uiButton* b) {
				self->decline_offer(source, char_id_t(global::playerid), indx, r_lock());
				emission_display_rec.needs_update = true;
			});
			y += 26;
		}
		
	}
}

void pact_mission::decline_offer(admin_id_t source, char_id_t actor, size_t indx, IN(g_lock) l) noexcept {
	const auto other = (offers[indx].a == head_of_state(source, l)) ? offers[indx].b : offers[indx].a;
	if (other == global::playerid) {
		pact_declined_popup(offers[indx], l);
	}
	offers[indx].pact_type = 0;
}


void generate_honor_gar(char_id_t a, char_id_t b, INOUT(pact_data) pact) noexcept {
	pact.guarantees.emplace_back();
	pact.guarantees.back().type = G_HONOR;
};

bool generate_marriage_gar(char_id_t a, char_id_t b, INOUT(pact_data) pact, IN(w_lock) l) noexcept {
	std::vector<char_id_t> lsta;
	std::vector<char_id_t> lstb;

	enum_court_family(get_prime_admin(a, l), a, lsta, l);
	enum_court_family(get_prime_admin(a, l), b, lstb, l);

	vector_erase_if(lsta, [](char_id_t id) {return !global::is_marriable(id); });
	vector_erase_if(lstb, [](char_id_t id) {return !global::is_marriable(id); });

	if (lsta.size() == 0 || lstb.size() == 0)
		return false;

	static const auto born_sort = [](char_id_t a, char_id_t b) { return get_object(a).born < get_object(b).born; };
	sort(lsta.begin(), lsta.end(), std::cref(born_sort));
	sort(lstb.begin(), lstb.end(), std::cref(born_sort));

	for (size_t i = lsta.size() - 1; i != SIZE_MAX; --i) {
		for (size_t j = lstb.size() - 1; j != SIZE_MAX; --j) {
			if (global::are_marriable(lsta[i], lstb[j])) {
				pact.guarantees.emplace_back();
				pact.guarantees.back().type = G_MARRIAGE;
				pact.guarantees.back().data.marriage.a = lsta[i].value;
				pact.guarantees.back().data.marriage.b = lstb[j].value;
				return true;
			}
		}
	}
	return false;
}

void generate_basic_gar(INOUT(pact_data) pact, char_id_t from, char_id_t recipient, IN(w_lock) l) noexcept {
	generate_marriage_gar(from, recipient, pact, l);
}

bool tribute_balance(INOUT(pact_data) pact, char_id_t from, char_id_t recipient, double from_desirability, double r_desirability, IN(g_lock) l) noexcept {
	if (from_desirability >= 0 && r_desirability >= 0) {
		return true;
	}
	const double f_base_income = global::project_income(from, l);
	const double r_base_income = global::project_income(recipient, l);
	
	if (from_desirability < 0) {
		const double r_max = r_base_income - current_mthly_expense(recipient, l);
		double req = from_desirability * -0.1 * f_base_income;
		if (req > r_max)
			req = r_max;
		if (r_desirability - (req * 10.0 / r_base_income) >= 0) {
			pact.guarantees.emplace_back();
			pact.guarantees.back().type = G_TRIBUTE;
			pact.guarantees.back().data.tribute.to = from.value;
			pact.guarantees.back().data.tribute.amount = static_cast<float>(req);
			return true;
		} else if (r_desirability >= 0) {
			req = r_desirability * -0.1 * r_base_income;
			pact.guarantees.emplace_back();
			pact.guarantees.back().type = G_TRIBUTE;
			pact.guarantees.back().data.tribute.to = from.value;
			pact.guarantees.back().data.tribute.amount = static_cast<float>(req);
		}
		return false;
	} else if (r_desirability < 0) {
		const double f_max = f_base_income - current_mthly_expense(from, l);
		const double req = r_desirability * -0.1 * r_base_income;
		if (req <= f_max && (from_desirability - (req * 10.0 / f_base_income)) >= 0) {
			pact.guarantees.emplace_back();
			pact.guarantees.back().type = G_TRIBUTE;
			pact.guarantees.back().data.tribute.to = recipient.value;
			pact.guarantees.back().data.tribute.amount = static_cast<float>(req);
			return true;
		}
		return false;
	}
	return false;
}

bool make_defensive_offer(char_id_t from, char_id_t to, INOUT(pact_data) pact, INOUT(pact_mission) m, INOUT(bool) possible, INOUT(w_lock) l) noexcept {
	generate_basic_gar(pact, from, to, l);

	const double from_d = std::max(desirability_of_defensive(from, to, m.envoy, false, l), 0.0f);
	const double to_d = desirability_of_defensive(to, from, m.envoy, true, l);
	
	if (from_d < 0 && to_d < 0)
		return false;
	if (!tribute_balance(pact, from, to, from_d, to_d, l))
		return false;

	if ((!honor_loss_on_break(pact, char_id_t(from), l) && to_d >= 0) || !honor_loss_on_break(pact, char_id_t(to), l))
		return false;

	possible = true;

	//pact made, check for player consent to offer
	if (to == global::playerid) {
		l.unlock();
		const bool res = make_yes_no_popup(global::uicontainer, get_simple_string(TX_ENVOY_RESULT),
			[from, &pact](IN(std::shared_ptr<uiScrollView>) sv) {
				size_t param = from.value;
				int y = create_tex_block(TX_ENVOY_PACT_OFFER, &param, 1, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text) + 5;
				r_lock l;
				pact_to_ui(0, y, sv, pact, l);
			}, to_d < 0 ? static_cast<size_t>(-to_d) : 0, to_d > 0 ? static_cast<size_t>(to_d) : 0);
		l.lock();
		return res;
	}
	return true;
}

bool make_defensive_against_offer(char_id_t from, char_id_t to, INOUT(pact_data) pact, INOUT(pact_mission) m, INOUT(bool) possible, INOUT(w_lock) l) noexcept {
	generate_basic_gar(pact, from, to, l);

	const double from_d = std::max(desirability_of_t_defensive(from, to, pact.against, m.envoy, false, l), 0.0f);
	const double to_d = desirability_of_t_defensive(to, from, pact.against, m.envoy, true, l);

	if (from_d < 0 && to_d < 0)
		return false;
	if (!tribute_balance(pact, from, to, from_d, to_d, l))
		return false;

	if ((!honor_loss_on_break(pact, from, l) && to_d >= 0) || !honor_loss_on_break(pact, to, l))
		return false;

	possible = true;
	//pact made, check for player consent to offer
	if (to == global::playerid) {
		l.unlock();
		const bool res = make_yes_no_popup(global::uicontainer, get_simple_string(TX_ENVOY_RESULT), [from, &pact](IN(std::shared_ptr<uiScrollView>) sv) {
			size_t param = from.value;
			int y = create_tex_block(TX_ENVOY_PACT_OFFER, &param, 1, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text) + 5;
			r_lock l;
			pact_to_ui(0, y, sv, pact, l);
		}, to_d < 0 ? static_cast<size_t>(-to_d) : 0, to_d > 0 ? static_cast<size_t>(to_d) : 0);
		l.lock();
		return res;
	}
	return true;
}

bool make_nonaggression_offer(char_id_t from, char_id_t to, INOUT(pact_data) pact, INOUT(pact_mission) m, INOUT(bool) possible, INOUT(w_lock) l) noexcept {
	generate_basic_gar(pact, from, to, l);

	const double from_d = std::max(desirability_of_non_aggression(from, to, m.envoy, false, l), 0.0f);
	const double to_d = desirability_of_non_aggression(to, from, m.envoy, true, l);

	if (from_d < 0 && to_d < 0)
		return false;
	if (!tribute_balance(pact, from, to, from_d, to_d, l))
		return false;

	if ((!honor_loss_on_break(pact, from, l) && to_d >= 0) || !honor_loss_on_break(pact, to, l))
		return false;

	possible = true;
	//pact made, check for player consent to offer
	if (to == global::playerid) {
		l.unlock();
		const bool res = make_yes_no_popup(global::uicontainer, get_simple_string(TX_ENVOY_RESULT), [from, &pact](IN(std::shared_ptr<uiScrollView>) sv) {
			size_t param = from.value;
			int y = create_tex_block(TX_ENVOY_PACT_OFFER, &param, 1, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text) + 5;
			r_lock l;
			pact_to_ui(0, y, sv, pact, l);
		}, to_d < 0 ? static_cast<size_t>(-to_d) : 0, to_d > 0 ? static_cast<size_t>(to_d) : 0);
		l.lock();
		return res;
	}
	return true;
}

void make_pact_offer(char_id_t from, char_id_t to, INOUT(pact_data) pact, INOUT(pact_mission) m, INOUT(bool) possible, INOUT(w_lock) l) noexcept {
		switch (pact.pact_type) {
		case P_DEFENCE_AGAINST:
			if (make_defensive_against_offer(from, to, pact, m, possible, l) && pact_reaction_a(get_associated_admin(to, l), to, pact, l))
				return;
			break;
		case P_NON_AGRESSION:
			if (make_nonaggression_offer(from, to, pact, m, possible, l) && pact_reaction_a(get_associated_admin(to, l), to, pact, l))
				return;
			break;
		case P_DEFENCE:
			if (make_defensive_offer(from, to, pact, m, possible, l) && pact_reaction_a(get_associated_admin(to, l), to, pact, l))
				return;
			break;
		}
	
	m.offers.pop_back();
}


void do_envoy_meeting(char_id_t from, char_id_t envoy, char_id_t to, IN(w_lock) l) noexcept {
	with_udata_2(to, envoy, l, [from, envoy, to, &l](IN(udata) td, IN(udata) ed) noexcept {
		IN(auto) ev = event_template_by_id(EVENT_MEETING);
		const auto occ = get_occurance(ev, td, ed, 0);

		if (to == global::playerid && occ != 0) {
			message_popup(get_simple_string(TX_ENVOY_RECEIVED), [occ, envoy, to, from](IN(std::shared_ptr<uiElement>) sv) noexcept {
				with_udata_2(to, envoy, r_lock(), [envoy, from, to, occ, &sv](IN(udata) itd, IN(udata) ied) noexcept {
					size_t params[2] = {envoy.value, from.value};
					const int y = create_tex_block(TX_ENVOY_NOTICE, params, 2, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
					create_occurance_text(event_template_by_id(EVENT_MEETING), occ, envoy, to, ied, itd, 0, y + 15, sv->pos.width - 10, sv);
				});
			});
		}
		int relation = relation_delta_by_occurance(occ);
		if (relation != 0) {
			adjust_relation_symmetric(to, envoy, static_cast<char>(relation), l);
			adjust_relation(to, from, static_cast<char>((relation + SGN(relation)) / 2), l);
		}
	});
}

void pact_mission::setup_next_offer(char_id_t actor, admin_id_t source, unsigned int on_day, IN(w_lock) l) noexcept {
	if (!offers_to_make.empty()) {
		const auto p = offers_to_make.back();
		offers_to_make.pop_back();
		global::schedule.emplace(on_day + p.second, new s_pact_mission_exec(actor, p.first, source, this->index));
	} else {
		global::schedule.emplace(on_day + 183, new s_mission_expire(actor, source, this->index));
	}
}

void pact_mission::start_mission(char_id_t actor, admin_id_t source, unsigned int on_day, IN(w_lock) l) noexcept {
	unsigned int cday = on_day;

	for (IN(auto) p : offers_to_make) {
		cday += p.second;
	}

	start_date = on_day;
	return_date = cday;
	expiration_date = cday + 183;

	setup_next_offer(actor, source, on_day, l);
	with_udata(envoy, l, [](INOUT(udata) d) noexcept { d.flags |= P_FLAG_ON_MISSION; });
}

void pact_mission::get_target_list(admin_id_t source, IN(g_lock) l) noexcept {
	const auto source_hos = head_of_state(source, l);
	switch (type) {
	case MIS_DEFENSIVE_AGAINST:
	{
		cvector<char_id_t> nearby;
		global::get_nearby_independant(source_hos, nearby, get_object(get_object(source,l).associated_title).type, l);

		for (const auto p : nearby) {
			if (target != p && !has_defensive_pact_against(source_hos, p, target, l)) {
				offers_to_make.emplace_back(p, std::max(1u, global::days_between(p, source_hos, l)));
			}
		}
	} break;
	case MIS_NON_AGGRESSION_TO:
		if (!has_non_agression_pact(source_hos, target, l)) {
			offers_to_make.emplace_back(target, std::max(1u, global::days_between(source_hos, target, l)));
		}
		break;
	case MIS_DEFENSIVE_PACT:
	{
		cvector<char_id_t> nearby;
		global::get_nearby_independant(source_hos, nearby, get_object(get_object(source, l).associated_title).type, l);

		for (const auto p : nearby) {
			if (!has_defensive_pact(source_hos, p, l)) {
				offers_to_make.emplace_back(p, std::max(1u, global::days_between(p, source_hos, l)));
			}
		}
	} break;
	default: break;
	}
	
	static const auto sort_f = [](IN(std::pair<char_id_t, unsigned int>) a, IN(std::pair<char_id_t, unsigned int>) b) {
		return mu_estimate(a.first, fake_lock()) < mu_estimate(b.first, fake_lock());
	};
	std::sort(offers_to_make.begin(), offers_to_make.end(), std::cref(sort_f));
}

void pact_mission::make_offer(admin_id_t source, char_id_t person, char_id_t other, INOUT(w_lock) l) noexcept {
	if (get_object(envoy).died != 0)
		return;

	const size_t isize = offers.size();
	bool possible = false;

	switch (type) {
	case MIS_DEFENSIVE_AGAINST:
	{
		if (!has_defensive_pact_against(person, other, target, l)) {
			offers.emplace_back();
			offers.back().a = person;
			offers.back().b = other;
			offers.back().against = target;
			offers.back().pact_type = P_DEFENCE_AGAINST;

			do_envoy_meeting(person, envoy, other, l);
			make_pact_offer(person, other, offers.back(), *this, possible, l);
		}

	} break;
	case MIS_NON_AGGRESSION_TO:
		if (!has_non_agression_pact(person, target, l)) {
			offers.emplace_back();
			offers.back().a = person;
			offers.back().b = target;
			offers.back().pact_type = P_NON_AGRESSION;
			
			do_envoy_meeting(person, envoy, target, l);
			make_pact_offer(person, target, offers.back(), *this, possible, l);
		}
		break;
	case MIS_DEFENSIVE_PACT:
	{
		if (!has_defensive_pact(person, other, l)) {
			offers.emplace_back();
			offers.back().a = person;
			offers.back().b = other;
			offers.back().pact_type = P_DEFENCE;
		
			do_envoy_meeting(person, envoy, other, l);
			make_pact_offer(person, other, offers.back(), *this, possible, l);
		}

	} break;
	default: return;
	}

	if ( offers.size() != isize) {
		if (person != global::playerid) {
			global::actionlist.add_new<accept_envoy_offer>(person, source, this->index, 0);
		} else {
			global::setFlag(FLG_MISSIONS_UPDATE);
			modeless_trinary_popup(global::uicontainer, get_simple_string(TX_ENVOY_RESULT), [other, &l, th = this](IN(std::shared_ptr<uiScrollView>) sv) {
				size_t param = other.value;
				int y = create_tex_block(TX_ENVOY_RETURN, &param, 1, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text) + 5;
				pact_to_ui(0, y, sv, th->offers.back(), l);
			}, [i = this->index, source, onum = (offers.size()-1)](int res) {
				if (res == 1) {
					global::actionlist.add_new<accept_envoy_offer>(char_id_t(global::playerid), source, i, static_cast<unsigned int>(onum));
				} else if (res == -1) {
					r_lock l;
					if(const auto tm = get_envoy_mission(source, i, l)) {
						tm->decline_offer(source, char_id_t(global::playerid), onum, l);
					}
				}
			}, 0, 0, 0);
			
		}

	}
}

unsigned int pact_mission::days_until_offers(char_id_t person,  IN(g_lock) l) noexcept {
	switch (type) {
	case MIS_DEFENSIVE_AGAINST:
	{
		cvector<char_id_t> nearby;
		global::get_nearby_independant(target, nearby, get_object(get_object(person).primetitle).type, l);
		unsigned int maxd = 0;
		for (const auto p : nearby) {
			maxd = std::max(maxd, global::days_between(person, p, l));
		}
		return maxd;
	}
	case MIS_NON_AGGRESSION_TO:
		return global::days_between(person, target, l);
	case MIS_DEFENSIVE_PACT:
	{
		cvector<char_id_t> nearby;
		global::get_nearby_independant(person, nearby, get_object(get_object(person).primetitle).type, l);
		unsigned int maxd = 0;
		for (const auto p : nearby) {
			maxd = std::max(maxd, global::days_between(person, p, l));
		}
		return maxd;
	}
	default: return 0;
	}
}

bool pact_mission::identical(IN(envoy_mission) m) const noexcept {
	if (envoy_mission::identical(m)) {
		if (type == MIS_NON_AGGRESSION_TO || type == MIS_DEFENSIVE_AGAINST) {
			return target == ((pact_mission*)&m)->target;
		}
		return true;
	}
	return false;
}


bool selection_valid(IN(std::shared_ptr<ui_button_disable>) button) noexcept {
	switch (mission_selection.current_selection) {
	case MIS_DEFENSIVE_AGAINST:
		if (!valid_ids(mission_selection.target)) {
			button->disable(get_simple_string(TX_NO_TARGET));
			return false;
		}
		return true;
	case MIS_DEFENSIVE_PACT: return true;
	case MIS_NON_AGGRESSION_TO:
		if (!valid_ids(mission_selection.target)) {
			button->disable(get_simple_string(TX_NO_TARGET));
			return false;
		}
		return true;
	default: return true;
	}
}

update_record emission_selection_rec([]{
	if (em_selection::em_selection_window->gVisible()) {
		em_selection::update(mission_selection, r_lock());
	}
});

update_record emission_display_rec([] {
	if (emission_display::emission_display_window->gVisible()) {
		r_lock l;
		emission_display::update(emissions_for_a, l);
	}
});

void open_mission_selection_a(admin_id_t target) noexcept {
	emissions_for_a = target;
	em_selection::em_selection_window->setVisible(true);
	mission_selection.target = 0;
	mission_selection.envoy = 0;
	emission_selection_rec.needs_update = true;
	global::uiqueue.push([] {
		em_selection::em_selection_window->toFront(global::uicontainer);
	});
}

bool can_add_mission_a(admin_id_t source, IN(g_lock) l) noexcept {
	return envoy_missions.count(source, l) < max_envoy_missions_a(source, l);
}

bool can_add_defensive_mission_a(admin_id_t source, IN(g_lock) l) noexcept {
	if (envoy_missions.count(source, l) < max_envoy_missions_a(source, l)) {
		return ! envoy_missions.for_each_breakable(source, l, [](IN(concurrent_uniq<envoy_mission>) ptr) {
			return ptr->type == envoy_mission::DEFENSIVE;
		});
	}
	return false;
}

bool can_add_defensive_against_mission_a(admin_id_t source, char_id_t against, IN(g_lock) l) noexcept {
	if (envoy_missions.count(source, l) < max_envoy_missions_a(source, l)) {
		return !envoy_missions.for_each_breakable(source, l, [against](IN(concurrent_uniq<envoy_mission>) ptr) {
			return ptr->type == envoy_mission::DEFENSIVE_AGAINST && ((pact_mission*)ptr.get())->target == against;
		});
	}
	return false;
}

bool can_add_non_aggression_mission_a(admin_id_t source, char_id_t with, IN(g_lock) l) noexcept {
	if (envoy_missions.count(source, l) < max_envoy_missions_a(source, l)) {
		return !envoy_missions.for_each_breakable(source, l, [with](IN(concurrent_uniq<envoy_mission>) ptr) {
			return ptr->type == envoy_mission::NON_AGGRESSION && ((pact_mission*)ptr.get())->target == with;
		});
	}
	return false;
}

unsigned int max_envoy_missions_a(admin_id_t a, IN(g_lock) l) noexcept {
	return static_cast<int>(get_object(a, l).stats.get_social() + 1.0);
	//return std::max(2,static_cast<int>(global::titles[global::people[person].primetitle].stats.social));
}

void open_envoys_window_a(admin_id_t target) noexcept {
	emissions_for_a = target;
	emission_display::emission_display_window->setVisible(true);
	emission_display_rec.needs_update = true;

	global::uiqueue.push([] {
		emission_display::emission_display_window->toFront(global::uicontainer);
	});
}

bool mission_possible_a(admin_id_t id, unsigned char i) noexcept {
	//TODO
	return true;
}

void add_generic_mission(char_id_t person, admin_id_t id, concurrent_uniq<envoy_mission> &&ptr, IN(g_lock) l) noexcept {
	ptr->get_target_list(id, l);
	global::actionlist.add_new<setup_envoy_mission>(person, id, std::move(ptr));
}

void add_defensive_pact_mission_a(admin_id_t id, char_id_t person, IN(g_lock) l) noexcept {
	auto ptr = concurrent_unique_cast<pact_mission, envoy_mission>(static_cast<unsigned char>(MIS_DEFENSIVE_PACT));

	add_generic_mission(person, id, std::move(ptr), l);
}

void add_defensive_against_mission_a(admin_id_t id, char_id_t person, char_id_t against, IN(g_lock) l) noexcept {
	auto ptr = concurrent_unique_cast<pact_mission, envoy_mission>(static_cast<unsigned char>(MIS_DEFENSIVE_AGAINST));
	((pact_mission*)ptr.get())->target = against;

	add_generic_mission(person, id, std::move(ptr), l);
}

void add_nonagression_mission_a(admin_id_t id, char_id_t person, char_id_t target, IN(g_lock) l) noexcept {
	auto ptr = concurrent_unique_cast<pact_mission, envoy_mission>(static_cast<unsigned char>(MIS_NON_AGGRESSION_TO));
	((pact_mission*)ptr.get())->target = target;

	add_generic_mission(person, id, std::move(ptr), l);
}

void add_selected_mission_a(admin_id_t id, char_id_t person, IN(g_lock) l) noexcept {
	switch (mission_selection.current_selection) {
	case MIS_DEFENSIVE_AGAINST:
		if (valid_ids(mission_selection.target)) {
			add_defensive_against_mission_a(id, person, mission_selection.target, l);
		}
		return;
	case MIS_DEFENSIVE_PACT:
		add_defensive_pact_mission_a(id, person, l);
		return;
	case MIS_NON_AGGRESSION_TO:
		if (valid_ids(mission_selection.target)) {
			add_nonagression_mission_a(id, person, mission_selection.target, l);
		}
		return;
	default: return;
	}
}

bool can_be_envoy(char_id_t id, IN(g_lock) l) noexcept {
	return with_udata(id, l, false, [id](IN(udata) u) noexcept {
		if ((u.flags & P_FLAG_ON_MISSION) != 0)
			return false;
		if (global::currentday - get_object(id).born < 365 * 18)
			return false;
		return true;
	});
}


void em_selection::disable_start_mission(IN(std::shared_ptr<ui_button_disable>) element, IN(emission_selection) obj, IN(g_lock) l) {
	selection_valid(element);
}


void emission_display::disable_new_mission(IN(std::shared_ptr<ui_button_disable>) element, IN(admin_id_t) obj, IN(g_lock) l) {
	if (!can_add_mission_a(emissions_for_a, l))
		element->disable(get_simple_string(TX_NO_MIS_SLOTS));
}

void em_selection::envoy_c_select_list(INOUT(cvector<char_id_t>) vec) {
	r_lock l;
	enum_court_a(emissions_for_a, vec, l);
	vector_erase_if(vec, [&l](char_id_t id) {
		return !can_be_envoy(id, l);
	});
}

void em_selection::envoy_c_select_action(char_id_t id) {
	mission_selection.envoy = id.value;
	emission_selection_rec.needs_update = true;
}

void em_selection::mission_type_options(IN(std::shared_ptr<uiDropDown>) dd, IN(emission_selection) obj, IN(g_lock) l) {
	//mission_selection.current_selection = 0;
	//mission_selection.target = 0;
	//mission_selection.envoy = 0;

	for (decltype(mission_selection.current_selection) i = MIS_DEFENSIVE_AGAINST; i <= MIS_NON_AGGRESSION_TO; ++i) {
		if (mission_possible_a(emissions_for_a, i)) {
			size_t ind = i;

			dd->add_option(get_p_string(TX_MISSION_NAME, &ind, 1), i);

			if (mission_selection.current_selection == 0)
				mission_selection.current_selection = i;
		} else if (mission_selection.current_selection == i) {
			mission_selection.current_selection = 0;
			em_selection::content_panes->deactivate_panes();
		}
	}
	dd->chooseaction = [](uiDropDown* dd, int option) {
		mission_selection.current_selection = static_cast<unsigned char>(option);
		switch (option) {
		case MIS_DEFENSIVE_AGAINST:
			em_selection::content_panes->activate_pane(em_selection::def_against);
			break;
		case MIS_NON_AGGRESSION_TO:
			em_selection::content_panes->activate_pane(em_selection::non_aggression);
			break;
		case MIS_DEFENSIVE_PACT:
			em_selection::content_panes->activate_pane(em_selection::def_pact);
			break;
		default:
			em_selection::content_panes->deactivate_panes();
			break;
		}
		start_mission->enable();
		selection_valid(start_mission);
	};
}

void em_selection::def_c_select_list(INOUT(cvector<char_id_t>) vec) {
	global::get_recent_people(vec);
	vector_erase(vec, global::playerid);
}

void em_selection::def_c_select_action(char_id_t id) {
	mission_selection.target = id;
	emission_selection_rec.needs_update = true;
}

void em_selection::na_c_select_list(INOUT(cvector<char_id_t>) vec) {
	global::get_recent_people(vec);
	vector_erase(vec, global::playerid);
}

void em_selection::na_c_select_action(char_id_t id) {
	mission_selection.target = id;
	emission_selection_rec.needs_update = true;
}

void em_selection::start_mission_action(ui_button_disable* b) {
	em_selection::em_selection_window->setVisible(false);
	add_selected_mission_a(emissions_for_a, char_id_t(global::playerid), r_lock());
}

void em_selection::cancel_action(uiButton* obj) {
	em_selection::em_selection_window->setVisible(false);
}


void emission_display::new_mission_action(ui_button_disable* b) {
	open_mission_selection_a(emissions_for_a);
}


std::wstring get_mission_name(unsigned char id) noexcept {
	size_t param = id;
	return get_p_string(TX_MISSION_NAME, &param, 1);
}

void envoy_mission::expire_mission(admin_id_t source, IN(g_lock) l) noexcept {
	with_udata(envoy, l, [](INOUT(udata) d) noexcept {
		d.flags &= ~P_FLAG_ON_MISSION;
	});
}