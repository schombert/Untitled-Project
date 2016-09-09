#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"

#define MAX_TRAITS 9

void load_trait_icons() noexcept;
std::shared_ptr<ui_tt_icon> get_pos_icon(unsigned int number, int x, int y, IN(std::shared_ptr<uiElement>) parent) noexcept;
std::shared_ptr<ui_tt_icon> get_neg_icon(unsigned int number, int x, int y, IN(std::shared_ptr<uiElement>) parent) noexcept;
std::wstring get_pos_name(unsigned int number) noexcept;
std::wstring get_neg_name(unsigned int number) noexcept;
void unload_trait_icons() noexcept;

struct untitled_data;
bool is_peaceful(IN(untitled_data) d) noexcept;
bool is_kind(IN(untitled_data) d) noexcept;
bool is_honest(IN(untitled_data) d) noexcept;
bool is_extrovert(IN(untitled_data) d) noexcept;
bool is_devout(IN(untitled_data) d) noexcept;
bool is_measured(IN(untitled_data) d) noexcept;
bool is_just(IN(untitled_data) d) noexcept;
bool is_decadent(IN(untitled_data) d) noexcept;
bool is_cautious(IN(untitled_data) d) noexcept;

bool is_aggressive(IN(untitled_data) d) noexcept;
bool is_cruel(IN(untitled_data) d) noexcept;
bool is_deceitful(IN(untitled_data) d) noexcept;
bool is_introvert(IN(untitled_data) d) noexcept;
bool is_cynical(IN(untitled_data) d) noexcept;
bool is_emotional(IN(untitled_data) d) noexcept;
bool is_arbitrary(IN(untitled_data) d) noexcept;
bool is_miserly(IN(untitled_data) d) noexcept;
bool is_reckless(IN(untitled_data) d) noexcept;

#define TRAIT_PEACEFUL		0
#define TRAIT_KIND			1
#define TRAIT_HONEST		2
#define TRAIT_EXTROVERT		3
#define TRAIT_DEVOUT		4
#define TRAIT_MEASURED		5
#define TRAIT_JUST			6
#define TRAIT_DECADENT		7
#define TRAIT_CAUTIOUS		8

#define TRAIT_AGGRESSIVE	0
#define TRAIT_CRUEL			1
#define TRAIT_DECEITFUL		2
#define TRAIT_INTROVERT		3
#define TRAIT_CYNICAL		4
#define TRAIT_EMOTIONAL		5
#define TRAIT_ARBITRARY		6
#define TRAIT_MISERLY		7
#define TRAIT_RECKLESS		8

int similarity_score(char_id_t a, char_id_t b, IN(g_lock) l) noexcept;

double adjust_by_feeling(IN(g_lock) l, char_id_t actor, char_id_t target, int feeling, double magnitude = 1.0) noexcept;
double adjust_by_feeling(IN(untitled_data) d, char_id_t actor, char_id_t target, int feeling, double magnitude = 1.0) noexcept;

double value_of_honor(double change, IN(untitled_data) d);
double value_of_justice(double change, IN(untitled_data) d);
double value_of_peacefulness(double change, IN(untitled_data) d);