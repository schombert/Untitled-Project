#include "globalhelpers.h"
#include "events.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "relations.h"
#include "requests.h"
#include "ChPane.h"
#include "datamanagement.hpp"
#include "schedule.h"
#include "actions.h"
#include "traits.h"
#include "i18n.h"
#include "living_data.h"
#include "finances.h"

void add_spouses(INOUT(std::vector<char_id_t>) lst) noexcept {
	const auto isize = lst.size();
	for (size_t indx = 0; indx < isize; ++indx) {
		const auto sp = get_object(lst[indx]).spouse;
		if ((valid_ids(sp) & (get_object(sp).died == 0)) && std::find(lst.cbegin(), lst.cend(), sp) == lst.cend())
			lst.push_back(sp);
	}
}

void add_leige_to_lst(char_id_t id, INOUT(std::vector<char_id_t>) lst, IN(g_lock) lk) noexcept {
	const auto l = global::get_character_leige(id, lk);
	if (valid_ids(l) && std::find(lst.cbegin(), lst.cend(), l) == lst.cend()) {
		lst.push_back(l);
	}
}

const auto empty_test = [](char_id_t host, char_id_t target, IN(g_lock) l) noexcept { return true; };
const auto list_court = [](char_id_t host, char_id_t target, INOUT(cvector<char_id_t>) lst, IN(g_lock) l) noexcept {
	global::get_court(get_associated_admin(host, l), lst, l);
	vector_erase_if(lst, [host, target](char_id_t c) { return c == host || c == target; });
};
const auto list_realm = [](char_id_t host, char_id_t target, INOUT(cvector<char_id_t>) lst, IN(g_lock) l) noexcept {
	global::get_extended_court(host, lst, l);
	vector_erase_if(lst, [host, target](char_id_t c) { return c == host || c == target; });
};
const auto std_months = [](char_id_t host, char_id_t target, IN(g_lock) l) noexcept {
	const auto title = get_object(host).primetitle;
	IN(auto) to = get_object(title);
	if (!valid_ids(title) || to.type < EMPIRE_TYPE || to.type > COUNTY_TYPE) {
		return static_cast<unsigned char>(1);
	} else {
		return static_cast<unsigned char>(pow(2, (COUNTY_TYPE - to.type) + 1));
	}
};
const auto mass_mth_cost = [](char_id_t host, char_id_t target, IN(g_lock) l) noexcept {
	const auto title = get_object(host).primetitle;
	IN(auto) to = get_object(title);
	if (!valid_ids(title) || to.type < EMPIRE_TYPE || to.type > COUNTY_TYPE) {
		return static_cast<unsigned char>(1);
	} else {
		return static_cast<unsigned char>(2 * (COUNTY_TYPE - to.type) + 1);
	}
};
const auto targeted_mth_cost = [](char_id_t host, char_id_t target, IN(g_lock) l) noexcept {
	const auto title = get_object(host).primetitle;
	IN(auto) to = get_object(title);
	if (!valid_ids(title) || to.type < EMPIRE_TYPE || to.type > COUNTY_TYPE) {
		return static_cast<unsigned char>(1);
	} else {
		return static_cast<unsigned char>(1 * (COUNTY_TYPE - to.type) + 1);
	}
};

										//none neg  pos att1  ...                                        soci marti IQ   rel  cul
