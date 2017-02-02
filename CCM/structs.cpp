#include "globalhelpers.h"
#include "structs.h"

using namespace std;

std::string master_string_data;

namespace detail {

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

	//reader_writer_lock uilock;
	event quitevent;
	event end_of_day;

	concurrency::task_group uiTasks;


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

	texture_rect back_tex;
	texture_rect close_tex;

	prov_id_t focused;
	bool paused = true;
	int speed = 1;
	unsigned int currentday = 0;
	int mapmode = 0;

	double horzrotation = -0.4;
	double vertrotation = -0.6;

	_map_d map;

	std::vector<sf::Sprite> mapsprites;
	concurrency::concurrent_queue<std::function<void()>> uiqueue;
	flat_map<prov_id_t, std::string> provtooltip;
};

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
