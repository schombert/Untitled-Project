#include "globalhelpers.h"
#include "sqlite3.h"
#include "generation.hpp"
#include "structs.hpp"
#include "uielements.hpp"
#include "datamanagement.hpp"
#include "ProvPane.h"
#include "ChPane.h"
#include "TPane.h"
#include "DynPane.h"
#include "WarPane.h"
#include "events.h"
#include "mapdisplay.h"
#include "TPane.h"
#include "ChPane.h"
#include "ProvPane.h"
#include "DynPane.h"
#include "CulPane.h"
#include "RelPane.h"
#include "peacewindow.h"
#include "WarPane.h"
#include "PlansWindow.h"
#include "reputation.h"
#include "traits.h"
#include "i18n.h"
#include "schedule.h"
#include "actions.h"
#include "pacts.h"
#include "random_activity.h"
#include "living_data.h"
#include "generated_ui.h"
#include "spies.h"
#include "prov_control.h"
#include "political_action.h"

using namespace boost::gregorian;
using namespace std;

/*void OutputDebugList(vector<boost::any> &results) {
	for (auto v : results) {
		if (v.type() == TYPE_VALUE) {
			string val = boost::any_cast<string>(v);
			OutputDebugStringA(val.c_str()); OutputDebugStringA("\r\n");
		}
		else if (v.type() == TYPE_ASSOC) {
			pair<string, string> val = boost::any_cast<pair<string, string>>(v);
			OutputDebugStringA(val.first.c_str()); OutputDebugStringA(" = "); OutputDebugStringA(val.second.c_str()); OutputDebugStringA("\r\n");
		}
		else if (v.type() == TYPE_LIST) {
			pair<string, vector<boost::any>> val = boost::any_cast<pair<string, vector<boost::any>>>(v);
			OutputDebugStringA(val.first.c_str()); OutputDebugStringA(" = {\r\n"); OutputDebugList(val.second); OutputDebugStringA("}\r\n");
		}
	}
}*/


bool inprovince(province &loc, sf::Vector2f mouse) {
	if (!loc.bounds.contains(static_cast<int>(mouse.x), static_cast<int>(mouse.y)))
		return false;
	float row = mouse.y - (float)loc.bounds.top;
	float rem = row - (int)row;
	int over = 0;
	const auto bounds = loc.intersect.get_row(static_cast<int>(row));
	for (auto it = bounds.first; it != bounds.second; ++it) {
		if (mouse.x > it->first + rem * it->second)
			over++;
	}
	/*for (auto it : loc.intersect[(int)row]) {
		if (mouse.x > it.first + rem * it.second)
			over++;
	}/**/
	if (over % 2 == 1)
		return true;
	return false;
}


// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines 
// intersect the intersection point may be stored in the floats i_x and i_y.
int get_line_intersection(float p0_x, float p0_y, float p1_x, float p1_y,
	float p2_x, float p2_y, float p3_x, float p3_y, float *i_x, float *i_y)
{
	float s02_x, s02_y, s10_x, s10_y, s32_x, s32_y, s_numer, t_numer, denom, t;
	s10_x = p1_x - p0_x;
	s10_y = p1_y - p0_y;
	s32_x = p3_x - p2_x;
	s32_y = p3_y - p2_y;

	denom = s10_x * s32_y - s32_x * s10_y;
	if (denom == 0)
		return 0; // Collinear
	bool denomPositive = denom > 0;

	s02_x = p0_x - p2_x;
	s02_y = p0_y - p2_y;
	s_numer = s10_x * s02_y - s10_y * s02_x;
	if ((s_numer < 0) == denomPositive)
		return 0; // No collision

	t_numer = s32_x * s02_y - s32_y * s02_x;
	if ((t_numer < 0) == denomPositive)
		return 0; // No collision

	if (((s_numer > denom) == denomPositive) || ((t_numer > denom) == denomPositive))
		return 0; // No collision
	// Collision detected
	t = t_numer / denom;
	if (i_x != NULL)
		*i_x = p0_x + (t * s10_x);
	if (i_y != NULL)
		*i_y = p0_y + (t * s10_y);

	return 1;
}

