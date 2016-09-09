#include "globalhelpers.h"
#include "pacts.h"
#include "relations.h"
#include "structs.hpp"
#include "requests.h"
#include "ChPane.h"
#include "datamanagement.hpp"
#include "i18n.h"
#include "finances.h"
#include "living_data.h"
#include "reputation.h"

namespace global {
	v_pool_t<pact_data, pact_id> pacts;
};

struct {
	std::shared_ptr<uiMenuRect> window;
	std::shared_ptr<uiDropDown> select_type;
	std::shared_ptr<uiButton> accpet_button;
	std::shared_ptr<uiScrollView> contents;

	guarantee current;
} guarantee_selection;

struct {
	std::shared_ptr<uiDragRect> window;
	std::shared_ptr<uiSimpleText> d_label;
	std::shared_ptr<uiDropDown> duration;
	std::shared_ptr<uiScrollView> guarantees;

	std::shared_ptr<uiButton> accpet_button;
	std::shared_ptr<uiButton> decline_button;

	pact_data pact;
	std::function<void(bool, const pact_data&)> result;
	event signal;
} pact_construction;

#define GS_WIDTH 200
#define GS_HEIGHT 200

#define PC_WIDTH 400
#define PC_HEIGHT 500

void update_offer_gs(IN(std::shared_ptr<uiScrollView>) sv, INOUT(pact_data) pact) noexcept;

bool guarantee_filled() noexcept {
	switch (guarantee_selection.current.type) {
	case G_MARRIAGE:
		return guarantee_selection.current.data.marriage.a != 0 && guarantee_selection.current.data.marriage.b != 0;
	case G_TRIBUTE:
		return guarantee_selection.current.data.tribute.to != 0;
	}
	return true;
}

void init_pacts_ui() noexcept {
	guarantee_selection.window = std::make_shared<uiMenuRect>(global::uicontainer, 50, 50, GS_WIDTH, GS_HEIGHT, global::solid_border);

	guarantee_selection.window->add_element<uiGButton>(GS_WIDTH - 20, 0, 20, 20, &global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [ptr = &guarantee_selection](uiGButton *a) { ptr->window->close(); });
	guarantee_selection.contents = guarantee_selection.window->add_element<uiScrollView>(0, 30, GS_WIDTH, GS_HEIGHT - 60);
	guarantee_selection.select_type = guarantee_selection.window->add_element<uiDropDown>(5, 5, GS_WIDTH-30, 20, global::solid_border, global::standard_text, [](uiDropDown* d, int selection) {
		setup_guarantee(selection, pact_construction.pact, r_lock());
	});
	guarantee_selection.select_type->add_option(get_simple_string(TX_GAR_H), G_HONOR);
	guarantee_selection.select_type->add_option(get_simple_string(TX_GAR_M), G_MARRIAGE);
	guarantee_selection.select_type->add_option(get_simple_string(TX_GAR_T), G_TRIBUTE);

	guarantee_selection.window->add_element<uiButton>(5, GS_HEIGHT - 30, 100, 25, get_simple_string(TX_ADD_GAR), global::solid_border, global::standard_text, [gs = &guarantee_selection](uiButton*b) {
		close_pick_guarantee();
		if (guarantee_filled()) {
			pact_construction.pact.guarantees.push_back(guarantee_selection.current);
			update_offer_gs(pact_construction.guarantees, pact_construction.pact);
		}
	});

	pact_construction.window = global::uicontainer->add_element<uiDragRect>(50, 50, PC_WIDTH, PC_HEIGHT, global::solid_border);
	pact_construction.window->setVisible(false);
	pact_construction.d_label = pact_construction.window->add_element<uiSimpleText>(5,25,get_simple_string(TX_E_DURATION), global::empty, global::standard_text);
	pact_construction.duration = pact_construction.window->add_element<uiDropDown>(pact_construction.d_label->pos.width + pact_construction.d_label->pos.left, 25, 150, 20, global::solid_border, global::standard_text, std::vector<std::wstring>{get_simple_string(TX_DUR_I), get_simple_string(TX_DUR_5), get_simple_string(TX_DUR_10)}, [ pc = &pact_construction](uiDropDown* d, int selection) {
		//switch (selection) {
		//case 0: pc->pact.expiration = 0; break;
		//case 1: pc->pact.expiration = global::currentday + 365*5; break;
		//case 2: pc->pact.expiration = global::currentday + 365*10; break;
		//default: break;
		//}
	});
	pact_construction.guarantees = pact_construction.window->add_element<uiScrollView>(5, 55, PC_WIDTH - 10, 100);
	pact_construction.accpet_button = pact_construction.window->add_element<uiButton>(PC_WIDTH / 2 - 155, PC_HEIGHT - 25, 150, 20, get_simple_string(TX_PROPOSE), global::solid_border, global::standard_text, [pc = &pact_construction](uiButton* b) {
		close_offer_window();
		pc->result(true, pc->pact);
		pc->signal.set();
	}, sf::Keyboard::Y);
	pact_construction.decline_button = pact_construction.window->add_element<uiButton>(PC_WIDTH / 2 + 5, PC_HEIGHT - 25, 150, 20, get_simple_string(TX_DECLINE), global::solid_border, global::standard_text, [pc = &pact_construction](uiButton* b) {
		close_offer_window();
		pc->result(false, pc->pact);
		pc->signal.set();
	}, sf::Keyboard::N);
}

