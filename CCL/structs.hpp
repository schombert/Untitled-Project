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

class province {
public:
	//std::vector<prov_id> connections;
	static constexpr unsigned char FLAG_WATER = 0x01;
	static constexpr unsigned char FLAG_COASTAL = 0x02;
	// static constexpr unsigned char FLAG_CONTROLLED = 0x04;
	static constexpr unsigned char FLAG_HIHGLIGHT = 0x08;
	static constexpr unsigned char FLAG_HASTITLE = 0x10;

	static constexpr prov_id NONE = 0;

	static size_t last_titled_p;

	sref name;

	sf::IntRect bounds;
	v_vector<std::pair<int, int>> intersect;
	glm::vec3 centroid;

	double tax = 0.0;
	unsigned short terrain[14];

	cul_id_t culture;
	rel_id_t religion;

	unsigned char climate = 4;
	unsigned char pflags = 0; // FLAG_CONTROLLED;

	province() {
		memset(terrain, 0, sizeof(unsigned short) * 14);
	}

	double dsq(IN(province) other) const { // less = closer
		//const float xdif = ((bounds.left + bounds.width / 2.0f) - (other.bounds.left + other.bounds.width / 2.0f));
		//const float ydif = ((bounds.top + bounds.height / 2.0f) - (other.bounds.top + other.bounds.height / 2.0f));
		//return xdif*xdif + ydif*ydif;
		return -glm::dot(centroid, other.centroid);
	}
};

class religion {
public:
	static constexpr rel_id NONE = 0;

	sref group;
	sref name;
};

class culture {
public:
	static constexpr rel_id NONE = 0;

	sref group;
	sref name;
	sref prefix;
	std::vector<sref> mnames;
	std::vector<sref> fnames;
};

struct statstruct {
	unsigned char analytic;
	unsigned char martial;
	unsigned char social;
	unsigned char intrigue;
};

struct title_stats {
	unsigned char i_analytic;
	unsigned char i_martial;
	unsigned char i_social;
	unsigned char i_intrigue;

	//float analytic;
	//float martial;
	//float social;
	//float intrigue;

	static constexpr double unpack(unsigned char v) noexcept {
		return static_cast<double>(v) / 32.0;
	}
	static constexpr float unpackf(unsigned char v) noexcept {
		return static_cast<float>(v) / 32.0f;
	}

	static constexpr unsigned char pack(double v) noexcept {
		return static_cast<unsigned char>(v * 32.0);
	}

	void clear() noexcept {
		i_analytic = i_martial = i_social = i_intrigue = 0;
	}
	double get_analytic() const noexcept {
		return unpack(i_analytic);
	}
	double get_martial() const noexcept {
		return unpack(i_martial);
	}
	double get_social() const noexcept {
		return unpack(i_social);
	}
	double get_intrigue() const noexcept {
		return unpack(i_intrigue);
	}
	float get_intrigue_f() const noexcept {
		return unpackf(i_intrigue);
	}

	void add(const statstruct& i, double factor, double base_adj = 0.0) {
		i_analytic = std::max(i_analytic, pack(std::max(0.0, (i.analytic + base_adj) * factor)));
		i_martial = std::max(i_martial, pack(std::max(0.0, (i.martial + base_adj) * factor)));
		i_social = std::max(i_social, pack(std::max(0.0, (i.social + base_adj) * factor)));
		i_intrigue = std::max(i_intrigue, pack(std::max(0.0, (i.intrigue + base_adj) * factor)));
	}

	__int64 save() const {
		return (static_cast<__int64>(i_analytic) << 12i64) + (static_cast<__int64>(i_martial) << 8i64) +
			(static_cast<__int64>(i_social) << 4i64) + (static_cast<__int64>(i_intrigue));
	}

	void load(__int64 val) {
		i_intrigue = static_cast<unsigned char>(0x0F & val);
		val >>= 4;
		i_social = static_cast<unsigned char>(0x0F & val);
		val >>= 4;
		i_martial = static_cast<unsigned char>(0x0F & val);
		val >>= 4;
		i_analytic = static_cast<unsigned char>(0x0F & val);
	}
};

class title {
public:
	static constexpr title_id NONE = 0;

	sref rname;
	sref adj;

	char_id_t holder;
	sf::Color color1;

	admin_id_t associated_admin;

	static constexpr unsigned __int8 EMPIRE = 1;
	static constexpr unsigned __int8 KINGDOM = 2;
	static constexpr unsigned __int8 DUTCHY = 3;
	static constexpr unsigned __int8 COUNTY = 4;
	
	unsigned __int8 type;
};

class dynasty {
public:
	static constexpr dyn_id NONE = 0;

	sref name;
	cul_id_t culture;
};

class person {
public:
	static constexpr char_id NONE = 0;

	sref name;
	
	char_id_t father;
	char_id_t mother;

	unsigned int born = 0;
	unsigned int died = 0;
	char_id_t spouse;

	unsigned int utdata = max_value<unsigned int>::value;
	
	title_id_t primetitle;
	dyn_id_t dynasty;

	unsigned __int8 gender = 0;

	static constexpr unsigned __int8 MALE = 0;
	static constexpr unsigned __int8 FEMALE = 1;
	static constexpr unsigned __int8 NEUTER = 2;

	//std::unique_ptr<ai_blobs> blb;
};


#define FLG_MAP_UPDATE 0x00000001
#define FLG_WWIN_UPDATE 0x00000002
#define FLG_BORDER_UPDATE 0x00000004
#define FLG_PANEL_UPDATE 0x00000008
#define FLG_DATE_UPDATE 0x00000010
#define FLG_MISSIONS_UPDATE 0x00000020

