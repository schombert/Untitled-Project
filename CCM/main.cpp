#include "globalhelpers.h"
#include "sqlite3.h"
#include "structs.h"
#include "generation.hpp"
#include "uielements.hpp"
#include "datamanagement.hpp"
#include "mapdisplay.h"
#include "i18n.h"
#include "generated_ui.h"

using namespace boost::gregorian;
using namespace std;


/*bool inprovince(province &loc, sf::Vector2f mouse) {
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
	if (over % 2 == 1)
		return true;
	return false;
}*/


#define MAP_MODES 4

void displaymapmode() {
	global::mapsprites.clear();
	global::provtooltip.clear();

	switch (global::mapmode) {
	case MAP_MODE_POL:
		break;
	case MAP_MODE_VAS:
		break;
	case MAP_MODE_ECON: 
		break;
	case MAP_MODE_WAR:
	case MAP_MODE_WARSEL:
		break;
	case MAP_MODE_CULTURE:
		break;
	case MAP_MODE_RELIGION:
		break;
	case MAP_MODE_DISTANCE:
		break;
	}
}

// GLuint uvao;
// GLuint ovao;

// update_record* urecords[] = {&emission_display_rec, &emission_selection_rec, &war_window_rec, &smission_display_rec, &smission_selection_rec, &prov_pane_rec, &modal_influence_window};

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	int nCmdShow
	)
{



	sf::Clock gameclock;
	bool quit = false;


	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		OutputDebugStringA("GLEW Error: ");
		OutputDebugStringA((char*)glewGetErrorString(err));
		
	}

	sf::RenderWindow window(sf::VideoMode(800, 600), "CCM", sf::Style::Default, settings);
	window.setVerticalSyncEnabled(true);

	global::setWindow(&window);

	ShowWindow(window.getSystemHandle(), SW_SHOWMAXIMIZED);

	window.clear();
	window.display();

	window.setActive(false);

	sf::Texture back;
	sf::Texture close;
	sf::Texture terrain;

	sf::Clock frameclock;
	sf::Clock update_limiter;

	concurrency::parallel_invoke([&] {
		window.setActive(true);

		LoadDDS(terrain, "map2\\mapSHADOW.png");
		LoadDDS(back, "gfx\\interface\\main_back_button.dds");
		LoadDDS(close, "gfx\\interface\\main_close_button.dds");
		
		window.setActive(false);

		global::back_tex.init(&back);
		global::close_tex.init(&close);

		
	}, [&] {

		// load_save("data.db");

		global::end_of_day.set();

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
		global::setFlag(FLG_MAP_UPDATE);
	});
	btl->add_element<uiButton>(0, 100, 145, 20, get_simple_string(TX_DISTANCE), global::solid_black, global::tooltip_text, [&](uiButton *b) {
		global::mapmode = MAP_MODE_DISTANCE;
		global::setFlag(FLG_MAP_UPDATE);
	});
	btl->calcTotalHeight();


	init_message_box();
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
						// ...
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

							//for (prov_id i = 1; i < detail::provinces.size(); i++) {
							//	if (inprovince(detail::provinces[i], sf::Vector2f(texposition.x, texposition.y))) {
							//		prov = i;
							//		break;
							//	}
							// }

							// if (prov_has_title(prov)) {
							//	global::clear_highlight();
							//	global::focused = prov.value;
							//	SetupProvPane(prov);
							// } else {
							//	global::focused = 0;
							//	global::infowindows->setVisible(false);
							//	global::clear_highlight();
							// }
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

								// for (prov_id i = 1; i < detail::provinces.size(); i++) {
								//	if (inprovince(detail::provinces[i], sf::Vector2f(texposition.x, texposition.y))) {
								//		global::focused = prov_id_t(i);
								///		break;
								//	}
								// }

							
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
							
							// for (prov_id i = 1; i < detail::provinces.size(); ++i) {
							//	if (inprovince(detail::provinces[i], sf::Vector2f(static_cast<float>(texposition.x), static_cast<float>(texposition.y)))) {
							//		over = i;
							//		break;
							//	}
							// }

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
			}

			if (global::testFlagAndReset(FLG_PANEL_UPDATE)) {
			}

			// for (auto p : urecords) {
			//	if (p->needs_update) {
			//		p->needs_update = false;
			//		p->func();
			//	}
			// }

			{

				OGLLock mainwin(global::lockWindow());

				if (global::testFlagAndReset(FLG_DATE_UPDATE)) {
					
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


	return 0;
}