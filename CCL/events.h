#pragma once
#include "globalhelpers.h"
#include <random>
#include <valarray> 
#include "uielements.hpp"

using occurace_distribution = std::valarray<double>;

class event_template {
public:
	const std::function<bool(char_id_t, char_id_t, const g_lock&)> test;
	const std::function<void(char_id_t, char_id_t, cvector<char_id_t>&, const g_lock&)> list_others;
	const std::function<unsigned char(char_id_t, char_id_t, const g_lock&)> months_to_prep;
	const std::function<double(char_id_t, char_id_t, const g_lock&)> monthly_cost;
	bool targeted;
	const size_t name;
	const size_t text;
	//const std::discrete_distribution<int> chances;
	const occurace_distribution distribution;

	template<typename T, typename T2, typename T3, typename T4>
	event_template(size_t n, size_t t, bool tar, IN(occurace_distribution) dist, T&& f, T2&& flist, T3&& prep, T4&& cost) :
		distribution(dist), name(n), text(t), months_to_prep(std::forward<T3>(prep)), monthly_cost(std::forward<T4>(cost)), targeted(tar),
		test(std::forward<T>(f)), list_others(std::forward<T2>(flist)) {} //, chances(std::begin(dist), std::end(dist)) {}
};

#define EVENT_HUNT 1
#define EVENT_FEAST 2
#define EVENT_GAMES 3
#define EVENT_THEATER 4
#define EVENT_TOURNAMENT 5
#define EVENT_SMALL_GATHERING 6
#define EVENT_WEDDING 7
#define EVENT_FUNERAL 8
#define EVENT_MEETING 9

#define DAYS_BT_EVENTS 45

struct untitled_data;
const event_template& event_template_by_id(unsigned int id) noexcept;
void execute_event(unsigned int id, char_id_t host, char_id_t target, IN(w_lock) l) noexcept;
void schedule_event(unsigned int id, char_id_t host, char_id_t target, IN(w_lock) l) noexcept;
char relation_delta_by_occurance(unsigned short occurance) noexcept;
unsigned short get_occurance(IN(event_template) ev, IN(untitled_data) a, IN(untitled_data) b, size_t eventval) noexcept;
int create_occurance_text(IN(event_template) ev, unsigned short occurance, char_id_t a, char_id_t b, IN(untitled_data) ad, IN(untitled_data) bd, int x, int y, int width, IN(std::shared_ptr<uiElement>) parent) noexcept;

void InitEventWindow() noexcept;
void SetupEventWindow(char_id_t host, bool clear = true) noexcept;