void close_offer_window() noexcept {
	pact_construction.window->setVisible(false);
}


int count_defensive_pacts(char_id_t id,  IN(g_lock) l) noexcept {
	int count = 0;
	enum_pacts_for(id, l, [&count, &l](pact_id_t id) {
		if (global::pacts.get(id.value, l).pact_type == P_DEFENCE) {
			++count;
		}
	});
	return count;
}

void add_pact( IN(pact_data) pact, IN(w_lock) l) noexcept {
	const auto key = global::pacts.add(pact, l);
	INOUT(auto) newpact = global::pacts.get(key, l);

	newpact = pact;
	// newpact.pact_id = pact_id_t(key);
	
	relation r;
	r.primary = pact.a.value;
	r.secondary = pact.b.value;
	r.type = REL_PACT;
	r.typedata.id = key;

	for (size_t i = newpact.guarantees.size() - 1; i != SIZE_MAX; -- i) {
		if (newpact.guarantees[i].type == G_MARRIAGE) {
			if (!valid_ids(detail::people[newpact.guarantees[i].data.marriage.a].spouse) && !valid_ids(detail::people[newpact.guarantees[i].data.marriage.b].spouse)) {
				detail::people[newpact.guarantees[i].data.marriage.a].spouse = newpact.guarantees[i].data.marriage.b;
				detail::people[newpact.guarantees[i].data.marriage.b].spouse = newpact.guarantees[i].data.marriage.a;
			} else {
				newpact.guarantees[i] = std::move(pact.guarantees.back());
				newpact.guarantees.pop_back();
			}
		} else if (newpact.guarantees[i].type == G_TRIBUTE) {
			if (newpact.a == char_id_t(newpact.guarantees[i].data.tribute.to)) {
				add_expense(newpact.b, EXPENSE_TRIBUTE, 0, newpact.guarantees[i].data.tribute.amount, l);
				add_expense(newpact.a, INCOME_TRIBUTE, 0, newpact.guarantees[i].data.tribute.amount, l);
			} else {
				add_expense(newpact.a, EXPENSE_TRIBUTE, 0, newpact.guarantees[i].data.tribute.amount, l);
				add_expense(newpact.b, INCOME_TRIBUTE, 0, newpact.guarantees[i].data.tribute.amount, l);
			}
		} 
	}

	add_relation(std::move(r),l);

	if (pact.pact_type == P_DEFENCE || pact.pact_type == P_DEFENCE_AGAINST) {
		update_defensive_pact_gained(pact.a, l);
		update_defensive_pact_gained(pact.b, l);
		//if (IN_P(auto) td = get_tdata(pact.a, l))
		//	td->interests.force_update_interests(pact.a, l);
		//if (IN_P(auto) td = get_tdata(pact.b, l))
		//	td->interests.force_update_interests(pact.b, l);
	}
}