#define MAX_PLAYER_STRESS 100

namespace detail {
	extern std::vector<dynasty> dynasties;
	extern std::vector<culture> cultures;
	extern std::vector<religion> religions;
	extern std::vector<province> provinces;
	extern std::vector<title> titles;
	extern std::vector<person> people;
}

inline const person& get_object(char_id_t id, IN(g_lock) l) noexcept {
	return detail::people[id.value];
}
inline person& get_object(char_id_t id, IN(w_lock) l) noexcept {
	return detail::people[id.value];
}
inline person& get_object(char_id_t id) noexcept {
	return detail::people[id.value];
}
inline char_id_t get_id(IN(person) obj, IN(g_lock) l) noexcept {
	return char_id_t(static_cast<char_id>(std::distance(&detail::people[0], const_cast<person*>(&obj))));
}

inline const title& get_object(title_id_t id, IN(g_lock) l) noexcept {
	return detail::titles[id.value];
}
inline title& get_object(title_id_t id, IN(w_lock) l) noexcept {
	return detail::titles[id.value];
}
inline title& get_object(title_id_t id) noexcept {
	return detail::titles[id.value];
}
inline title_id_t get_id(IN(title) obj, IN(g_lock) l) noexcept {
	return title_id_t(static_cast<title_id>(std::distance(&detail::titles[0], const_cast<title*>(&obj))));
}

inline const province& get_object(prov_id_t id, IN(g_lock) l) noexcept {
	return detail::provinces[id.value];
}
inline province& get_object(prov_id_t id, IN(w_lock) l) noexcept {
	return detail::provinces[id.value];
}
inline province& get_object(prov_id_t id) noexcept {
	return detail::provinces[id.value];
}
inline prov_id_t get_id(IN(province) obj, IN(g_lock) l) noexcept {
	return prov_id_t(static_cast<prov_id>(std::distance(&detail::provinces[0], const_cast<province*>(&obj))));
}

inline const dynasty& get_object(dyn_id_t id, IN(g_lock) l) noexcept {
	return detail::dynasties[id.value];
}
inline dynasty& get_object(dyn_id_t id, IN(w_lock) l) noexcept {
	return detail::dynasties[id.value];
}
inline dynasty& get_object(dyn_id_t id) noexcept {
	return detail::dynasties[id.value];
}
inline dyn_id_t get_id(IN(dynasty) obj, IN(g_lock) l) noexcept {
	return dyn_id_t(static_cast<dyn_id>(std::distance(&detail::dynasties[0], const_cast<dynasty*>(&obj))));
}

inline const culture& get_object(cul_id_t id, IN(g_lock) l) noexcept {
	return detail::cultures[id.value];
}
inline culture& get_object(cul_id_t id, IN(w_lock) l) noexcept {
	return detail::cultures[id.value];
}
inline culture& get_object(cul_id_t id) noexcept {
	return detail::cultures[id.value];
}
inline cul_id_t get_id(IN(culture) obj, IN(g_lock) l) noexcept {
	return cul_id_t(static_cast<cul_id>(std::distance(&detail::cultures[0], const_cast<culture*>(&obj))));
}

inline const religion& get_object(rel_id_t id, IN(g_lock) l) noexcept {
	return detail::religions[id.value];
}
inline religion& get_object(rel_id_t id, IN(w_lock) l) noexcept {
	return detail::religions[id.value];
}
inline religion& get_object(rel_id_t id) noexcept {
	return detail::religions[id.value];
}
inline rel_id_t get_id(IN(religion) obj, IN(g_lock) l) noexcept {
	return rel_id_t(static_cast<rel_id>(std::distance(&detail::religions[0], const_cast<religion*>(&obj))));
}

namespace global {
	extern sf::RenderWindow* window;
	extern LONG flags;

	extern HWND whandle;

	extern multiindex<char_id_t, title_id_t> holdertotitle;

	extern flat_multimap<edge, border> borders;
	extern std::vector<province_display> prov_display;

	extern v_vector_t<prov_id, prov_id> province_connections;
	extern std::shared_ptr<uiElement> uicontainer;
	extern std::shared_ptr<uiElement> overlay;

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

	extern std::shared_ptr<uiDragRect> infowindows;

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

	extern texture_array<40, 40, 2, 1> takeaction_tex;
	extern texture_array<28, 28, 2, 1> lock_tex;
	extern texture_array<32, 32, 2, 1> messages_button_tex;
	extern texture_rect back_tex;
	extern texture_rect close_tex;
	extern texture_rect sword_icon;
	extern texture_array<28, 28, 2, 1> arrows_tex;
	extern texture_array<24, 24, 4, 1> checkbox;
	extern texture_array<32, 32, 34, 1> iconstrip;

	extern texture_array<64, 64, 6, 5> bigcrowns_tex;
	extern texture_array<40, 40, 6, 5> smallcrowns_tex;


	extern std::list<std::pair<int, sqlite_int64>> mhistory;

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
	extern short playerstress;
	extern char_id_t playerid;
	extern interested_vector interested;
	extern std::vector<sf::Sprite> mapsprites;

	extern flat_multimap<unsigned int, std::unique_ptr<s_actionbase>> schedule;
	extern concurrency::concurrent_queue<std::function<void()>> uiqueue;

	extern flat_map<prov_id_t, std::string> provtooltip;

	void registerCharUpdate(char_id id);
	void registerTitleUpdate(title_id id);
	void registerProvinceUpdate(prov_id id);

	void flag_ch_update(char_id id);
	void flag_prov_update(prov_id id);
	void flag_title_update(title_id id);
};

std::string convertName(IN(std::string) input);

#define M_WIDTH 650