bool intersect(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {

	float a = (float)(y1 - y2) / (float)(x1 - x2);
	float b = (float)y1 - a * (float)x1;
	float c = (float)(y3 - y4) / (float)(x3 - x4);
	float d = (float)y3 - c * (float)x3;
	if (a == c)
		return false;
	float sx = (d - b) / (a - c);
	if (sx > min(x1, x2) && sx < max(x1, x2) && sx > min(x3, x4) && sx < max(x3, x4))
		return true;
	return false;
}

void setup_ex_chb(sf::RenderTexture& excheckbox) {
	sf::Texture checkbox;
	LoadDDS(checkbox, "gfx\\interface\\checkbox_default.dds");

	excheckbox.create(24 * 4, 24);
	excheckbox.clear(sf::Color::Black);

	sf::Sprite stamp;
	stamp.setTexture(checkbox);
	stamp.setTextureRect(sf::IntRect(0, 0, 24 * 2, 24));
	stamp.setPosition(0.0f, 0.0f);

	excheckbox.draw(stamp);

	stamp.setColor(sf::Color(128, 128, 128));
	stamp.setPosition(24.0f * 2, 0.0f);

	excheckbox.draw(stamp);
}

void setupPRender(sf::RenderTexture& composedcrowns, sf::RenderTexture& composedcrowns2){
	sf::Texture bigcrowns;
	LoadDDS(bigcrowns, "gfx\\interface\\shield_crown_strip_big.dds");
	sf::Texture medcrowns;
	LoadDDS(medcrowns, "gfx\\interface\\shield_crown_strip_medium.dds");
	sf::Texture prof;
	LoadDDS(prof, "gfx\\interface\\charaction_main_character_big.dds");
	sf::Texture prof2;
	LoadDDS(prof2, "gfx\\interface\\charaction_main_character_medium.dds");
	sf::Texture man;
	LoadDDS(man, "gfx\\traits\\monk.dds");
	sf::Texture woman;
	LoadDDS(woman, "gfx\\traits\\nun.dds");
	sf::Texture dead;
	LoadDDS(dead, "gfx\\interface\\overlay_char_dead.dds");
	sf::Texture sdead;
	LoadDDS(sdead, "gfx\\interface\\overlay_char_dead2.dds");


	composedcrowns.create(64 * 6, 64 * 5);
	composedcrowns.clear(sf::Color(0, 0, 0, 0));

	composedcrowns2.create(40 * 6, 40 * 5);
	composedcrowns2.clear(sf::Color(0, 0, 0, 0));

	sf::Sprite stamp;
	stamp.setTexture(bigcrowns);
	stamp.setTextureRect(sf::IntRect(64, 0, 64 * 5, 64));
	for (int q = 0; q < 5; q++) {
		stamp.setPosition(64.0f, 64.0f * q);
		composedcrowns.draw(stamp);
	}

	stamp.setTexture(medcrowns);
	stamp.setTextureRect(sf::IntRect(40, 0, 40 * 5, 40));
	for (int q = 0; q < 5; q++) {
		stamp.setPosition(40.0f, 40.0f * q);
		composedcrowns2.draw(stamp);
	}

	stamp.setTexture(prof);
	stamp.setTextureRect(sf::IntRect(0, 0, 48, 48));
	for (int q = 0; q < 4; q++) {
		stamp.setPosition(8.0f, 8.0f + 64 * q);
		composedcrowns.draw(stamp);
	}
	stamp.setTexture(prof2);
	stamp.setTextureRect(sf::IntRect(0, 0, 38, 38));
	for (int q = 0; q < 4; q++) {
		stamp.setPosition(1.0f, 1.0f + 40 * q);
		composedcrowns2.draw(stamp);
	}
	stamp.setTexture(man);
	stamp.setTextureRect(sf::IntRect(0, 0, 24, 24));
	for (int q = 0; q < 6; q++) {
		stamp.setPosition(64.0f * q + 35, 35.0f);
		composedcrowns.draw(stamp);
		stamp.setPosition(64.0f * q + 35, 35.0f + 64 * 2);
		composedcrowns.draw(stamp);
		stamp.setPosition(40.0f * q + 16, 16.0f);
		composedcrowns2.draw(stamp);
		stamp.setPosition(40.0f * q + 16, 16.0f + 40 * 2);
		composedcrowns2.draw(stamp);
	}
	stamp.setTexture(woman);
	for (int q = 0; q < 6; q++) {
		stamp.setPosition(64.0f * q + 35, 35 + 64.0f);
		composedcrowns.draw(stamp);
		stamp.setPosition(64.0f * q + 35, 35.0f + 64 + 64 * 2);
		composedcrowns.draw(stamp);
		stamp.setPosition(40.0f * q + 16, 16.0f + 40);
		composedcrowns2.draw(stamp);
		stamp.setPosition(40.0f * q + 16, 16.0f + 40 + 40 * 2);
		composedcrowns2.draw(stamp);
	}
	stamp.setTexture(dead);
	stamp.setTextureRect(sf::IntRect(0, 0, 32, 32));
	for (int q = 0; q < 6; q++) {
		stamp.setPosition(64.0f * q + 0, 2.0f + 64 * 2);
		composedcrowns.draw(stamp);
		stamp.setPosition(64.0f * q + 0, 2.0f + 64 * 3);
		composedcrowns.draw(stamp);
	}
	stamp.setTexture(sdead);
	stamp.setTextureRect(sf::IntRect(0, 0, 28, 28));
	for (int q = 0; q < 6; q++) {
		stamp.setPosition(40.0f * q - 4, 40.0f * 2 - 4);
		composedcrowns2.draw(stamp);
		stamp.setPosition(40.0f * q - 4, 40.0f * 3 - 4);
		composedcrowns2.draw(stamp);
	}

	composedcrowns.display();
	composedcrowns2.display();
}


void Back() {
	global::uiqueue.push([] {
		if (global::mhistory.size() > 1) {
			global::mhistory.pop_back();
			pair<int, sqlite_int64> gotop = global::mhistory.back();
			global::mhistory.pop_back();
			if (gotop.first == 1) {
				SetupProvPane( prov_id_t(static_cast<prov_id>(gotop.second)));
			} else if (gotop.first == 2) {
				SetupChPane( char_id_t(static_cast<char_id>(gotop.second)));
			} else if (gotop.first == 3) {
				SetupTPane( title_id_t(static_cast<title_id>(gotop.second)));
			} else if (gotop.first == 4) {
				SetupDynPane( dyn_id_t(static_cast<dyn_id>(gotop.second)));
			} else if (gotop.first == 5) {
				SetupCulPane( cul_id_t(static_cast<cul_id>(gotop.second)));
			} else if (gotop.first == 6) {
				SetupRelPane( rel_id_t(static_cast<rel_id>(gotop.second)) );
			}
		}
	});
}

void inline_update_panels() {
	if (!global::infowindows->gVisible())
		return;

	if (global::mhistory.size() > 0) {
		pair<int, sqlite_int64> gotop = global::mhistory.back();
		//global::mhistory.pop_back();
		if (gotop.first == 1) {
			//inline_prov_p_update(static_cast<prov_id>(gotop.second));
		} else if (gotop.first == 2) {
			inline_update_ch_pane(char_id_t(static_cast<char_id>(gotop.second)));
		} else if (gotop.first == 3) {
			inline_upate_title_pane(title_id_t(static_cast<title_id>(gotop.second)));
		} else if (gotop.first == 4) {
			inline_update_dyn_pane(dyn_id_t(static_cast<dyn_id>(gotop.second)));
		} else if (gotop.first == 5) {
			inline_update_cul_pane(cul_id_t(static_cast<cul_id>(gotop.second)));
		} else if (gotop.first == 6) {
			inline_update_rel_pane(rel_id_t(static_cast<rel_id>(gotop.second)));
		}
	}
	
}


struct _icons {
	sf::Texture* seigeicon;
	sf::Texture* ticon;
	sf::Texture* cap;
} icons;




#define MAP_MODES 4

void displaymapmode() {
	global::mapsprites.clear();
	global::provtooltip.clear();

	switch (global::mapmode) {
	case MAP_MODE_POL:
		color_prov_by_title(*icons.cap, r_lock());
		break;
	case MAP_MODE_VAS:
	{
		r_lock l;
		color_prov_by_vassal(lowest_admin_in_prov(global::focused, l), *icons.cap, l);
	}
		break;
	case MAP_MODE_ECON: 
		color_prov_by_tax();
		break;
	case MAP_MODE_WAR:
	case MAP_MODE_WARSEL:
		if (valid_ids(global::map.displayedwar.wid)) {
			r_lock l;
			displaywar(global::map.displayedwar.wid, is_agressor(global::map.displayedwar.adm, global::map.displayedwar.wid, l), *icons.seigeicon, *icons.ticon, l);
		} else {
			display_nowar(r_lock());
		}
		break;
	case MAP_MODE_CULTURE:
		color_prov_by_culture(global::map.displayedcul);
		break;
	case MAP_MODE_RELIGION:
		color_prov_by_religion(global::map.displayedrel);
		break;
	case MAP_MODE_DISTANCE:
		color_prov_by_distance(global::map.displayedprov);
		break;
	}
}

GLuint uvao;
GLuint ovao;

update_record* urecords[] = {&emission_display_rec, &emission_selection_rec, &war_window_rec, &smission_display_rec, &smission_selection_rec, &prov_pane_rec, &modal_influence_window};

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	int nCmdShow
	)
{

	if (!file_exists("data.db")) {
		generate_data("data.db");
		return 0;
	}

	int modulus = 0;
	//global::provinces.resize(MAX_PROVS);


	sf::Clock gameclock;
	bool quit = false;

	sf::Vector2u size;

	sf::Texture speedicons;
	sqlite3* db = nullptr;

	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;

	

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		OutputDebugStringA("GLEW Error: ");
		OutputDebugStringA((char*)glewGetErrorString(err));
		
	}

	sf::RenderWindow window(sf::VideoMode(800, 600), "CCL", sf::Style::Default, settings);
	window.setVerticalSyncEnabled(true);
	//window.setFramerateLimit(60);
	global::setWindow(&window);
	ShowWindow(window.getSystemHandle(), SW_SHOWMAXIMIZED);

	window.clear();
	window.display();

	
	window.setActive(false);

	sf::Texture ctex;
	sf::Texture ctex2;
	sf::Texture excheckbox;

	sf::Texture lockicon;
	sf::Texture arrowicons;
	sf::Texture ticon;
	sf::Texture seigeicon;
	sf::Texture back;
	sf::Texture close;
	sf::Texture cap;
	sf::Texture takeaction;
	sf::Texture focusedbutton;
	
	sf::Texture terrain;
	sf::Texture iconstrip_t;

	sf::Clock frameclock;
	sf::Clock update_limiter;

	concurrency::parallel_invoke([&] {
		window.setActive(true);

		LoadDDS(terrain, "map2\\mapSHADOW.png");
		LoadDDS(speedicons, "gfx\\interface\\speed_indicator.dds");

		LoadDDS(lockicon, "gfx\\interface\\Lock_button.dds");
		LoadDDS(arrowicons, "gfx\\interface\\toggle_icons.dds");

		LoadDDS(ticon, "gfx\\interface\\icon_martial.dds");
		LoadDDS(seigeicon, "gfx\\interface\\siege_assault_button.dds");
		LoadDDS(cap, "gfx\\interface\\icon_prestige.dds");
		LoadDDS(takeaction, "gfx\\interface\\job_improve_relations.dds");
		LoadDDS(focusedbutton, "gfx\\interface\\charaction_togglemessage_big.dds");
		

		LoadDDS(back, "gfx\\interface\\main_back_button.dds");
		LoadDDS(close, "gfx\\interface\\main_close_button.dds");
		LoadDDS(iconstrip_t, "gfx\\interface\\charaction_sub_diplomacy_big.dds");

		load_trait_icons();

		sf::RenderTexture composedcrowns;
		sf::RenderTexture composedcrowns2;
		setupPRender(composedcrowns, composedcrowns2);
		ctex = composedcrowns.getTexture();
		ctex2 = composedcrowns2.getTexture();

		sf::RenderTexture echb;
		setup_ex_chb(echb);
		excheckbox = echb.getTexture();
		
		window.setActive(false);

		global::takeaction_tex.init(&takeaction);
		global::lock_tex.init(&lockicon);
		global::messages_button_tex.init(&focusedbutton);
		global::back_tex.init(&back);
		global::close_tex.init(&close);
		global::sword_icon.init(&ticon);
		global::arrows_tex.init(&arrowicons);
		global::checkbox.init(&excheckbox);
		global::iconstrip.init(&iconstrip_t);

		global::bigcrowns_tex.init(&ctex);
		global::smallcrowns_tex.init(&ctex2);

		icons.seigeicon = &seigeicon;
		icons.ticon = &ticon;
		icons.cap = &cap;
		
	}, [&] {

		load_save("data.db");
		global::monthly_update_title_stats(fake_lock());
		// init_interests(w_lock());
		// for (auto p : global::living) {
			// titled_data* t = nullptr;
			// untitled_data* u = nullptr;
			// get_living_data(p, u, fake_lock());
			// if(u)
			//	u->income_estimate = global::force_project_income(p.value, fake_lock());
			//if(t)
			//	t->strength_estimate = global::force_calculate_strength(p, fake_lock());
		// }
		global::end_of_day.set();

		//initactiontypes();
		//initactionblocktypes();
	}, [&] {
		init_label_numbers();
		load_text_file(TEXT("text.txt"));
		clear_label_numbers();
	}
	);

	{
		const auto lk = global::lockWindow();
		create_province_program();
	}


	window.setActive(true);
	window.clear();
	window.display();

	flat_map<edge,int> bordertypes;

	sf::Font fallback1;
	fallback1.loadFromFile("NotoSans-Bold.ttf");

	sf::Font fallback2;
	fallback2.loadFromFile("unifont-9.0.02.ttf");

	fallback1.set_fallback(fallback2);

	sf::Font font;
	font.loadFromFile("FallingSkyBd.otf");
	font.set_fallback(fallback1);
	
	global::font = &font;

	sf::Font font2;
	font2.loadFromFile("Primitive.ttf");
	font2.set_fallback(fallback1);

	global::gothic_font = &font2;

	global::standard_text.init(global::font, sf::Color::Black, 14);
	global::header_text.init(global::gothic_font, sf::Color::Black, 18);
	global::tooltip_text.init(global::font, sf::Color::White, 14);
	global::large_tooltip_text.init(global::gothic_font, sf::Color::White, 18);

	const sf::Color mback(200, 200, 200);

	global::solid.init(mback);
	global::solid_border.init(mback, sf::Color::Black, 1);
	global::solid_black.init(sf::Color::Black);
	global::disabled_fill.init(mback, sf::Color(128, 128, 128), 1);
	global::bar_fill.init(sf::Color::Green, sf::Color::Black, 1);

	global::mapmode = MAP_MODE_POL;
	global::setFlag(FLG_MAP_UPDATE | FLG_BORDER_UPDATE);

	uiSimpleTooltip::init_shared_tooltip(global::overlay, global::tooltip_text);

	
	days dtemp = date(1400 + 867, Jan, 1) - date(1400, Jan, 1);
	global::currentday = dtemp.days();

	const auto speedicon = std::make_shared<uiIcon>(0,5,20,20,global::uicontainer,&speedicons,sf::IntRect(0,0,20,20));
	const auto currentdate = global::uicontainer->add_element<uiSimpleText>(0, 5, w_day_to_string_short(global::currentday), global::solid_black, global::tooltip_text);

	global::uicontainer->subelements.push_back(speedicon);
	
	currentdate->pos.height = 20;
	currentdate->pos.width = 125;
	currentdate->margin = 3;

	auto globaltooltip = global::overlay->add_element<uiSimpleText>(0, 10, TEXT("XXXXXXXXX"), global::solid_black, global::large_tooltip_text);
	globaltooltip->margin = 3;
	globaltooltip->setVisible(false);

	auto fr = std::make_shared<uiFloatRect>(-160, -305, 0.0, 0.0, 155, 300, global::uicontainer);
	global::uicontainer->subelements.push_back(fr);
	auto btl = std::make_shared<uiScrollView>(0, 0, 155, 300, fr);
	fr->subelements.push_back(btl);
	btl->add_element<uiButton>(0, 0, 145, 20, get_simple_string(TX_MAP_POLITICAL), global::solid_black, global::tooltip_text, [&](uiButton *b){
		global::mapmode = MAP_MODE_POL;
		globaltooltip->setVisible(false);
		global::setFlag(FLG_MAP_UPDATE);
	});
	btl->add_element<uiButton>(0, 25, 145, 20, get_simple_string(TX_MAP_VASSALS), global::solid_black, global::tooltip_text, [&](uiButton *b){
		global::mapmode = MAP_MODE_VAS;
		global::setFlag(FLG_MAP_UPDATE);
	});
	btl->add_element<uiButton>(0, 50, 145, 20, get_simple_string(TX_MAP_ECONOMY), global::solid_black, global::tooltip_text, [&](uiButton *b){
		global::mapmode = MAP_MODE_ECON;
		global::setFlag(FLG_MAP_UPDATE);
	});
	btl->add_element<uiButton>(0, 75, 145, 20, get_simple_string(TX_MAP_WARS), global::solid_black, global::tooltip_text, [&](uiButton *b){
		global::mapmode = MAP_MODE_WAR;
		global::map.displayedwar.adm = get_prime_admin(char_id_t(global::playerid), r_lock()).value;
		global::map.displayedwar.wid = newwar::NONE;
		global::setFlag(FLG_MAP_UPDATE);
	});
	btl->add_element<uiButton>(0, 100, 145, 20, get_simple_string(TX_DISTANCE), global::solid_black, global::tooltip_text, [&](uiButton *b) {
		global::mapmode = MAP_MODE_DISTANCE;
		global::map.displayedprov = valid_ids(global::focused) ? global::focused : prov_id_t(1);
		global::setFlag(FLG_MAP_UPDATE);
	});
	btl->calcTotalHeight();

	global::infowindows = global::uicontainer->add_element<uiDragRect>(10, 10, M_WIDTH, 500, global::solid_border);
	global::infowindows->setVisible(false);

	global::infowindows->add_element<uiGButton>(M_WIDTH - 20, 0, 20, 20, global::close_tex, get_simple_string(TX_CLOSE), global::tooltip_text, [](uiGButton *a) {  global::infowindows->setVisible(false); });
	global::infowindows->add_element<uiGButton>(M_WIDTH - 40, 0, 20, 20, global::back_tex, get_simple_string(TX_BACK), global::tooltip_text, [](uiGButton *a) {Back(); });

	InitTPane();
	InitChPane();
	InitDynPane();
	InitCulPane();
	InitRelPane();
	InitPlansWindow(&font);
	InitPeaceWindow();
	InitEventWindow();
	init_message_box();

	init_pacts_ui();
	init_all_generated_ui();
	
	double zoom = 1.0;

	bool dragging = false;
	glm::dvec3 draggedvector(0.0);
	glm::ivec2 draggedpoint(0);

	int lastx = 0;
	int lasty = 0;
	

	global::uiTasks.run([&] {
		//blobcontainer blbs;
		while (!quit) {
			sf::Time elapsed = gameclock.getElapsedTime();
			if (!global::paused && (global::speed == 5 || elapsed.asMilliseconds() >= 1500 / pow(2, global::speed))) {
				gameclock.restart();
				global::end_of_day.reset();

				fake_lock l;
				++global::currentday;

				date d = date_offset + days(global::currentday);

				if (d.day() == 1 && d.month() == 1) {
					update_dj(w_lock());
				}

				if (d.day() == 1) {
					global::get_tax_income(l);

					w_lock wl;
					update_pacts(wl);
				}

				//concurrency::parallel_invoke([&]{
				//	modulus = (modulus + 1) % UPDATE_FREQ;
				//	global::executeAI(modulus, blbs);
				//}, [&]{
				if (d.day() == 1) {
					w_lock wl;
					war_pool.for_each(wl, [&wl](INOUT(war_pair) wp) {
						pay_for_war(wp.attacker, global::currentday, wl);
						pay_for_war(wp.defender, global::currentday, wl);

						if(get_war_decider(get_object(wp.attacker.primary, wl), wl) != global::playerid)
							balance_war(wp, true, wl);
						if (get_war_decider(get_object(wp.defender.primary, wl), wl) != global::playerid)
							balance_war(wp, false, wl);

						updatewar(wp, wl);
					});
					update_seiges(wl);

					if (global::mapmode == 3 || global::mapmode == 4) {
						global::setFlag(FLG_MAP_UPDATE);
					}

					global::setFlag(FLG_PANEL_UPDATE);
					war_window_rec.needs_update = true;
				} else if (d.day() == 2) {
					global::monthly_update_title_stats(l);
					global::setFlag(FLG_PANEL_UPDATE);
				} else if (d.day() == 10) {
					if (d.month() == 1) {
						for (INOUT(auto) ch : global::untitled_pool.pool) {
							if ((ch.flags & P_FLAG_VALID) != 0) {
								ch.p_honorable = drift_reputation(ch.p_honorable);
								ch.p_peaceful = drift_reputation(ch.p_peaceful);
							}
						}
					}
					global::setFlag(FLG_PANEL_UPDATE);
				}

				modulus = (modulus + 1) % UPDATE_FREQ;

				concurrency::parallel_for_each(modulus_begin<UPDATE_FREQ>(modulus, admin_pool.pool),
					modulus_end<UPDATE_FREQ>(modulus, admin_pool.pool),
					[](IN(administration) adm) {
						if (!adm.is_clear())
							do_admin_step(adm);
				});

				execute_actions();

				execute_schedule();

				proposals.execute_entries();
				political_actions.execute_entries();

				global::setFlag(FLG_DATE_UPDATE);
				global::end_of_day.set();
			} else if (global::paused) {
				concurrency::wait(100);
			} else {
#undef Yield
				concurrency::wait(1);
				//Context::Yield();
#define Yield()
			}
		}
	});
	


		
		
		while (window.isOpen()) {
			sf::Event event;
			while (window.pollEvent(event)) {
				if (event.type == sf::Event::Closed) {
					global::quitevent.set();
					window.close();
					quit = true;
				} else if (event.type == sf::Event::Resized) {
					auto tm = window.getSize();

					{
						global::uicontainer->pos.width = tm.x;
						global::uicontainer->pos.height = tm.y;

						speedicon->pos.left = tm.x - 150;
						currentdate->pos.left = tm.x - 130;
						global::uicontainer->updatepos();

						global::overlay->pos.width = tm.x;
						global::overlay->pos.height = tm.y;
						global::overlay->updatepos();
					}

					OGLLock mainwin(global::lockWindow());
					sf::Vector2f worldPos = mainwin->mapPixelToCoords(sf::Vector2i(0, 0));
					sf::FloatRect visibleArea(static_cast<float>(tm.x) / -2.0f, static_cast<float>(tm.y) / -2.0f, static_cast<float>(tm.x), static_cast<float>(tm.y));
					sf::View v(visibleArea);
					v.zoom(static_cast<float>(zoom));

					mainwin->setView(v);
				} else if(event.type == sf::Event::TextEntered) {
					global::uicontainer->textinput(event.text.unicode);
				} else if (event.type == sf::Event::KeyPressed) {
					bool caught = false;
					{
						caught = global::uicontainer->keypress(event.key.code);
					}
					if (!caught) {
						if (event.key.code == sf::Keyboard::Space) {
							global::paused = !global::paused;
							gameclock.restart();
							if (global::paused)
								speedicon->texbox = sf::IntRect(0, 0, 20, 20);
							else
								speedicon->texbox = sf::IntRect(20 * global::speed, 0, 20, 20);

						} else if (event.key.code == sf::Keyboard::Subtract || event.key.code == sf::Keyboard::Dash) {
							global::speed--;
							if (global::speed < 1) {
								global::speed = 1;
								global::paused = true;
							}
							if (global::paused)
								speedicon->texbox = sf::IntRect(0, 0, 20, 20);
							else
								speedicon->texbox = sf::IntRect(20 * global::speed, 0, 20, 20);
						} else if (event.key.code == sf::Keyboard::Equal || event.key.code == sf::Keyboard::Add) {
							if (global::paused) {
								global::paused = false;
							} else {
								global::speed++;
								if (global::speed > 5) {
									global::speed = 5;
								}
							}
							speedicon->texbox = sf::IntRect(20 * global::speed, 0, 20, 20);
						}
					}
				} else if (event.type == sf::Event::MouseWheelMoved) {
					sf::Vector2i pixelPos = sf::Mouse::getPosition(window);

					bool caught;
					{
						caught = global::uicontainer->scroll(event.mouseWheel.x, event.mouseWheel.y,- event.mouseWheel.delta);
					}

					if (!caught) {

						OGLLock mainwin(global::lockWindow());
						sf::Vector2f worldPos = mainwin->mapPixelToCoords(pixelPos);

						const auto inverse_r = generate_mat_inverse_transform(global::horzrotation, global::vertrotation);
						const glm::dvec3 cvec = map_to_sphere(glm::dvec2(worldPos.x, worldPos.y), inverse_r);

						sf::View v = mainwin->getView();
						double zoommult = pow(2, event.mouseWheel.delta / 2.0);
						zoom *= zoommult;
						if (zoom > 10.0) {
							zoommult *= 10.0 / zoom;
							zoom = 10.0;
						}
						if (zoom < .5) {
							zoommult *= .5 / zoom;
							zoom = .5;
						}
						v.zoom(static_cast<float>(zoommult));

						//const auto inverse_r = generate_mat_inverse_transform(global::horzrotation, global::vertrotation);
						//const auto ipos = map_to_globe(glm::vec2(worldPos.x, worldPos.y), inverse_r);

						mainwin->setView(v);

						sf::Vector2f ppos2 = mainwin->mapPixelToCoords(pixelPos);

						bool nosolution = false;
						const auto rotation = project_sphere_to_map(cvec, glm::dvec2(ppos2.x, ppos2.y), nosolution);

						if (!nosolution) {
							global::horzrotation = rotation.x;
							global::vertrotation = rotation.y;
						}

						//const auto npos = map_to_globe(glm::vec2(ppos2.x, ppos2.y), inverse_r);

						//global::horzrotation += static_cast<double>(npos.x - ipos.x);
						//global::vertrotation -= static_cast<double>(npos.y - ipos.y);
					}
				} else if (event.type == sf::Event::MouseButtonPressed) {
					if (event.mouseButton.button == sf::Mouse::Left) {
						if (!global::uicontainer->click(event.mouseButton.x, event.mouseButton.y)) {
							dragging = true;
							draggedpoint = glm::ivec2(0, 0);
							OGLLock mainwin(global::lockWindow());
							const sf::Vector2f worldPos = mainwin->mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
							const auto inverse_r = generate_mat_inverse_transform(global::horzrotation, global::vertrotation);
							draggedvector = map_to_sphere(glm::dvec2(worldPos.x, worldPos.y), inverse_r);
						}
						close_menus();
					}
					if (event.mouseButton.button == sf::Mouse::Right) {
						bool caught;
						{
							caught = global::uicontainer->rclick(event.mouseButton.x, event.mouseButton.y);
							close_menus();
						}
						if (!caught) {
							prov_id_t prov;

							glm::vec2 texposition;
							{
								OGLLock mainwin(global::lockWindow());
								const sf::Vector2f worldPos = mainwin->mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
								const auto inverse_r = generate_mat_inverse_transform(global::horzrotation, global::vertrotation);
								texposition = map_to_texture(glm::vec2(worldPos.x, worldPos.y), inverse_r);
							}

							for (prov_id i = 1; i < detail::provinces.size(); i++) {
								if (inprovince(detail::provinces[i], sf::Vector2f(texposition.x, texposition.y))) {
									prov = i;
									break;
								}
							}

							if (prov_has_title(prov)) {
								global::clear_highlight();
								global::focused = prov.value;
								SetupProvPane(prov);
							} else {
								global::focused = 0;
								global::infowindows->setVisible(false);
								global::clear_highlight();
							}
						}
					}
				} else if (event.type == sf::Event::MouseButtonReleased) {
					if (event.mouseButton.button == sf::Mouse::Left) {
						bool caught;
						{
							caught = global::uicontainer->release(event.mouseButton.x, event.mouseButton.y);
						}

						if (!caught) {
							if (dragging && draggedpoint.x < 10 && draggedpoint.x > -10 && draggedpoint.y > -10 && draggedpoint.y < 10) {
								global::clear_highlight();
								global::focused = 0;

								glm::vec2 texposition;
								{
									OGLLock mainwin(global::lockWindow());
									const sf::Vector2f worldPos = mainwin->mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
									const auto inverse_r = generate_mat_inverse_transform(global::horzrotation, global::vertrotation);
									texposition = map_to_texture(glm::vec2(worldPos.x, worldPos.y), inverse_r);
								}

								for (prov_id i = 1; i < detail::provinces.size(); i++) {
									if (inprovince(detail::provinces[i], sf::Vector2f(texposition.x, texposition.y))) {
										global::focused = prov_id_t(i);
										break;
									}
								}

								if (!P_HAS_TITLE(global::focused.value)) {
									//currentdate.text.setString(std::to_string(global::focused));
									global::focused = 0;
									global::infowindows->setVisible(false);
								} else {
									if (peace_window_open()) {
										//add_province_to_peace(global::focused, r_lock(global::index_lock));
									} else if (global::mapmode == MAP_MODE_VAS) {
										global::setFlag(FLG_MAP_UPDATE);
									} else if (global::mapmode == MAP_MODE_CULTURE) {
										IN(auto) prv = get_object(global::focused);
										global::map.displayedcul = prv.culture;
										global::setFlag(FLG_MAP_UPDATE);
									} else if (global::mapmode == MAP_MODE_RELIGION) {
										IN(auto) prv = get_object(global::focused);
										global::map.displayedrel = prv.religion;
										global::setFlag(FLG_MAP_UPDATE);
									} else if (global::mapmode == MAP_MODE_WAR) {
										if (valid_ids(global::focused)) {
											SetupWarPane(highest_admin_in_prov(global::focused, r_lock()), war_id_t());
										} else {
											SetupWarPane(admin_id_t(), war_id_t());
										}
										global::setFlag(FLG_MAP_UPDATE);
									} else if (global::mapmode == MAP_MODE_WARSEL) {
										
									} else if (global::mapmode == MAP_MODE_DISTANCE) {
										if (valid_ids(global::focused)) {
											global::map.displayedprov = global::focused;
											global::setFlag(FLG_MAP_UPDATE);
										}
									} else { //no special action to be taken on click
										//SetupProvPane(& global::focused);
										r_lock l;
										SetupChPane(get_object(highest_admin_in_prov(global::focused, l), l).executive);
									}
								}
							}
						}

						if (dragging) {
							dragging = false;
						}
					}
					if (event.mouseButton.button == sf::Mouse::Right) {
						global::uicontainer->rrelease(event.mouseButton.x, event.mouseButton.y);
					}
				} else if (event.type == sf::Event::MouseMoved) {

					sf::Vector2f ppos1;
					{
						OGLLock mainwin(global::lockWindow());
						ppos1 = mainwin->mapPixelToCoords(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
					}


					if (dragging) {
						
						bool nosolution = false;
						const auto rotation = project_sphere_to_map(draggedvector, glm::dvec2(ppos1.x, ppos1.y), nosolution);

						if (!nosolution) {
							global::horzrotation = rotation.x;
							global::vertrotation = rotation.y;
						}

						draggedpoint.x -= static_cast<int>((event.mouseMove.x - lastx)*zoom);
						draggedpoint.y -= static_cast<int>((event.mouseMove.y - lasty)*zoom);
					}

					if (!global::uicontainer->move(event.mouseMove.x, event.mouseMove.y)) {
						if (global::provtooltip.size() > 0) {
							prov_id over = 0;
							const auto inverse_r = generate_mat_inverse_transform(global::horzrotation, global::vertrotation);
							const auto texposition = map_to_texture(glm::vec2(ppos1.x, ppos1.y), inverse_r);
							
							for (prov_id i = 1; i < detail::provinces.size(); ++i) {
								if (inprovince(detail::provinces[i], sf::Vector2f(static_cast<float>(texposition.x), static_cast<float>(texposition.y)))) {
									over = i;
									break;
								}
							}

							if (global::provtooltip.find(prov_id_t(over)) != global::provtooltip.cend()) {
								globaltooltip->updateText(str_to_wstr(global::provtooltip[prov_id_t(over)]));
								globaltooltip->pos.left = (global::uicontainer->pos.width - globaltooltip->pos.width) / 2;
								globaltooltip->setVisible(true);
							} else {
								globaltooltip->setVisible(false);
							}
						} else {
							globaltooltip->setVisible(false);
						}
					} else {
						globaltooltip->setVisible(false);
					}

					lastx = event.mouseMove.x;
					lasty = event.mouseMove.y;
				}
			}

			std::function<void()> f;
			//size_t uicnt = 0;
			while (global::uiqueue.try_pop(f)) {
				f();
				//if (++uicnt == 60)
				//	break;
			}

			


			

#define MINIMUM_UPDATE_MS 250

			if (update_limiter.getElapsedTime().asMilliseconds() > MINIMUM_UPDATE_MS) {
				if (global::testFlagAndReset(FLG_MAP_UPDATE)) {
					displaymapmode();
				}
				if (global::testFlagAndReset(FLG_BORDER_UPDATE)) {
					popualte_border_types(r_lock());
				}
				update_limiter.restart();
			}

			if (global::testFlagAndReset(FLG_WWIN_UPDATE)) {
				//inline_update_warpane();
				inline_update_peace_window();
			}

			if (global::testFlagAndReset(FLG_PANEL_UPDATE)) {
				inline_update_panels();
			}

			for (auto p : urecords) {
				if (p->needs_update) {
					p->needs_update = false;
					p->func();
				}
			}

			{

				OGLLock mainwin(global::lockWindow());

				if (global::testFlagAndReset(FLG_DATE_UPDATE)) {
					currentdate->updateText(w_day_to_string_short(global::currentday));
				}

				mainwin->resetGLStates();
				mainwin->clear();
				RawGraphicsSetUp(mainwin->getViewport(mainwin->getView()), mainwin->getView(), mainwin->getSize().y);

				draw_map(mainwin, zoom, global::horzrotation, global::vertrotation, terrain);

				RawGraphicsFinish();
				mainwin->resetGLStates();
				

				//draw map icons
				const auto r_mat = generate_mat_transform(global::horzrotation, global::vertrotation);
				for (auto &sprt : global::mapsprites) {
					sprt.setScale(static_cast<float>(zoom*.20 + .80), static_cast<float>(zoom*.20 + .80));
					sf::Vector2f origp = sprt.getPosition();
					const auto newg = texture_to_globe(glm::vec2(origp.x, origp.y), r_mat);
					if (newg.x >= -3 && newg.x <= 3) {
						const auto newp = map_from_globe(newg);
						sprt.setPosition(static_cast<float>(newp.x), static_cast<float>(newp.y));
						mainwin->draw(sprt);
						sprt.setPosition(origp);
					}
				}


				sf::View old = mainwin->getView();
				auto tm = mainwin->getSize();
				mainwin->setView(sf::View(sf::FloatRect(0.0f, 0.0f, static_cast<float>(tm.x), static_cast<float>(tm.y))));
				global::uicontainer->draw(mainwin, 0, 0);
				global::overlay->draw(mainwin, 0, 0);
				mainwin->setView(old);
				

				mainwin->_display();
			}

			auto felapsed = frameclock.getElapsedTime();
			if (felapsed.asMilliseconds() < (1000 / 60)) {
				Concurrency::wait((1000 / 60) - felapsed.asMilliseconds() );
			}
			frameclock.restart();

		}


	//quit = true;

	global::uiTasks.wait();

	
#ifndef HAS_PEOPLE
	sqlite3_close(db);
#endif

	unload_trait_icons();

	return 0;
}