void guarantee_to_ui(int x, int &y, IN(std::shared_ptr<uiElement>) parent, IN(pact_data) pact, IN(guarantee) g, IN(g_lock) l) noexcept {
	switch (g.type) {
	case G_HONOR:
		parent->add_element<uiSimpleText>(x, y, get_simple_string(TX_G_HONOR), global::empty, global::standard_text);
		y += (global::standard_text.csize + 2);
		break;
	case G_MARRIAGE:
	{
		size_t param = g.data.marriage.a;
		get_linear_ui(TX_G_MARRIAGE_OF, &param, 1, parent, x, y, global::empty, global::standard_text);	
		y += (global::standard_text.csize + 2);
		param = g.data.marriage.b;
		get_linear_ui(TX_G_MAND, &param, 1, parent, x, y, global::empty, global::standard_text);
		y += (global::standard_text.csize + 2);
		break;
	}
	case G_TRIBUTE:
	{
		auto label = parent->add_element<uiSimpleText>(x, y, get_simple_string(TX_G_TRIBUTE), global::empty, global::standard_text);
		
		if (pact.a == char_id_t(g.data.tribute.to)) {
			parent->add_element<uiSimpleText>(x + label->pos.width, y, format_float(g.data.tribute.amount), global::empty, global::standard_text);
		} else {
			parent->add_element<uiSimpleText>(x + label->pos.width, y, format_float(g.data.tribute.amount), global::empty, global::standard_text);
		}
		
		
		y += (global::standard_text.csize + 2);

		size_t param = g.data.tribute.to;
		get_linear_ui(TX_TO, &param, 1, parent, x, y, global::empty, global::standard_text);
		y += (global::standard_text.csize + 2);
		break;
	}
	default: break;
	}
}

std::wstring pact_header(int type) noexcept {
	size_t param = static_cast<size_t>(type);
	return get_p_string(TX_PACT_HEADER, &param, 1);
}

void pact_to_ui(int x, int &y, IN(std::shared_ptr<uiElement>) parent, IN(pact_data) pact, IN(g_lock) l) noexcept {
	parent->add_element<uiSimpleText>(x, y, pact_header(pact.pact_type), global::empty, global::standard_text);
	y += (global::standard_text.csize + 2);

	size_t param = pact.a.value;
	get_linear_ui(TX_EX_NAME_AND, &param, 1, parent, x + 10, y, global::empty, global::standard_text);

	y += (global::standard_text.csize + 2);
	parent->add_element<uiHLink>(x + 10, y, global::get_expanded_name(pact.b), global::empty, global::standard_text, global::whandle, [ ch = pact.b](uiHLink* h){SetupChPane( ch); });
	y += (global::standard_text.csize + 2);
	if (pact.pact_type == P_DEFENCE_AGAINST) {
		param = pact.against.value;
		get_linear_ui(TX_AGAINST_CH, &param, 1, parent, x + 10, y, global::empty, global::standard_text);
		y += (global::standard_text.csize + 2);
	}

	/*if (pact.expiration == 0) {
		parent->add_element<uiSimpleText>(x + 10, y, get_simple_string(TX_INDEFINATE_D), global::empty, global::standard_text);
	} else {
		const date cday = date(1400, Jan, 1) + days(pact.expiration);
		size_t mparams[2] = {static_cast<size_t>(cday.month()), static_cast<size_t>(cday.year() - 1400)};
		parent->add_element<uiSimpleText>(x + 10, y, get_p_string(TX_UNTIL_M, mparams, 2), global::empty, global::standard_text);
	}
	y += (global::standard_text.csize + 2);*/

	if (pact.guarantees.size() > 0) {
		parent->add_element<uiSimpleText>(x + 10, y, get_simple_string(TX_GAR_BY), global::empty, global::standard_text);
	} else {
		parent->add_element<uiSimpleText>(x + 10, y, get_simple_string(TX_NO_GAR), global::empty, global::standard_text);
	}
	y += (global::standard_text.csize + 2);

	for (const auto&  __restrict g : pact.guarantees) {
		guarantee_to_ui(x + 20, y, parent, pact, g, l);
	}
}

bool extend_pact_popup(IN(pact_data) pact) noexcept {
	return make_yes_no_popup(global::uicontainer, get_simple_string(TX_PACT_EXPIRE), [&pact](const std::shared_ptr<uiScrollView>& sv) {
		sv->add_element<uiSimpleText>(5, 0, get_simple_string(TX_EXTEND_PROMPT), global::empty, global::standard_text);
		int y = 20;
		pact_to_ui(0, y, sv, pact, r_lock());
	}, 0.0);
}

void pact_tribute_notpaid_popup(IN(pact_data) pact, char_id_t breaker, IN(g_lock) l) noexcept {
	message_popup(global::uicontainer, get_simple_string(TX_PACT_ENDED), [&pact, &l, breaker](const std::shared_ptr<uiScrollView>& sv) {
		size_t param = breaker.value;
		const auto tb = create_tex_block(TX_PACT_TRIBUTE_EX, &param, 1, sv, 1, 1, sv->pos.width - 5, global::empty, global::standard_text);
		sv->subelements.push_back(tb);
		int y = tb->pos.height + 10;
		pact_to_ui(1, y, sv, pact, l);
	});
}

