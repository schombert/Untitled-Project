#include "globalhelpers.h"
#include "traits.h"
#include "structs.hpp"
#include "i18n.h"
#include "living_data.h"

texture_rect traits[MAX_TRAITS*2];
texture_rect peaceful;
std::vector<sf::Texture> t;

void unload_trait_icons() noexcept {
	t.clear();
}

void load_trait_icons() noexcept {
	t.resize(MAX_TRAITS*2);

	LoadDDS(t[0], "gfx\\traits\\duelist.dds");
	traits[0].init(&t[0]);
	LoadDDS(t[1], "gfx\\traits\\impaler.dds");
	traits[1].init(&t[1]);
	LoadDDS(t[2], "gfx\\traits\\in_hiding.dds");
	traits[2].init(&t[2]);
	LoadDDS(t[3], "gfx\\traits\\shy.dds");
	traits[3].init(&t[3]);
	LoadDDS(t[4], "gfx\\traits\\cynical.dds");
	traits[4].init(&t[4]);
	LoadDDS(t[5], "gfx\\traits\\lustful.dds");
	traits[5].init(&t[5]);
	LoadDDS(t[6], "gfx\\traits\\arbitrary.dds");
	traits[6].init(&t[6]);
	LoadDDS(t[7], "gfx\\traits\\greedy.dds");
	traits[7].init(&t[7]);
	LoadDDS(t[8], "gfx\\traits\\brave.dds");
	traits[8].init(&t[8]);
	
	LoadDDS(t[MAX_TRAITS], "gfx\\traits\\patient.dds");
	traits[MAX_TRAITS].init(&t[MAX_TRAITS]);
	LoadDDS(t[MAX_TRAITS+1], "gfx\\traits\\kind.dds");
	traits[MAX_TRAITS + 1].init(&t[MAX_TRAITS + 1]);
	LoadDDS(t[MAX_TRAITS + 2], "gfx\\traits\\honest.dds");
	traits[MAX_TRAITS + 2].init(&t[MAX_TRAITS + 2]);
	LoadDDS(t[MAX_TRAITS + 3], "gfx\\traits\\gregarious.dds");
	traits[MAX_TRAITS + 3].init(&t[MAX_TRAITS +3]);
	LoadDDS(t[MAX_TRAITS + 4], "gfx\\traits\\theologian.dds");
	traits[MAX_TRAITS + 4].init(&t[MAX_TRAITS + 4]);
	LoadDDS(t[MAX_TRAITS + 5], "gfx\\traits\\game_master.dds");
	traits[MAX_TRAITS + 5].init(&t[MAX_TRAITS + 5]);
	LoadDDS(t[MAX_TRAITS + 6], "gfx\\traits\\just.dds");
	traits[MAX_TRAITS + 6].init(&t[MAX_TRAITS + 6]);
	LoadDDS(t[MAX_TRAITS + 7], "gfx\\traits\\decadent.dds");
	traits[MAX_TRAITS + 7].init(&t[MAX_TRAITS + 7]);
	LoadDDS(t[MAX_TRAITS + 8], "gfx\\traits\\craven.dds");
	traits[MAX_TRAITS + 8].init(&t[MAX_TRAITS + 8]);
	
}

std::shared_ptr<ui_tt_icon> get_pos_icon(unsigned int number, int x, int y, IN(std::shared_ptr<uiElement>) parent) noexcept {
	return parent->add_element<ui_tt_icon>(x, y, 24, 24, traits[MAX_TRAITS +number], get_pos_name(number), global::tooltip_text);
}

std::shared_ptr<ui_tt_icon> get_neg_icon(unsigned int number, int x, int y, IN(std::shared_ptr<uiElement>) parent) noexcept {
	return parent->add_element<ui_tt_icon>(x, y, 24, 24, traits[number], get_neg_name(number), global::tooltip_text);
}

std::wstring get_pos_name(unsigned int number) noexcept {
	size_t param = number;
	return get_p_string(TX_POS_TRAIT, &param, 1);
}

std::wstring get_neg_name(unsigned int number) noexcept {
	size_t param = number;
	return get_p_string(TX_NEG_TRAIT, &param, 1);
}


double value_of_honor(double change, IN(untitled_data) d) {
	return (d.attrib.has_positive_N<TRAIT_HONEST>() ? 1.0 : 0.5) * (d.attrib.has_negative_N<TRAIT_HONEST>() ? 0.5 : 1.0) * change;
}

double value_of_justice(double change, IN(untitled_data) d) {
	return (d.attrib.has_positive_N<TRAIT_JUST>() ? 1.0 : 0.5) * (d.attrib.has_negative_N<TRAIT_JUST>() ? 0.5 : 1.0) * change;
}

double value_of_peacefulness(double change, IN(untitled_data) d) {
	return (d.attrib.has_positive_N<TRAIT_PEACEFUL>() ? 1.0 : 0.5) * (d.attrib.has_negative_N<TRAIT_PEACEFUL>() ? 0.5 : 1.0) * change;
}

bool is_peaceful(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<0>();
}
bool is_kind(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<1>();
}
bool is_honest(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<2>();
}
bool is_extrovert(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<3>();
}
bool is_devout(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<4>();
}
bool is_measured(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<5>();
}
bool is_just(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<6>();
}
bool is_decadent(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<7>();
}
bool is_cautious(IN(untitled_data) d) noexcept {
	return d.attrib.has_positive_N<8>();
}

bool is_aggressive(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<0>();
}
bool is_cruel(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<1>();
}
bool is_deceitful(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<2>();
}
bool is_introvert(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<3>();
}
bool is_cynical(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<4>();
}
bool is_emotional(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<5>();
}
bool is_arbitrary(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<6>();
}
bool is_miserly(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<7>();
}
bool is_reckless(IN(untitled_data) d) noexcept {
	return d.attrib.has_negative_N<8>();
}

int similarity_score(char_id_t a, char_id_t b, IN(g_lock) l) noexcept {
	return with_udata_2(a, b, l, 0, [](IN(udata) ad, IN(udata) bd) noexcept {
		return ad.attrib.compare<MAX_TRAITS>(bd.attrib);
	});
}

double adjust_by_feeling(IN(untitled_data) d, char_id_t actor, char_id_t target, int feeling, double magnitude) noexcept {
	if (is_emotional(d)) magnitude *= 2.0;
	else if (is_measured(d))  magnitude *= 0.5;
	
	if (feeling == -1) {
		return  -magnitude;
	} else if (feeling == 1) {
		return magnitude;
	} else if (get_object(actor).dynasty == get_object(target).dynasty && valid_ids(get_object(actor).dynasty)) {
		return 0.5 * magnitude;
	}
	return 0.0;
}

double adjust_by_feeling(IN(g_lock) l, char_id_t actor, char_id_t target, int feeling, double magnitude) noexcept {
	return with_udata(actor, l, 0.0, [actor, target, feeling, magnitude](IN(udata) d) noexcept {
		return adjust_by_feeling(d, actor, target, feeling, magnitude);
	});
}
