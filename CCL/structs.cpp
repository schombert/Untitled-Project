#include "globalhelpers.h"
#include "structs.hpp"
#include <string>
#include "TPane.h"
#include "ChPane.h"
#include "ProvPane.h"
#include "DynPane.h"
#include "CulPane.h"
#include "RelPane.h"
#include "schedule.h"
#include "actions.h"

using namespace std;

size_t province::last_titled_p = 0;

std::string master_string_data;

namespace detail {
	std::vector<dynasty> dynasties;
	std::vector<culture> cultures;
	std::vector<religion> religions;
	std::vector<province> provinces;
	std::vector<title> titles;
	std::vector<person> people;
}

namespace global {
	sf::RenderWindow* window;
	LONG flags = 0;

	HWND whandle;

	multiindex<char_id_t, title_id_t> holdertotitle;

	flat_multimap<edge, border> borders;
	std::vector<province_display> prov_display;

	v_vector_t<prov_id, prov_id> province_connections;
	std::shared_ptr<uiElement> uicontainer = std::make_shared<uiElement>(0, 0, 800, 600, std::weak_ptr<uiElement>());
	std::shared_ptr<uiElement> overlay = std::make_shared<uiElement>(0, 0, 800, 600, std::weak_ptr<uiElement>());

	std::mt19937_64 randnumbers = std::mt19937_64(std::random_device()());

	//reader_writer_lock uilock;
	event quitevent;
	event end_of_day;

	concurrency::task_group uiTasks;

	std::shared_ptr<uiDragRect> infowindows;

	sf::Font* font;
	sf::Font* gothic_font;

	text_format standard_text;
	text_format header_text;
	text_format tooltip_text;
	text_format large_tooltip_text;

	no_fill empty;
	paint_rect solid;
	paint_rect solid_border;
	paint_rect solid_black;
	paint_rect disabled_fill;
	paint_rect bar_fill;

	texture_array<40, 40, 2, 1> takeaction_tex;
	texture_array<28, 28, 2, 1> lock_tex;
	texture_array<32, 32, 2, 1> messages_button_tex;
	texture_rect back_tex;
	texture_rect close_tex;
	texture_rect sword_icon;
	texture_array<28, 28, 2, 1> arrows_tex;
	texture_array<24, 24, 4, 1> checkbox;
	texture_array<32, 32, 34, 1> iconstrip;

	texture_array<64, 64, 6, 5> bigcrowns_tex;
	texture_array<40, 40, 6, 5> smallcrowns_tex;


	std::list<pair<int, sqlite_int64>> mhistory;

	prov_id_t focused;
	bool paused = true;
	int speed = 1;
	unsigned int currentday = 0;
	int mapmode = 0;

	double horzrotation = -0.4;
	double vertrotation = -0.6;

	_map_d map;

	short playerstress = 0;
	char_id_t playerid;
	interested_vector interested;
	std::vector<sf::Sprite> mapsprites;

	flat_multimap<unsigned int, std::unique_ptr<s_actionbase>> schedule;
	//concurrency::concurrent_queue<actionbase*> actionlist;
	//SList<actionbase*> actionlist;
	concurrency::concurrent_queue<std::function<void()>> uiqueue;

	flat_map<prov_id_t, std::string> provtooltip;

};

std::string convertName(IN(std::string) input) {
	bool cap = true;
	unsigned int start = 2;
	std::string output;
	while (start < input.size()) {
		if (input[start] == '_') {
			cap = true;
			output += ' ';
		}
		else {
			if (cap) {
				output += static_cast<char>(toupper(input[start]));
				cap = false;
			}
			else {
				output += input[start];
			}
		}
		start++;
	}

	return output;
}

bool global::adjacent(prov_id a, prov_id b) {
	return borders.count(edge(a, b)) != 0;
}

void global::setWindow(sf::RenderWindow * w) noexcept {
	window = w;
	whandle = w->getSystemHandle();
}

OGLLock global::lockWindow() {
	return OGLLock(window);
}

bool global::testFlagAndReset(LONG flag) {
	return (InterlockedAnd(&flags, ~flag) & flag) != 0;
}

void global::setFlag(LONG flag) {
	InterlockedOr(&flags, flag);
}


double global::randdouble() {
	return global_store.get_double();
}

__int64 global::randint() {
	return global_store.get_int();
}

void global::clear_highlight() noexcept {
	for (auto& p : detail::provinces)
		p.pflags &= ~PROV_FLAG_HIHGLIGHT;
}

void global::HideAll() {
	//hide_pp();
	hide_chm();
	hide_tm();
	hide_dm();
	hide_cm();
	hide_rm();
}

void global::enterPane(int pane, sqlite_int64 f) {
	if (mhistory.size() > 50) {
		mhistory.pop_front();
	}
	mhistory.push_back(pair<int, sqlite_int64>(pane, f));
	infowindows->toFront(uicontainer);
}


void global::registerCharUpdate(char_id id) {
	if (mhistory.size() > 0 && mhistory.back().first == 2 && mhistory.back().second == id) {
		setFlag(FLG_PANEL_UPDATE);
	}
}

void global::registerTitleUpdate(title_id id) {
	if (mhistory.size() > 0 && mhistory.back().first == 3 && mhistory.back().second == id) {
		setFlag(FLG_PANEL_UPDATE);
	}
}

void global::registerProvinceUpdate(prov_id id) {
	if (mhistory.size() > 0 && mhistory.back().first == 1 && mhistory.back().second == id) {
		setFlag(FLG_PANEL_UPDATE);
	}
}

void global::flag_prov_update(prov_id id) {
	uiqueue.push([id] {
		if (global::mhistory.size() > 0) {
			pair<int, sqlite_int64> gotop = global::mhistory.back();
			if (gotop.first == 1 && static_cast<prov_id>(gotop.second) == id) {
				global::setFlag(FLG_PANEL_UPDATE);
			}
		}
	});
}

void global::flag_title_update(title_id id) {
	uiqueue.push([id] {
		if (global::mhistory.size() > 0) {
			pair<int, sqlite_int64> gotop = global::mhistory.back();
			if (gotop.first == 3 && static_cast<title_id>(gotop.second) == id) {
				global::setFlag(FLG_PANEL_UPDATE);
			}
		}
	});
}

void global::flag_ch_update(char_id id) {
	uiqueue.push([id] {
		if (global::mhistory.size() > 0) {
			pair<int, sqlite_int64> gotop = global::mhistory.back();
			if (gotop.first == 2 && static_cast<char_id>(gotop.second) == id) {
				global::setFlag(FLG_PANEL_UPDATE);
			}
		}
	});
}