void pact_expired_popup(IN(pact_data) pact, IN(g_lock) l) noexcept {
	message_popup(global::uicontainer, get_simple_string(TX_PACT_ENDED), [&pact, &l](const std::shared_ptr<uiScrollView>& sv) {
		int y = 0;
		pact_to_ui(0, y, sv, pact, l);
	});
}

void pact_entered_popup(IN(pact_data) pact, IN(g_lock) l) noexcept {
	message_popup(global::uicontainer, get_simple_string(TX_PACT_CREATED), [&pact, &l](const std::shared_ptr<uiScrollView>& sv) {
		int y = 0;
		pact_to_ui(0, y, sv, pact, l);
	});
}


void pact_declined_popup(IN(pact_data) pact, IN(g_lock) l) noexcept {
	message_popup(global::uicontainer, get_simple_string(TX_PACT_DECLINED), [&pact, &l](const std::shared_ptr<uiScrollView>& sv) {
		sv->add_element<uiSimpleText>(5, 0, get_simple_string(TX_LONG_PACT_DECLINED), global::empty, global::standard_text);
		int y = global::standard_text.csize + 2;
		pact_to_ui(0, y, sv, pact, l);
	});
}

void vaidate_guarantees(INOUT(pact_data) pact, IN(g_lock) l) noexcept {
	for (size_t i = pact.guarantees.size() - 1; i != SIZE_MAX; --i) {
		if (pact.guarantees[i].type == G_MARRIAGE) {
			if (!global::are_marriable(char_id_t(pact.guarantees[i].data.marriage.a), char_id_t(pact.guarantees[i].data.marriage.b))) {
				pact.guarantees[i] = std::move(pact.guarantees.back());
				pact.guarantees.pop_back();
			}
		} 
	}
}

void update_pacts(INOUT(w_lock) l) noexcept {
	global::pacts.for_each(l, [&l](INOUT(pact_data) pact) {
		if(pact.pact_type != 0) {
			if(get_object(pact.a).died != 0 || get_object(pact.b).died != 0) {
				if (global::interested.in_set(pact.a.value) || global::interested.in_set(pact.b.value))
					pact_expired_popup(pact, l);
				clear_finished_pact(pact, l);
				global::pacts.free(pact, l);
			} else {
				for (size_t i = pact.guarantees.size() - 1; i != SIZE_MAX; --i) {
					if (pact.guarantees[i].type == G_TRIBUTE) {
						const auto tsender = (pact.a == char_id_t(pact.guarantees[i].data.tribute.to)) ? pact.b : pact.a;
						if (global::get_wealth(tsender, l) < 0.0) {
							if (global::interested.in_set(pact.a.value) || global::interested.in_set(pact.b.value))
								pact_tribute_notpaid_popup(pact, tsender, l);
							break_pact(pact, tsender, l);
							break;
						}
					} else if (pact.guarantees[i].type == G_MARRIAGE) {
						if (detail::people[pact.guarantees[i].data.marriage.a].died != 0 ||
							detail::people[pact.guarantees[i].data.marriage.b].died != 0 || 
							detail::people[pact.guarantees[i].data.marriage.b].spouse.value != pact.guarantees[i].data.marriage.a) {
							pact.guarantees[i] = std::move(pact.guarantees.back());
							pact.guarantees.pop_back();
						}
					}
				}
			}
		}
	});
	//global::pacts.erase_if(l, [](size_t indx, const pact_data& pact) { return pact.pact_type == 0; });
}

void free_pact(INOUT(pact_data) pact, IN(w_lock) l) noexcept {
	for (IN(auto) g : pact.guarantees) {
		if (g.type == G_TRIBUTE) {
			if (pact.a == char_id_t(g.data.tribute.to)) {
				remove_expense(char_id_t(pact.b), EXPENSE_TRIBUTE, g.data.tribute.amount, l);
				remove_expense(char_id_t(pact.a), INCOME_TRIBUTE, g.data.tribute.amount, l);
			} else {
				remove_expense(char_id_t(pact.a), EXPENSE_TRIBUTE, g.data.tribute.amount, l);
				remove_expense(char_id_t(pact.b), INCOME_TRIBUTE, g.data.tribute.amount, l);
			}
		}
	}
	if (pact.pact_type == P_DEFENCE || pact.pact_type == P_DEFENCE_AGAINST) {
		update_defensive_pact_lost(char_id_t(pact.a), l);
		update_defensive_pact_lost(char_id_t(pact.b), l);
	}
	global::pacts.free(pact, l);
}