occurace_distribution basic_distribution{3.0, 0.5, 1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 1.0, 1.0};
occurace_distribution social_distribution{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0};
occurace_distribution martial_distribution{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0};
occurace_distribution analytic_distribution{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0};
occurace_distribution unlikely_distribution{29.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
#define MAX_OCCURACE 17

event_template basic_event(TX_G_EVENT, TX_ED_GEN_2, false, basic_distribution,
	empty_test, list_realm, std_months, mass_mth_cost);

event_template event_feast(TX_FEAST, TX_ED_GEN_1, false, basic_distribution + (social_distribution * 2.0), 
	empty_test, list_realm, std_months, mass_mth_cost);

event_template event_theater(TX_PERFORMANCE, TX_ED_GEN_1, false, basic_distribution + (analytic_distribution * 2.0), 
	empty_test, list_realm, std_months, mass_mth_cost);

event_template event_tournament(TX_TOURNAMENT, TX_ED_GEN_1, false, basic_distribution + (martial_distribution * 2.0), 
	empty_test, list_realm, std_months, mass_mth_cost);

event_template event_small_gathering(TX_GATHERING, TX_ED_GEN_2, true, basic_distribution + (social_distribution * 4.0),
	empty_test, list_court, std_months, targeted_mth_cost);

event_template event_hunt(TX_HUNT, TX_ED_GEN_2, true, basic_distribution + (martial_distribution * 4.0),
	empty_test, list_court, std_months, targeted_mth_cost);

event_template event_games(TX_GAMES, TX_ED_GEN_2, true, basic_distribution + (analytic_distribution * 4.0),
	empty_test, list_court, std_months, targeted_mth_cost);

event_template event_meeting(TX_MEETING, TX_ED_GEN_1, false, basic_distribution + unlikely_distribution,
	empty_test, list_court, std_months, targeted_mth_cost);

const event_template& event_template_by_id(unsigned int id) noexcept {
	switch (id) {
	case EVENT_HUNT: return event_hunt;
	case EVENT_FEAST: return event_feast;
	case EVENT_GAMES: return event_games;
	case EVENT_THEATER: return event_theater;
	case EVENT_TOURNAMENT: return event_tournament;
	case EVENT_SMALL_GATHERING: return event_small_gathering;
	case EVENT_MEETING: return event_meeting;
	default: return basic_event;
	}
}


bool occurance_test( unsigned short occurance, IN(untitled_data) a, IN(untitled_data) b, size_t eventvalue) noexcept {
	switch (occurance) {
	case 0:
	case 1:
	case 2:
		return true;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		if ((a.attrib.has_positive_N(occurance - 3) || a.attrib.has_negative_N(occurance - 3)) &&
			(b.attrib.has_positive_N(occurance - 3) || b.attrib.has_negative_N(occurance - 3))) {
			return true;
		}
		return false;
	case 12:
	{
		int adjusted = eventvalue % 10;
		return a.stats.social + adjusted >= 8 && b.stats.social + adjusted >= 8;
	}
	case 13:
	{
		int adjusted = eventvalue % 10;
		return a.stats.martial + adjusted >= 8 && b.stats.martial + adjusted >= 8;
	}
	case 14:
	{
		int adjusted = eventvalue % 10;
		return a.stats.analytic + adjusted >= 8 && b.stats.analytic + adjusted >= 8;
	}
	case 15: //rel
		return (a.religion != b.religion) && !(is_cynical(a) && is_cynical(b));
	case 16: //cul
		return a.culture != b.culture;
	default:
		return false;
	}
}

const std::vector<unsigned short> index_gen(counting_iterator<1, unsigned short>(0), counting_iterator<1, unsigned short>(MAX_OCCURACE));//0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};

unsigned short get_occurance(IN(event_template) ev, IN(untitled_data) a, IN(untitled_data) b, size_t eventval) noexcept {
	//const auto f = [&ed = ev.distribution, &a, &b, eventval](unsigned short indx) {
	//	if (occurance_test(indx, a, b, eventval))
	//		return ed[indx];
	//	else
	//		return 0.0;
	//};

	// using it_type = generating_iterator<double, unsigned short, decltype(f)>;

	double chances[MAX_OCCURACE] = {0.0};
	double total = 0.0;
	for (unsigned short i = 0; i != MAX_OCCURACE; ++i) {
		if (occurance_test(i, a, b, eventval))
			total += (chances[i] = ev.distribution[i]);
	}
	double chosen = global_store.get_fast_double() * total;
	for (unsigned short i = 1; i != MAX_OCCURACE; ++i) {
		if ((total -= chances[i]) < 0.0)
			return i;
	}
	return 0;

	//return static_cast<unsigned short>(global::applyDistribution(std::discrete_distribution<int>(it_type(f, index_gen.begin()), it_type(f, index_gen.end()))));
}

int create_occurance_text(IN(event_template) ev, unsigned short occurance, char_id_t a, char_id_t b, IN(untitled_data) ad, IN(untitled_data) bd, int x, int y, int width, IN(std::shared_ptr<uiElement>) parent) noexcept {
	std::vector<layoutelement> layout;

	switch (occurance) {
	case 1:
	{
		size_t params[3] = {a.value, b.value, ev.name};
		return create_tex_block(TX_EV_NEG_REL, params, 3, parent, x, y, width, global::empty, global::standard_text);
	}
	case 2:
	{
		size_t params[3] = {a.value, b.value, ev.name};
		return create_tex_block(TX_EV_POS_REL, params, 3, parent, x, y, width, global::empty, global::standard_text);
	}
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	{
		size_t params[3] = {a.value, b.value, static_cast<size_t>(occurance - 3)};
		if ((ad.attrib.has_positive_N(occurance - 3) && bd.attrib.has_positive_N(occurance - 3)) ||
			(ad.attrib.has_negative_N(occurance - 3) && bd.attrib.has_negative_N(occurance - 3))) {
			return create_tex_block(TX_EV_POS_TRAIT, params, 3, parent, x, y, width, global::empty, global::standard_text);
		} else {
			return create_tex_block(TX_EV_NEG_TRAIT, params, 3, parent, x, y, width, global::empty, global::standard_text);
		}
	}
	case 12:
	case 13:
	case 14:
	{
		size_t params[3] = {a.value, b.value, ev.name};
		return create_tex_block(TX_EV_ENJOYED, params, 3, parent, x, y, width, global::empty, global::standard_text);
	}
	case 15: //rel
	{
		size_t params[2] = {a.value, b.value};
		return create_tex_block(TX_EV_REL_DIF, params, 2, parent, x, y, width, global::empty, global::standard_text);
	}
	case 16: //cul
	{
		size_t params[2] = {a.value, b.value};
		return create_tex_block(TX_EV_CUL_DIF, params, 2, parent, x, y, width, global::empty, global::standard_text);
	}
	default:
		return 0;
	}
}

char relation_delta_by_occurance(unsigned short occurance) noexcept {
	switch (occurance) {
	case 1:
		return -1;
	case 2:
		return 1;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		return -2;
	case 12:
	case 13:
	case 14:
		return 2;
	case 15: //rel
		return -1;
	case 16: //cul
		return -1;
	default:
		return 0;
	}
}

void adjust_relations_by_occurance(IN(event_template) ev, unsigned short occurance, char_id_t a, char_id_t b,  IN(w_lock) l) noexcept {
	adjust_relation_symmetric(a, b, relation_delta_by_occurance(occurance), l);
}


void schedule_event(unsigned int id, char_id_t host, char_id_t target, IN(w_lock) l) noexcept {
	const auto mths = event_template_by_id(id).months_to_prep(host, target, l);
	const auto d = global::currentday + mths * 31;
	if (!valid_ids(target)) {
		global::schedule.emplace(d, new s_event(host, target, static_cast<unsigned short>(id)));
		add_expense(host, EXPENSE_EVENT, mths, event_template_by_id(id).monthly_cost(host, target, l), l);

		with_udata(host, l, [](INOUT(udata) d) noexcept { d.flags |= P_FLAG_PERPARING_EVENT; });
	} else {
		if (will_attend_event_date(target, host, id, d)) {
			global::schedule.emplace(d, new s_event(host, target, static_cast<unsigned short>(id)));
			add_expense(host, EXPENSE_EVENT, mths, event_template_by_id(id).monthly_cost(host, target, l), l);
			with_udata(host, l, [](INOUT(udata) d) noexcept { d.flags |= P_FLAG_PERPARING_EVENT; });
		} else if (host == global::playerid) {
			message_popup(get_simple_string(TX_L_INVITATION), [target](IN(std::shared_ptr<uiElement>) sv) {
				size_t param = target.value;
				create_tex_block(TX_INVITATION_REFUSED, &param, 1, sv, 0, 0, sv->pos.width, global::empty, global::standard_text);
			});
		}
	}
}

void execute_event(unsigned int id, char_id_t host, char_id_t target, IN(w_lock) l) noexcept {
	cvector<char_id_t> involved;

	IN(auto) ev = event_template_by_id(id);
	with_udata(host, l, [](INOUT(udata) d) noexcept { d.flags &= ~P_FLAG_PERPARING_EVENT; });

	ev.list_others(host, target, involved, l);
	const auto evalue = global_store.get_fast_int();
	for (size_t indx = involved.size() - 1; indx != SIZE_MAX; --indx) {
		if (!will_attend_event(involved[indx], host, id)) {
			involved[indx] = involved.back();
			involved.pop_back();
		} 
	}

	involved.push_back(host);
	if (valid_ids(target))
		involved.push_back(target);
	
	for (size_t oindex = 0; oindex < involved.size(); ++oindex) {
		char_id_t b = involved[oindex];

		with_udata(b, l, [oindex, &involved, &l, &ev, evalue, b, id](IN(udata) bd) noexcept {
			for (size_t iindex = oindex + 1; iindex < involved.size(); ++iindex) {
				char_id_t a = involved[iindex];

				with_udata(a, l, [oindex, &involved, &l, &ev, evalue, b, &bd, a, id](IN(udata) ad) noexcept {
					const auto occurance = get_occurance(ev, ad, bd, evalue);
					if (occurance > 0) {
						adjust_relations_by_occurance(event_template_by_id(id), occurance, a, b, l);

						if (global::interested.in_set(b.value) || global::interested.in_set(a.value)) {
							global::uiqueue.push([a, b, id, occurance] {
								r_lock l;
								with_udata_2(a, b, l, [a, b, id, occurance](IN(udata) iad, IN(udata) ibd) noexcept {
									message_popup(get_simple_string(TX_L_REL_CHANGE), [id, a, b, &ibd, &iad, occurance](IN(std::shared_ptr<uiElement>) sv) {
										create_occurance_text(event_template_by_id(id), occurance, a, b, iad, ibd, 0, 0, sv->pos.width - 10, sv);
									});
								});
							});
						}

					}
				});
			}
		});
	}
}

struct {
	std::shared_ptr<uiDragRect> window;
	std::shared_ptr<uiDropDown> event_selection;
	std::shared_ptr<uiSimpleText> cost;
	std::shared_ptr<uiSimpleText> time;
	std::shared_ptr<uiScrollView> decription;
	std::shared_ptr<uiScrollView> people_added;

	std::shared_ptr<uiButton> add_family;
	std::shared_ptr<uiButton> add_realm;
	std::shared_ptr<uiButton> add_recent;

	std::shared_ptr<ui_button_disable> execute_event;

	std::shared_ptr<uiMenuRect> addition_menu;
	std::shared_ptr<uiScrollView> menu_contents;

	char_id_t host;
	unsigned int event_type = EVENT_HUNT;
	char_id_t target;
} event_win;

#define EV_WIN_WIDTH 400
#define EV_WIN_HEIGHT 600

void InitEventWindow() noexcept {
	const sf::Color mback(200, 200, 200);

	const auto& ev = event_template_by_id(0);

	event_win.window = global::uicontainer->add_element<uiDragRect>(50, 50, EV_WIN_WIDTH, EV_WIN_HEIGHT, global::solid_border);
	event_win.window->setVisible(false);

	event_win.window->add_element<uiCenterdText>(0, 3, EV_WIN_WIDTH, get_simple_string(TX_L_HOST), global::empty, global::header_text);
	event_win.window->add_element<uiGButton>(EV_WIN_WIDTH - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) { event_win.window->setVisible(false); });
	
	event_win.event_selection = event_win.window->add_element<uiDropDown>(10, 25, 250, 20, global::solid_border, global::standard_text, [](uiDropDown* dd, unsigned int option) {
		event_win.event_type = option;
		SetupEventWindow(event_win.host);
	});
	for (size_t i = EVENT_HUNT; i != EVENT_SMALL_GATHERING; ++i) {
		event_win.event_selection->add_option(get_simple_string(event_template_by_id(static_cast<unsigned int>(i)).name), static_cast<int>(i));
	}

	event_win.cost = event_win.window->add_element<uiSimpleText>(270, 25, get_simple_string(TX_COST), global::empty, global::standard_text);
	event_win.time = event_win.window->add_element<uiSimpleText>(270, 50, get_simple_string(TX_E_TIME), global::empty, global::standard_text);

	event_win.window->subelements.push_back(event_win.decription = std::make_shared<uiScrollView>(10, 75, EV_WIN_WIDTH - 20, 40, event_win.window));
	event_win.window->subelements.push_back(event_win.people_added = std::make_shared<uiScrollView>(10, 125, EV_WIN_WIDTH - 20, EV_WIN_HEIGHT - 225, event_win.window));

	event_win.add_family = character_selection_menu(event_win.window, 10, EV_WIN_HEIGHT - 90, EV_WIN_WIDTH / 2 - 15, 20, get_simple_string(TX_ADD_FAMILY),
		global::solid_border, global::standard_text, [](cvector<char_id_t>& lst) {global::get_living_family(event_win.host, lst); },
		[](char_id_t c) {event_win.target = c; SetupEventWindow(event_win.host, false); });
	event_win.add_realm = character_selection_menu(event_win.window, EV_WIN_WIDTH / 2 + 5, EV_WIN_HEIGHT - 90, EV_WIN_WIDTH / 2 - 15, 20, get_simple_string(TX_ADD_REALM),
		global::solid_border, global::standard_text, [](cvector<char_id_t>& lst) {
		r_lock l;
		global::get_realm(event_win.host, lst, l); },
		[](char_id_t c) {event_win.target = c; SetupEventWindow(event_win.host, false); });
	event_win.add_recent = character_selection_menu(event_win.window, 10, EV_WIN_HEIGHT - 60, EV_WIN_WIDTH / 2 - 15, 20, get_simple_string(TX_ADD_RECENT),
		global::solid_border, global::standard_text, [](cvector<char_id_t>& lst) { global::get_recent_people(lst); },
		[](char_id_t c) {event_win.target = c; SetupEventWindow(event_win.host, false); });

	event_win.execute_event =  event_win.window->add_element<ui_button_disable>(EV_WIN_WIDTH / 2 - 100, EV_WIN_HEIGHT - 30, 200, 20, get_simple_string(TX_OPT_HOST), paint_states<2>(global::solid_border, global::disabled_fill), global::standard_text, global::tooltip_text, [](ui_button_disable* b) {
		global::uiqueue.push([] {
			event_win.window->setVisible(false);
			global::actionlist.add_new<planned_event>(event_win.host, event_win.target, event_win.event_type);
		});
	});

	event_win.addition_menu = std::make_shared<uiMenuRect>(global::uicontainer, 0, 0, 210, 400, global::solid_border);
	event_win.addition_menu->subelements.push_back(event_win.menu_contents = std::make_shared<uiScrollView>(0, 0, event_win.addition_menu->pos.width, event_win.addition_menu->pos.height, event_win.addition_menu));
}

