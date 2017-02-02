#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"

class actionbase;
class s_actionbase;

struct border {
	// std::vector<sf::Vector2<short>> borderline;
	GLuint vertexbuffer = 0;
	GLsizei numvertex = 0;
	int type = -1;
};

#define PROV_FLAG_WATER			0x01
#define PROV_FLAG_COASTAL		0x02
// #define PROV_FLAG_CONTROLLED	0x04
#define PROV_FLAG_HIHGLIGHT		0x08
#define PROV_FLAG_HASTITLE		0x10

#define P_IS_WATER(x) ((x & PROV_FLAG_WATER) != 0)
// #define P_IS_CONTROLLED(x) ((x & PROV_FLAG_CONTROLLED) != 0)
#define P_HAS_TITLE(x) (x != 0 && x <= province::last_titled_p)
#define P_GET_TITLE(x) (P_HAS_TITLE(x) ? x : 0)

extern std::string master_string_data;
using sref = string_ref_t<std::string, &master_string_data>;

struct province_display {
	GLsizei numvertex = 0;
	GLuint vertexbuffer = 0;

	GLfloat red;
	GLfloat blue;
	GLfloat green;
};

#define FLG_MAP_UPDATE 0x00000001
#define FLG_WWIN_UPDATE 0x00000002
#define FLG_BORDER_UPDATE 0x00000004
#define FLG_PANEL_UPDATE 0x00000008
#define FLG_DATE_UPDATE 0x00000010
#define FLG_MISSIONS_UPDATE 0x00000020

#define MAX_PLAYER_STRESS 100

namespace detail {

}

namespace global {
	extern sf::RenderWindow* window;
	extern LONG flags;

	extern HWND whandle;


	extern event quitevent;
	extern event end_of_day;


	bool adjacent(prov_id a, prov_id b);

	void setWindow(sf::RenderWindow* w) noexcept;
	OGLLock lockWindow();

	extern concurrency::task_group uiTasks;

	bool testFlagAndReset(LONG flag);
	void setFlag(LONG flag);

	__int64 randint();
	double randdouble();
	void clear_highlight() noexcept;
	void HideAll();

	void enterPane(int pane, sqlite_int64 focused);

	template<typename T>
	auto applyDistribution(T&& d) -> decltype(d(global_store.get_generator())) {
		return d(global_store.get_generator());
	}

	extern std::shared_ptr<uiElement> uicontainer;
	extern std::shared_ptr<uiElement> overlay;

	extern sf::Font* font;
	extern sf::Font* gothic_font;

	extern text_format standard_text;
	extern text_format header_text;
	extern text_format tooltip_text;
	extern text_format large_tooltip_text;

	extern no_fill empty;
	extern paint_rect solid;
	extern paint_rect solid_border;
	extern paint_rect solid_black;
	extern paint_rect disabled_fill;
	extern paint_rect bar_fill;

	
	extern texture_rect back_tex;
	extern texture_rect close_tex;


	extern prov_id_t focused;
	extern bool paused;
	extern int speed;
	extern unsigned int currentday;
	extern int mapmode;

	extern double horzrotation;
	extern double vertrotation;
	
	union _map_d {
		struct {
			war_id_t wid;
			admin_id_t adm;
		} displayedwar;
		rel_id_t displayedrel;
		cul_id_t displayedcul;
		prov_id_t displayedprov;

		_map_d() {
		}
	};

	extern _map_d map;

	extern std::vector<sf::Sprite> mapsprites;

	extern concurrency::concurrent_queue<std::function<void()>> uiqueue;

	extern flat_map<prov_id_t, std::string> provtooltip;
};


#define M_WIDTH 650