bool has_non_agression_pact(char_id_t a, char_id_t b, IN(g_lock) l) noexcept {
	bool found = false;
	enum_pacts_between(a, b, l, [&found, &l](pact_id_t id) {
		if (global::pacts.get(id.value, l).pact_type == P_NON_AGRESSION) {
			found = true;
		}
	});
	return found;
}

bool has_defensive_pact(char_id_t a, char_id_t b, IN(g_lock) l) noexcept {
	bool found = false;
	enum_pacts_between(a, b, l, [&found, &l](pact_id_t id) {
		if (get_object(id, l).pact_type == P_DEFENCE) {
			found = true;
		}
	});
	return found;
}

bool has_defensive_pact_against(char_id_t a, char_id_t b, char_id_t target, IN(g_lock) l) noexcept {
	bool found = false;
	enum_pacts_between(a, b, l, [&found, &l, target](pact_id_t id) {
		IN(auto) pct = get_object(id, l);
		if ((pct.pact_type == P_DEFENCE) | ((pct.pact_type == P_DEFENCE_AGAINST) & (pct.against == target))) {
			found = true;
		}
	});
	return found;
}



void open_pick_guarantee(int x, int y,  IN(std::function<void(const guarantee&)>) handle_result) noexcept {
	global::uiqueue.push([x, y, gs = &guarantee_selection] {
		memset(&(gs->current.data), 0, sizeof(gs->current.data));
		setup_guarantee(gs->select_type->chosen, pact_construction.pact, r_lock());
		gs->window->open(global::uicontainer);
		gs->window->move_to(x, y);
	});
}

void setup_guarantee(unsigned int type, IN(pact_data) pact, IN(g_lock) l) noexcept {
	guarantee_selection.current.type = static_cast<unsigned char>(type);
	guarantee_selection.contents->subelements.clear();

	switch (type) {
	case G_HONOR:
		break;
	case G_MARRIAGE:
	{
		character_selection_menu(guarantee_selection.contents, 10, 5, 50, 20, get_simple_string(TX_SELECT), global::solid_border, global::standard_text, [ pc = &pact_construction](cvector<char_id_t> &lst) {
			global::get_living_family(pc->pact.a, lst);
			vector_erase_if(lst, [](char_id_t id) {return !global::is_marriable(id); });
		}, [ gs = &guarantee_selection](char_id_t id) {
			gs->current.data.marriage.a = id.value;
			gs->current.data.marriage.b = person::NONE;
			global::uiqueue.push([ gs] {
				setup_guarantee(gs->current.type, pact_construction.pact, r_lock());
			});
		});

		if (guarantee_selection.current.data.marriage.a != char_id_t::NONE) {
			character_selection_menu(guarantee_selection.contents, 10, 30, 50, 20, get_simple_string(TX_SELECT), global::solid_border, global::standard_text,  [ o = guarantee_selection.current.data.marriage.a, pc = &pact_construction](cvector<char_id_t> &lst) {
				global::get_living_family(pc->pact.b, lst);
				vector_erase_if(lst, [o](char_id_t id) {return !global::are_marriable(id, char_id_t(o)); });
			}, [ gs = &guarantee_selection](char_id_t id) {
				gs->current.data.marriage.b = id.value;
				global::uiqueue.push([ gs] {
					setup_guarantee(gs->current.type, pact_construction.pact, r_lock());
				});
			});
			guarantee_selection.contents->add_element<uiSimpleText>(0, 30, TEXT("&"), global::empty, global::standard_text);


			guarantee_selection.contents->add_element<uiHLink>(65, 0, global::w_character_name(char_id_t(guarantee_selection.current.data.marriage.a)), global::empty, global::standard_text, global::whandle,
															   [ id = guarantee_selection.current.data.marriage.a](uiHLink* h) {SetupChPane(char_id_t(id)); });
		}

		if (guarantee_selection.current.data.marriage.b != 0) {
			guarantee_selection.contents->add_element<uiHLink>(65, 30, global::w_character_name(char_id_t(guarantee_selection.current.data.marriage.b)), global::empty, global::standard_text, global::whandle,
															   [ id = guarantee_selection.current.data.marriage.b](uiHLink* h) {SetupChPane(char_id_t(id)); });
		}
		break;
	}
	case G_TRIBUTE:
	{
		const auto label = guarantee_selection.contents->add_element<uiSimpleText>(5, 5, get_simple_string(TX_E_TO), global::empty, global::standard_text);
		character_selection_menu(guarantee_selection.contents, label->pos.left + label->pos.width , 5, 50, 20, get_simple_string(TX_SELECT), global::solid_border, global::standard_text,  [ pc = &pact_construction](cvector<char_id_t> &lst) {
			lst.emplace_back(pc->pact.a);
			lst.emplace_back(pc->pact.b);
		}, [ gs = &guarantee_selection](char_id_t id) {
			gs->current.data.tribute.to = id.value;
			global::uiqueue.push([ gs] {
				setup_guarantee(gs->current.type, pact_construction.pact, r_lock());
			});
		});
		if (guarantee_selection.current.data.tribute.to != 0) {
			guarantee_selection.contents->add_element<uiHLink>(label->pos.left + label->pos.width + 55, 5, global::w_character_name(char_id_t(guarantee_selection.current.data.tribute.to)), global::empty, global::standard_text, global::whandle,
															   [ id = guarantee_selection.current.data.tribute.to](uiHLink* h) {SetupChPane(char_id_t(id)); });
			if (pact.a == char_id_t(guarantee_selection.current.data.tribute.to)) {
				guarantee_selection.contents->add_element<uiSimpleText>(5, 25, get_simple_string(TX_AMOUNT) + format_double(global::project_income(pact.b, l) * 0.1), global::empty, global::standard_text);
			} else {
				guarantee_selection.contents->add_element<uiSimpleText>(5, 25, get_simple_string(TX_AMOUNT) + format_double(global::project_income(pact.a, l) * 0.1), global::empty, global::standard_text);
			}
		}

		break;
	}
	default: break;
	}
	guarantee_selection.contents->calcTotalHeight();
}