void SetupEventWindow(char_id_t host, bool clear) noexcept {
	global::uiqueue.push([ host, clear] {
		event_win.addition_menu->close();
		event_win.host = host;

		const auto ev = event_template_by_id(event_win.event_type);
		if (clear) {
			event_win.target = 0;
			event_win.decription->subelements.clear();
			std::vector<layoutelement> layout;
			create_tex_block(ev.text,event_win.decription,0,0, event_win.decription->pos.width, global::empty, global::standard_text);
			//event_win.decription->add_element<uiTextBlock>(0, 0, event_win.decription->pos.width, layout, global::empty, global::standard_text, std::vector<std::string>{ev.text});
			event_win.decription->calcTotalHeight();
		}
		double cost = 0.0;
		unsigned char duration = 0;
		{
			r_lock l;
			cost = ev.monthly_cost(host, event_win.target, l);
			duration = ev.months_to_prep(host, event_win.target, l);
		}

		if (ev.targeted) {
			event_win.add_family->setVisible(true);
			event_win.add_realm->setVisible(true);
			event_win.add_recent->setVisible(true);
		} else {
			event_win.target = 0;
			event_win.add_family->setVisible(false);
			event_win.add_realm->setVisible(false);
			event_win.add_recent->setVisible(false);
		}

		event_win.cost->updateText(get_simple_string(TX_E_COST) + format_double(cost * duration));
		event_win.time->updateText(get_simple_string(TX_E_TIME) + format_double(duration));

		event_win.execute_event->enable();

		{
			r_lock l;
			if (global::get_wealth(host, l) <= 0.0)
				event_win.execute_event->disable(get_simple_string(TX_NO_MONEY));
			with_udata(host, l, [](IN(udata) d) noexcept {
				if ((d.flags & P_FLAG_PERPARING_EVENT) != 0)
					event_win.execute_event->disable(get_simple_string(TX_ALREADY_PREP));
			});
		}
		event_win.people_added->subelements.clear();

		int yoff = 5;
		if(valid_ids(event_win.target)) {
			const auto f = event_win.people_added->add_element<uiHLink>(5, yoff, global::w_character_name(event_win.target), global::empty, global::standard_text, global::whandle, [ch = event_win.target](uiHLink* h) {SetupChPane( ch); });
			const auto b = event_win.people_added->add_element<uiButton>(event_win.people_added->pos.width - 35, yoff, 20, f->pos.height, TEXT("X"), global::solid_border, text_format{14, global::font, sf::Color::Red},
				[](uiButton* b) {
				event_win.target = 0;
				SetupEventWindow( event_win.host, false);
			});
			yoff += (global::standard_text.csize + 4);
		}

		event_win.people_added->calcTotalHeight();

		if(clear)
			event_win.window->toFront(global::uicontainer);
		event_win.window->setVisible(true);
	});

}