void close_pick_guarantee() noexcept {
	guarantee_selection.window->close();
}

void update_offer_gs(IN(std::shared_ptr<uiScrollView>) sv, INOUT(pact_data) pact) noexcept {
	global::uiqueue.push([sv,&pact, pc = &pact_construction] {
		pc->guarantees->subelements.clear();

		int yoff = 5;
		text_format red{14, global::font, sf::Color::Red};
		{
			r_lock l;
			for (size_t indx = 0; indx < pact.guarantees.size(); ++indx) {
				sv->add_element<uiButton>(5, yoff, 15, 20, TEXT("X"), global::solid_border, red, [&pact, indx, wsv = std::weak_ptr<uiScrollView>(sv)](uiButton*b) {
					pact.guarantees[indx] = pact.guarantees.back();
					pact.guarantees.pop_back();
					if (const auto s = wsv.lock())
						update_offer_gs(s, pact);
				});
				guarantee_to_ui(25, yoff, sv, pact, pact.guarantees[indx], l);
			}
		}

		sv->add_element<uiButton>(5, yoff, 100, 20, get_simple_string(TX_ADD_GAR), global::solid_border, global::standard_text, [ &pact, wsv = std::weak_ptr<uiScrollView>(sv)](uiButton* __restrict b) {
			int x = 0;
			int y = 0;
			b->posabs(x, y);
			open_pick_guarantee(x, y,  [ &pact, w = wsv](IN(guarantee)g) {
				pact.guarantees.push_back(g);
				if (const auto s = w.lock())
					update_offer_gs(s, pact);
			});
		});

		pc->guarantees->calcTotalHeight();
	});

}

void open_offer_window(char_id_t from, IN(pact_data) pact,  IN(std::function<void(bool, const pact_data&)>) result) noexcept {
	pact_construction.signal.reset();
	pact_construction.pact = pact;
	pact_construction.result = result;

	global::uiqueue.push([ from, pc = &pact_construction] {
		pc->window->subelements.clear();

		int ystart = 0;

		size_t param = pc->pact.pact_type;

		switch (pc->pact.pact_type) {
		case P_DEFENCE:
		{
			pc->window->add_element<uiCenterdText>(0, 4, PC_WIDTH, get_p_string(TX_PACT_OFFER, &param,1), global::empty, global::header_text);

			size_t iparam = from.value;
			get_linear_ui(TX_FROM_CH, &iparam, 1, pc->window, 5, 24, global::empty, global::standard_text);
			ystart = 24 + global::standard_text.csize + 4;
		}
			break;
		case P_DEFENCE_AGAINST:
		{
			pc->window->add_element<uiCenterdText>(0, 4, PC_WIDTH, get_p_string(TX_PACT_OFFER, &param, 1), global::empty, global::header_text);

			size_t iparam = from.value;
			get_linear_ui(TX_FROM_CH, &iparam, 1, pc->window, 5, 24, global::empty, global::standard_text);
				
			iparam = pc->pact.against.value;
			get_linear_ui(TX_AGAINST_CH, &iparam, 1, pc->window, 5, 24 + global::standard_text.csize + 4, global::empty, global::standard_text);
			
			ystart = 24 + 2*( global::standard_text.csize + 4);
		}
			break;
		case P_NON_AGRESSION:
		{
			pc->window->add_element<uiCenterdText>(0, 4, PC_WIDTH, get_p_string(TX_PACT_OFFER, &param, 1), global::empty, global::header_text);

			size_t iparam = from.value;
			get_linear_ui(TX_FROM_CH, &iparam, 1, pc->window, 5, 24, global::empty, global::standard_text);

			ystart = 24 + global::standard_text.csize + 4;
		}
			break;
		default: break;
		}
		
		pc->d_label->pos.top = ystart;
		pc->window->subelements.push_back(pc->d_label);
		pc->duration->pos.top = ystart;
		pc->window->subelements.push_back(pc->duration);

		pc->guarantees->pos.top = ystart + global::standard_text.csize + 4;
		pc->guarantees->pos.height = (PC_HEIGHT - pc->guarantees->pos.top) - 30;
		pc->window->subelements.push_back(pc->guarantees);

		update_offer_gs(pc->guarantees, pc->pact);

		pc->window->subelements.push_back(pc->accpet_button);
		pc->window->subelements.push_back(pc->decline_button);

		pc->window->setVisible(true);
	});
	event* earray[] = {&pact_construction.signal, &global::quitevent};
	event::wait_for_multiple(earray, 2, false);
}

void break_pact(INOUT(pact_data) pact, char_id_t breaker, IN(w_lock) l) noexcept {
	const auto other_c = (breaker == pact.a) ? pact.b : pact.a;
	const double hloss = honor_loss_on_break_val(pact, breaker,  l);
	
	if (hloss != 0.0) {
		with_udata(breaker, l, [hloss](INOUT(udata) d) noexcept { d.p_honorable -= hloss; });
		adjust_relation(other_c, breaker, -2, l);
	}

	clear_finished_pact(pact, l);
	free_pact(pact, l);
}

double honor_loss_on_break_val(IN(pact_data) pact, char_id_t breaker, IN(g_lock) l) noexcept {
	const auto other_c = (breaker == pact.a) ? pact.b : pact.a;

	return with_udata_2(other_c, breaker, l, 0.0, [&pact, breaker](IN(udata) odat, IN(udata) bdat) noexcept {
		if (odat.p_honorable > 0.49 || odat.p_honorable * 2 >= bdat.p_honorable)
			return bdat.p_honorable - update_reputation(reputation::p_dishonor_reliable, reputation::p_dishonor_unreliable, bdat.p_honorable);

		if (pact.guarantees.size() != 0) {
			for (IN(auto) g : pact.guarantees) {
				if (g.type == G_MARRIAGE && detail::people[g.data.marriage.a].died == 0 && detail::people[g.data.marriage.b].died == 0 && detail::people[g.data.marriage.b].spouse == char_id_t(g.data.marriage.a)) {
					return bdat.p_honorable - update_reputation(reputation::p_dishonor_reliable, reputation::p_dishonor_unreliable, bdat.p_honorable);
				} else if (g.type == G_TRIBUTE && breaker == char_id_t(g.data.tribute.to)) {
					return bdat.p_honorable - update_reputation(reputation::p_dishonor_reliable, reputation::p_dishonor_unreliable, bdat.p_honorable);
				}
			}
		}

		return 0.0;
	});
}

float desirability_of_defensive(char_id_t from, char_id_t other, char_id_t envoy, bool recipient, IN(g_lock) l) noexcept {
	const float tv = (max_threat_percentage(from, l) * mu_estimate(other, l) / mu_estimate(from, l));
	if (recipient) {
		return -2.0f + opinion(from, other, l) + opinion(from, envoy, l) + tv;
	} else {
		return -2.0f + opinion(from, other, l) + opinion(envoy, other, l) + tv;
	}
}

float desirability_of_t_defensive(char_id_t from, char_id_t other, char_id_t against, char_id_t envoy, bool recipient, IN(g_lock) l) noexcept {
	const float tv = mu_estimate(against, l) / (mu_estimate(from, l) + def_mu_estimate(from, l) + def_against_mu_estimate(from, against, l));
	if (recipient) {
		return -1.0f + 0.5f * (opinion(from, other, l) + opinion(from, envoy, l) - 2.0f * opinion(from, against, l)) + tv;
	} else {
		return -1.0f + 0.5f * (opinion(from, other, l) + opinion(envoy, other, l) - 2.0f * opinion(from, against, l)) + tv;
	}
}

float desirability_of_non_aggression(char_id_t from, char_id_t other, char_id_t envoy, bool recipient, IN(g_lock) l) noexcept {
	float baseval = -1.0f;
	const auto tg = interest_status_of(from, other, l);

	if (tg == interest_relation_contents::TAG_THREAT) {
		baseval = 1.0f;
	} else if (tg == interest_relation_contents::TAG_PTHREAT) {
		baseval = 0.0f;
	} else {
		if (tg == interest_relation_contents::TAG_TARGET)
			baseval -= 1.0f;
		with_udata(from, l, [&baseval](IN(udata) d) noexcept {
			if (is_aggressive(d))
				baseval -= 0.5f;
			else if (is_peaceful(d))
				baseval += 0.5f;
		});
	}
	const float tv = mu_estimate(other, l) / (mu_estimate(from, l) + def_mu_estimate(from, l) + def_against_mu_estimate(from, other, l));

	if (recipient) {
		return baseval + 0.5f * (opinion(from, other, l) + opinion(from, envoy, l)) + tv;
	} else {
		return baseval + 0.5f * (opinion(from, other, l) + opinion(envoy, other, l)) + tv;
	}
}

float tribute_adjuistment(IN(pact_data) pact, char_id_t from, IN(g_lock) l) noexcept {
	double r = 0.0;
	for (IN(auto) g : pact.guarantees) {
		if (g.type == G_TRIBUTE) {
			if (from == char_id_t(g.data.tribute.to)) {
				r += g.data.tribute.amount / global::project_income(from, l);
			} else {
				r -= g.data.tribute.amount / global::project_income(from, l);
			}
		}
	}
	return static_cast<float>(r);
}

float continuing_desirability_of_defensive(IN(pact_data) pact, char_id_t from, char_id_t other, IN(g_lock) l) noexcept {
	// IN_P(auto) td = get_tdata(from, l);
	const auto other_mu = mu_estimate(other, l);
	const auto tp = max_p_threat_percentage(from, other_mu, l);
	return std::min(1.0f, tp.second - tp.first) + tribute_adjuistment(pact, from, l);
}

float continuing_desirability_of_t_defensive(IN(pact_data) pact, char_id_t from, char_id_t other, IN(g_lock) l) noexcept {
	const auto against_mu = mu_estimate(pact.against, l);
	const auto other_mu = mu_estimate(other, l);
	const auto self_mu = mu_estimate(from, l) + def_mu_estimate(from, l) + def_against_mu_estimate(from, pact.against, l);
	return std::min(1.0f,  against_mu / (self_mu + against_mu - other_mu) - against_mu / (self_mu + against_mu)) + tribute_adjuistment(pact, from, l);
}

float continuing_desirability_of_non_aggression(IN(pact_data) pact, char_id_t from, char_id_t other, IN(g_lock) l) noexcept {
	const auto other_mu = mu_estimate(other, l);
	return other_mu / (other_mu + mu_estimate(from, l) + def_mu_estimate(from, l) + def_against_mu_estimate(from, other, l));
}

bool honor_loss_on_break(IN(pact_data) pact, char_id_t breaker, IN(g_lock) l) noexcept {
	return honor_loss_on_break_val(pact, breaker, l) != 0.0;
}

float continuing_desirability_of_pact(pact_id_t pact, char_id_t from, IN(g_lock) l) noexcept {
	IN(auto) p = get_object(pact, l);
	const auto other = (p.a == from) ? p.b : p.a;
	switch (p.pact_type) {
	case P_DEFENCE:
		return continuing_desirability_of_defensive(p, from, other, l);
	case P_NON_AGRESSION:
		return continuing_desirability_of_non_aggression(p, from, other, l);
	case P_DEFENCE_AGAINST:
		return continuing_desirability_of_t_defensive(p, from, other, l);
	default:
		return 0.0f;
	}
}