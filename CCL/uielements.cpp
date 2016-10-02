#include "globalhelpers.h"
#include "uielements.hpp"
#include "structs.hpp"
#include "ChPane.h"
#include "TPane.h"
#include "ProvPane.h"
#include "events.h"
#include "actions.h"
#include "i18n.h"
#include "datamanagement.hpp"
#include "mapdisplay.h"

const size_t yes_no_array[2] = {TX_YES, TX_NO};
const size_t accept_decline_array[2] = {TX_ACCEPT, TX_DECLINE};
const size_t options_array[2] = {TX_A_OPT1, TX_A_OPT2};
const size_t accept_postpone_decline_array[3] = {TX_ACCEPT, TX_DECLINE, TX_POSTPONE};
const size_t proceed_cancel_array[2] = {TX_PROCEED, TX_CANCEL};
const size_t approve_abstain_oppose_array[3] = {TX_APPROVE, TX_ABSTAIN, TX_OPPOSE};

std::vector<std::weak_ptr<uiMenuRect>> menulst;
bool menu_clicked = false;

void add_to_menus(IN(std::shared_ptr<uiMenuRect>) m) {
	for (INOUT(auto) w : menulst) {
		if (w.expired()) {
			w = m;
			return;
		}
	}
	menulst.emplace_back(m);
}

void remove_from_menus(IN(std::shared_ptr<uiMenuRect>) m) {
	for (INOUT(auto) w : menulst) {
		if (weak_equals(w, m)) {
			w.reset();
		}
	}
}

void close_menus() {
	if (!menu_clicked) {
		for (IN(auto) w : menulst) {
			if (auto m = w.lock()) {
				m->close();
			}
		}
	}
	menu_clicked = false;
}

void ArrangeSubelements(uiElement* e, int margin) {
	int cx = margin;
	int cy = margin;
	int nxtcy = 0;
	for (auto i = e->subelements.begin(); i != e->subelements.end(); i++) {
		if (cx + (*i)->pos.width > e->pos.width) {
			cx = margin;
			cy = nxtcy;
		}

		(*i)->pos.left = cx;
		(*i)->pos.top = cy;
		(*i)->updatepos();

		cx += ((*i)->pos.width + margin);
		nxtcy = std::max(nxtcy, (*i)->pos.height + cy + margin);
	}
}

inline bool rectcontains(const sf::IntRect &ir, const int x, const int y) {
	return x >= ir.left && y >= ir.top && x <= ir.left + ir.width && y <= ir.top + ir.height;
}

sf::IntRect largest_rect(decltype(std::declval<uiElement>().subelements.begin()) begin, decltype(std::declval<uiElement>().subelements.end()) end, IN(sf::IntRect) base, int minx, int miny) {
	if (begin == end)
		return base;

	if(!(*begin)->visible)
		return largest_rect(++begin, end, base, minx, miny);

	IN(sf::IntRect) current = (*begin)->pos;

	if (current.left + current.width < base.left ||
		current.left > base.left + base.width ||
		current.top + current.height < base.top ||
		current.top > base.top + base.height)
		return largest_rect(++begin, end, base, minx, miny);

	sf::IntRect left(0, base.top, 0, base.height);
	sf::IntRect right(0, base.top, 0, base.height);
	sf::IntRect top(base.left, 0, base.width, 0);
	sf::IntRect bottom(base.left, 0, base.width, 0);

	//lr
	if (current.left < base.left && current.left + current.width > base.left + base.width) {
		//no left & right
	} else if (current.left < base.left) {
		// no left
		right.left = current.left + current.width;
		right.width = (base.left + base.width) - (current.left + current.width);
	} else if (current.left + current.width > base.left + base.width) {
		// no right
		left.left = base.left;
		left.width = current.left - base.left;
	} else {
		//both
		right.left = current.left + current.width;
		right.width = base.left + base.width - (current.left + current.width);
		left.left = base.left;
		left.width = current.left - base.left;
	}


	//tb
	if (current.top < base.top && current.top + current.height > base.top + base.height) {
		//no top & bottom
	} else if (current.top < base.top) {
		// no top
		bottom.top = current.top + current.height;
		bottom.height = (base.top + base.height) - (current.top + current.height);
	} else if (current.top + current.height > base.top + base.height) {
		// no bottom
		top.top = base.top;
		top.height = current.top - base.top;
	} else {
		//both
		bottom.top = current.top + current.height;
		bottom.height = base.top + base.height - (current.top + current.height);
		top.top = base.top;
		top.height = current.top - base.top;
	}

	++begin;

	const static sf::IntRect z(0, 0, 0, 0);

	if (top.width >= minx && top.height >= miny)
		top = largest_rect(begin, end, top, minx, miny);
	else
		top = z;

	if (bottom.width >= minx && bottom.height >= miny)
		bottom = largest_rect(begin, end, bottom, minx, miny);
	else
		bottom = z;

	if (left.width >= minx && left.height >= miny)
		left = largest_rect(begin, end, left, minx, miny);
	else
		left = z;

	if (right.width >= minx && right.height >= miny)
		right = largest_rect(begin, end, right, minx, miny);
	else
		right = z;

	
	sf::IntRect* maxr = &top;

	if (rect_area(*maxr) < rect_area(bottom))
		maxr = &bottom;
	if (rect_area(*maxr) < rect_area(left))
		maxr = &left;
	if (rect_area(*maxr) < rect_area(right))
		maxr = &right;
	return *maxr;
}

void make_point_visible(glm::dvec2 texture_point, IN(OGLLock) mainwin) {
	auto openrect = global::uicontainer->largest_free_rect(20, 20);
	if (openrect.width == 0) {
		openrect = global::uicontainer->pos;
	}

	const sf::Vector2f worldPos = mainwin->mapPixelToCoords(sf::Vector2i(openrect.left + openrect.width / 2, openrect.top + openrect.height / 2));
	const glm::dvec3 source = texture_to_sphere(texture_point);

	bool nosolution = false;
	const auto rotation = project_sphere_to_map(source, glm::dvec2(worldPos.x, worldPos.y), nosolution);

	if (!nosolution) {
		global::horzrotation = rotation.x;
		global::vertrotation = rotation.y;
	} 

	//checking

	/*global::uicontainer->add_element<uiRect>(openrect.left + openrect.width / 2 - 2, openrect.top + openrect.height / 2 - 2, 4, 4, global::solid_border);

	const auto r_mat = generate_mat_transform(global::horzrotation, global::vertrotation);
	const auto newg = texture_to_globe(mappoint, r_mat);
	const auto newp = map_from_globe(newg);

	const sf::Vector2i pxpos = mainwin->mapCoordsToPixel(sf::Vector2f(newp.x, newp.y));

	global::uicontainer->add_element<uiRect>(pxpos.x - 4, pxpos.y - 4, 8, 8, global::solid_border);*/
	
}

void make_vec_visible(glm::dvec3 source, IN(OGLLock) mainwin) {
	auto openrect = global::uicontainer->largest_free_rect(20, 20);
	if (openrect.width == 0) {
		openrect = global::uicontainer->pos;
	}

	const sf::Vector2f worldPos = mainwin->mapPixelToCoords(sf::Vector2i(openrect.left + openrect.width / 2, openrect.top + openrect.height / 2));

	bool nosolution = false;
	const auto rotation = project_sphere_to_map(source, glm::dvec2(worldPos.x, worldPos.y), nosolution);

	if (!nosolution) {
		global::horzrotation = rotation.x;
		global::vertrotation = rotation.y;
	}
}

std::shared_ptr<uiElement> uiElement::top() {
	if (auto p = parent.lock())
		return p->top();
	return shared_from_this();
}

void uiElement::posabs(int & x, int & y) {
	x += pos.left;
	y += pos.top;
	if (auto p = parent.lock()) p->posabs(x, y);
}

void uiElement::poslocal(int & x, int & y, IN(std::shared_ptr<uiElement>) element) {
	x -= pos.left;
	y -= pos.top;
	if (auto p = parent.lock()) {
		if (p != element)
			p->poslocal(x, y, element);
	}
}

void uiElement::draw(OGLLock &target, int x, int y) {
	if (!visible)
		return;
	for (auto i = subelements.rbegin(); i != subelements.rend(); ++i) {
		(*i)->draw(target, (*i)->pos.left + x, (*i)->pos.top + y);
	}
}

bool uiElement::scroll(int x, int y, float amount) {
	if (!visible)
		return false;
	for (auto i = subelements.begin(); i != subelements.end(); ++i) {
		if (rectcontains((*i)->pos, x, y)) {
			if ((*i)->scroll(x - (*i)->pos.left, y - (*i)->pos.top, amount))
				return true;
		} 
	}
	return false;
}

bool uiElement::click(int x, int y) {
	if (!visible)
		return false;
	bool caught = false;
	for (auto i = subelements.begin(); i != subelements.end(); ++i) {
		if (rectcontains((*i)->pos, x, y)) {
			caught = (*i)->click(x - (*i)->pos.left, y - (*i)->pos.top);
			if (caught) {
				//for_each(++i, subelements.end(), [](auto &e) {e->clickoff(); });
				break;
			} 
		} else {
			//(*i)->clickoff();
		}
	}
	return caught;
}

bool uiElement::release(int x, int y) {
	if (!visible)
		return false;
	bool caught = false;
	for (auto i = subelements.begin(); i != subelements.end(); ++i) {
		if (rectcontains((*i)->pos, x, y)) {
			caught = (*i)->release(x - (*i)->pos.left, y - (*i)->pos.top);
			if (caught)
				break;
		}
	}

	for (auto i = capture.begin(); i != capture.end(); ++i) {
		if (const auto ptr = i->lock()) {
			int tx = x; int ty = y;
			ptr->poslocal(tx, ty, shared_from_this());
			ptr->release(tx, ty);
		}
	}

	return caught;
}

bool uiElement::rclick(int x, int y) {
	if (!visible)
		return false;
	bool caught = false;
	for (auto i = subelements.begin(); i != subelements.end(); ++i) {
		if (rectcontains((*i)->pos, x, y)) {
			caught = (*i)->rclick(x - (*i)->pos.left, y - (*i)->pos.top);
			if (caught) {
				//for_each(++i, subelements.end(), [](auto &e) {e->clickoff(); });
				break;
			} 
		} else {
			//(*i)->clickoff();
		}
	}
	return caught;
}

bool uiElement::rrelease(int x, int y) {
	if (!visible)
		return false;
	bool caught = false;
	for (auto i = subelements.begin(); i != subelements.end(); ++i) {
		if (rectcontains((*i)->pos, x, y)) {
			caught = (*i)->rrelease(x - (*i)->pos.left, y - (*i)->pos.top);
			if (caught)
				break;
		}
	}

	for (auto i = capture.rbegin(); i != capture.rend(); ++i) {
		if (const auto ptr = i->lock()) {
			int tx = x; int ty = y;
			ptr->poslocal(tx, ty, shared_from_this());
			ptr->rrelease(tx, ty);
		}
	}

	return caught;
}

bool uiElement::move(int x, int y) {
	if (!visible)
		return false;

	if (!selected) {
		selected = true;
		enter();
	}

	bool tripped = false;
	for (auto i = subelements.begin(); i != subelements.end(); ++i) {
		if (!tripped && rectcontains((*i)->pos, x, y) && (*i)->move(x - (*i)->pos.left, y - (*i)->pos.top)) {
			tripped = true;
		}
		else {
			if ((*i)->selected) {
				(*i)->leave();
			}
		}
	}


	for (IN(auto) in : capture) {
		if (const auto ptr = in.lock()) {
			int tx = x; int ty = y;
			ptr->poslocal(tx, ty, shared_from_this());
			ptr->move(tx, ty);
		}
	}
	return tripped;
}

bool uiElement::keypress(sf::Keyboard::Key k) {
	if (!visible)
		return false;

	for (auto i = subelements.begin(); i != subelements.end(); ++i) {
		if ((*i)->keypress(k))
			return true;
	}
	return false;
}

bool uiElement::textinput(unsigned int cvalue) {
	if (!visible)
		return false;

	for (auto i = subelements.begin(); i != subelements.end(); ++i) {
		if ((*i)->textinput(cvalue))
			return true;
	}
	return false;
}

void uiElement::enter() {

}

void uiElement::leave() {
	selected = false;
	for(IN(auto) i : subelements) {
		if (i->selected)
			i->leave();
	}
}

void uiElement::clickoff() {
	for_each(subelements.cbegin(), subelements.cend(), [](auto& e) {e->clickoff(); });
}

void uiElement::registerCapture(IN(std::weak_ptr<uiElement>) element) {
	global::uiqueue.push([th = shared_from_this(), e = std::weak_ptr<uiElement>(element)]{
		vector_erase_if(th->capture, [](IN(std::weak_ptr<uiElement>)i) { return i.expired(); });
		th->capture.push_back(e);
	});
}

void uiElement::releaseCapture(IN(std::weak_ptr<uiElement>) element) {
	global::uiqueue.push([th = shared_from_this(), e = std::weak_ptr<uiElement>(element)] {
		vector_erase_if(th->capture, [&e](IN(std::weak_ptr<uiElement>) wp) {return weak_equals(wp, e); });
	});
}

sf::IntRect uiElement::largest_free_rect(int maxx, int maxy) {
	sf::IntRect base(pos.left, pos.top, pos.width, pos.height);
	return largest_rect(subelements.begin(), subelements.end(), base, maxx, maxy);
}

std::shared_ptr<uiSimpleText> uiSimpleTooltip::shared_tooltip;
std::weak_ptr<uiSimpleTooltip> uiSimpleTooltip::active;


void makeWarDeclaration( title_id titleA, title_id titleB, unsigned char type) {
	size_t params[2] = {titleA, titleB};
	i18n_message_popup(TX_L_WAR_DEC, type == wargoal::WARGOAL_DEJURE ? TX_DJ_WAR_DEC : TX_CON_WAR_DEC, params, 2);
}

void makePeaceDeclaration( title_id titleA, title_id titleB) {
	size_t params[2] = {titleA, titleB};
	i18n_message_popup(TX_L_PEACE, TX_PEACE_DEC, params, 2);
}

void makeSeigeResults( title_id winnertitle, title_id losertitle, prov_id prov, unsigned int duration) {
	size_t params[4] = {losertitle, winnertitle, prov, duration};
	i18n_message_popup(TX_L_SEIGE, TX_SEIGE_END, params, 4);
}

void make_relationship_announcement( char_id a, char_id b, bool hate, bool gained) {
	size_t params[4] = {a, b, gained ? 0ui64 : 1ui64, hate ? 0ui64 : 1ui64};
	i18n_message_popup(TX_L_REL_CHANGE, TX_REL_CHANGE, params, 4);
}

std::shared_ptr<uiButton> character_selection_menu(IN(std::shared_ptr<uiElement>) parent, int x, int y, int w, int h, IN(std::wstring) text, IN(paint_region) paint, IN(text_format) format,  IN(std::function<void(cvector<char_id_t>&)>) get_list, IN(std::function<void(char_id_t)>) choice_made) {
	const auto menu = std::make_shared<uiMenuRect>(global::uicontainer, 0, 0, 210, 400, global::solid_border);
	const auto menu_contents = menu->add_element<uiScrollView>(0, 0, 210, 400);

	
	return parent->add_element<uiButton>(x, y, w, h, text, paint, format, [ get_list, choice_made, x, y, w, menu, menu_contents](uiButton* b) {
		global::uiqueue.push([ get_list, choice_made, x, y, w, menu, menu_contents, button = b->shared_from_this()] {
			menu_contents->subelements.clear();
			int yoff = 5;
			cvector<char_id_t> list;
			get_list(list);
			for (const auto id : list) {
				menu_contents->add_element<uiButton>(5, yoff, menu_contents->pos.width - 15, 20, global::w_character_name(id), global::solid_border, global::standard_text,
					[id, choice_made, mp = std::weak_ptr<uiMenuRect>(menu)](uiButton* b) {
					if (auto m = mp.lock())
						m->close();
					choice_made(id); });
				yoff += 25;
			}
			menu_contents->calcTotalHeight();
			if (const auto plk = button->parent.lock()) {
				int ax = 0; int ay = 0;
				plk->posabs(ax, ay);
				menu->move_to(ax + x + w, ay + y);
			}
			menu->open(global::uicontainer);
		});
	});
}

void makeBattleResults( title_id winnertitle, title_id losertitle, int winnerloss, int loserloss) {
	size_t params[4] = {winnertitle, losertitle, static_cast<size_t>(winnerloss), static_cast<size_t>(loserloss)};
	i18n_message_popup(TX_L_BAT_RESULTS, TX_BATTLE_RESULTS, params, 4);
}

template<typename T, typename U>
void generateLayout(INOUT(T) layout, sf::Font* font, unsigned int maxwidth, unsigned int csize, IN(U) sourcetext) {
	std::vector<sf::Text> strings;
	size_t next;

	float lineheight = font->getLineSpacing(csize) + csize / 4.0f;

	float cline = 0.0f;
	float cwidth = 0.0f;
	unsigned int indx = 0;

	for (size_t i = 0; i < sourcetext.size(); ++i) {
		IN(auto) txt = sourcetext[i];

		size_t prev = 0;
		size_t ipos = 0;
		next = txt.find_first_of(TEXT(' '));

		bool reset = false;

		while (prev != std::wstring::npos) {
			std::wstring news = ipos < txt.size() ? txt.substr(ipos, next - ipos) : TEXT("");

			sf::FloatRect bounds = font->getTextBounds(news, csize);
			if (next == std::wstring::npos && i < sourcetext.size() - 1) {
				sf::FloatRect extrabounds = font->getTextBounds(sourcetext[i + 1].substr(0,sourcetext[i+1].find_first_of(TEXT(' '))), csize);
				bounds.width += extrabounds.width;
			}

			if (cwidth + bounds.width > maxwidth) {
				if (cwidth == 0.0f && prev == 0) {
					layout.push_back({ indx, ipos, next, sf::Vector2f(cwidth, cline) });
					cwidth = 0.0f;
					cline += lineheight;
					ipos = txt.find_first_not_of(TEXT(' '), next);
				}
				else{
					if (prev != 0) {
						layout.push_back({ indx, ipos, prev, sf::Vector2f(cwidth, cline) });
						ipos = txt.find_first_not_of(TEXT(' '), prev);
					}

					cwidth = 0.0f;
					cline += lineheight;
					reset = true;
				}
			}

			if (!reset) {
				prev = next;
				next = txt.find_first_of(TEXT(' '), txt.find_first_not_of(TEXT(' '), next));
			}
			else {
				reset = false;
			}
		}

		if (ipos != std::wstring::npos) {
			sf::FloatRect bounds = font->getTextBounds(txt.substr(ipos, txt.size() - ipos), csize);
			layout.push_back({ indx, ipos, txt.size(), sf::Vector2f(cwidth, cline) });
			cwidth += bounds.width;
		}
		++indx;
	}
}

template<typename T, typename U, typename V>
void layouttotext(INOUT(T) elm, IN(U) layout, sf::Font* font, unsigned int maxwidth, unsigned int csize, IN(V) sourcetext, const sf::Color &color) {
	for (const auto & i : layout) {
		elm.emplace_back(sourcetext[i.stringindx].substr(i.firstchar, i.endchar - i.firstchar), *font,csize);
		elm.back().setPosition(floor(i.position.x), floor(i.position.y));
		elm.back().setColor(color);
	}
}

void open_window_centered(IN(std::shared_ptr<uiDragRect>) win) {
	win->setVisible(true);
	global::uiqueue.push([win] {
		win->setVisible(false);
		auto openrect = global::uicontainer->largest_free_rect(win->pos.width, win->pos.height);
		win->setVisible(true);

		if (openrect.width == 0) {
			openrect = global::uicontainer->pos;
		}

		win->pos.left = openrect.width / 2 + openrect.left - win->pos.width / 2;
		win->pos.top = openrect.height / 2 + openrect.top - win->pos.height / 2;

		win->toFront(global::uicontainer);
	});
}
int open_win_lastx = 5;
int open_win_lasty = 5;

void _inline_open_window_tiled(IN(std::shared_ptr<uiDragRect>) win, bool to_front) {
	win->setVisible(false);
	auto openrect = global::uicontainer->largest_free_rect(win->pos.width + 5, win->pos.height + 5);
	win->setVisible(true);

	if (openrect.width == 0) {
		win->pos.left = open_win_lastx;
		win->pos.top = open_win_lasty;
	} else {
		open_win_lastx = (win->pos.left = openrect.left + 5);
		open_win_lasty = (win->pos.top = openrect.top + 5);
	}

	if (to_front)
		win->toFront(global::uicontainer);
	else
		global::uicontainer->subelements.push_back(win);
}

void open_window_tiled(IN(std::shared_ptr<uiDragRect>) win, bool to_front) {
	win->setVisible(true);
	global::uiqueue.push([win, to_front] {
		_inline_open_window_tiled(win, to_front);
	});
}

#define MB_WIDTH 500
#define MB_HEIGHT 350
#define MB_BORDER 15
#define MB_TITLE 25
#define MB_BUTTON 30

auto standard_title(const std::wstring& title) {
	return [&title](const std::shared_ptr<uiDragRect>& dr){
		dr->add_element<uiCenterdText>(0, MB_BORDER, MB_WIDTH, title, global::empty, global::header_text);
	};
}

auto name_link_title(const std::wstring& label, char_id person) {
	return [&label, person](const std::shared_ptr<uiDragRect>& dr) {
		auto ttl = dr->add_element<uiSimpleText>(0, MB_BORDER, label, global::empty, global::header_text);
		auto hl = dr->add_element<uiHLink>(0, MB_BORDER, global::w_character_name(char_id_t(person)), global::empty, global::header_text, global::whandle, [ chid = person](uiHLink* b) {SetupChPane( char_id_t(chid)); });
		int prefwdth = static_cast<int>(global::font->getTextBounds(label, 18).width);
		int hwdth = static_cast<int>(hl->textBounds(global::font).width);
		ttl->pos.left = (MB_WIDTH - (prefwdth + hwdth)) / 2;
		hl->pos.left = ttl->pos.left + prefwdth;
	};
}

template <typename F1, typename F2>
void make_popup_rect(const std::shared_ptr<uiElement> &parent,  F1&& title_function, F2&& body_function) {
	static int offset = -50;
	const int loff = offset;
	const int xoff = (global::uicontainer->pos.width - MB_WIDTH) / 2 + loff;
	const int yoff = (global::uicontainer->pos.height - MB_HEIGHT) / 2 + loff;
	offset += 10;
	if (offset > 50) offset = -50;

	const auto dr = std::make_shared<uiDragRect>(parent, xoff, yoff, MB_WIDTH, MB_HEIGHT, global::solid_border);
	title_function(dr);
	body_function(dr);

	open_window_tiled(dr, false);
}

// void makePopup(const std::shared_ptr<uiElement> &parent, sf::Font* font, const std::wstring &title, size_t indx, size_t* params, size_t len) {
//	message_popup(parent, title, [params, len, indx](IN(std::shared_ptr<uiScrollView>) sv) {
//		create_tex_block(indx, params, len, sv, 0, 0, sv->pos.width - 10, global::empty, global::standard_text);
//	});
// }

void resize_popup(IN(std::shared_ptr<uiDragRect>) dr, IN(std::shared_ptr<uiScrollView>) sv) {
	if (sv->totalheight < sv->pos.height) {
		int shrunk_by = sv->pos.height - sv->totalheight;
		sv->pos.height = sv->totalheight;
		for (IN(auto) se : dr->subelements) {
			if (se->pos.top >= (sv->pos.top + sv->pos.height)) {
				se->pos.top -= shrunk_by;
			}
		}
		dr->pos.height -= shrunk_by;
	}
}

std::shared_ptr<uiDragRect> message_scroll;
std::shared_ptr<uiScrollView> message_scroll_contents;
std::shared_ptr<uiButton> message_scroll_close;
int mag_contents_y_off = 0;

constexpr int min_yoff = 30;

void init_message_box() {
	message_scroll = global::uicontainer->add_element<uiDragRect>(10, min_yoff, MB_WIDTH, MB_HEIGHT, global::solid_border);
	message_scroll_contents = message_scroll->add_element<uiScrollView>(MB_BORDER, MB_BORDER, MB_WIDTH - MB_BORDER * 2 + 10, MB_HEIGHT - MB_BORDER * 2 - MB_BUTTON);
	message_scroll_close = message_scroll->add_element<uiButton>(MB_WIDTH / 2 - 150 / 2, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 150, MB_BUTTON - 5, get_simple_string(TX_CLOSE), global::solid_border, global::header_text, [](uiButton*b){
		message_scroll->setVisible(false);
		global::uiqueue.push([]() {
			message_scroll_contents->subelements.clear();
			mag_contents_y_off = 0;
		});
	});
	message_scroll->setVisible(false);
}

void message_popup(const std::wstring &title, IN(std::function<void(const std::shared_ptr<uiElement>&)>) setup_contents) {
	const auto content = std::make_shared<uiElement>(0, 0, message_scroll_contents->pos.width, 0, message_scroll_contents);
	setup_contents(content);

	for (IN(auto) ce : content->subelements) {
		content->pos.height = std::max(content->pos.height, ce->pos.height + ce->pos.top);
	}

	global::uiqueue.push([content, title]() {
		const auto title_lbl = message_scroll_contents->add_element<uiCenterdText>(0, mag_contents_y_off, message_scroll_contents->pos.width, title, global::empty, global::header_text);
		mag_contents_y_off += (title_lbl->pos.height + 5);

		message_scroll_contents->subelements.push_back(content);

		content->pos.top = mag_contents_y_off;
		mag_contents_y_off += (content->pos.height + 5);

		const int yextent = std::min(global::uicontainer->pos.height - min_yoff, mag_contents_y_off + MB_BORDER * 2 + MB_BUTTON);

		message_scroll->pos.height = yextent;

		message_scroll_contents->pos.height = yextent - (MB_BORDER * 2 + MB_BUTTON);
		message_scroll_contents->calcTotalHeight();

		message_scroll_close->pos.top = yextent - (MB_BORDER + MB_BUTTON);

		message_scroll->setVisible(true);
		_inline_open_window_tiled(message_scroll, true);
	});
}

/*void message_popup(const std::shared_ptr<uiElement> &parent, const std::wstring &title,  const std::function<void(const std::shared_ptr<uiScrollView>&)> &setup_contents) {
	make_popup_rect(parent, standard_title(title), [&setup_contents, wp = std::weak_ptr<uiElement>(parent)](const std::shared_ptr<uiDragRect>& dr) {
		const auto sv = std::make_shared<uiScrollView>(MB_BORDER, MB_BORDER + MB_TITLE, MB_WIDTH - MB_BORDER * 2 + 10, MB_HEIGHT - MB_BORDER * 2 - MB_TITLE - MB_BUTTON, dr);
		dr->subelements.push_back(sv);
		dr->add_element<uiButton>(MB_WIDTH / 2 - 150 / 2, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 150, MB_BUTTON - 5, get_simple_string(TX_CLOSE), global::solid_border, global::header_text, [wdr = std::weak_ptr<uiDragRect>(dr), wp](uiButton*b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find(p->subelements.cbegin(), p->subelements.cend(), idr.lock())) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
		}, sf::Keyboard::Return);
		setup_contents(sv);
		sv->calcTotalHeight();
		resize_popup(dr, sv);
	});
}*/

void i18n_message_popup(size_t title_text, size_t body_text, const size_t* params, size_t numparams) {
	message_popup(get_simple_string(title_text), [body_text, params, numparams](IN(std::shared_ptr<uiElement>) sv) {
		create_tex_block(body_text, params, numparams, sv, 1, 1, sv->pos.width - 11, global::empty, global::standard_text);
	});
}

bool i18n_yes_no_popup(size_t title_text, size_t body_text, int stress_bias, const size_t* params, size_t numparams, const IN_P(size_t) labels) {
	return make_yes_no_popup_i(global::uicontainer, get_simple_string(title_text), [body_text, params, numparams](IN(std::shared_ptr<uiScrollView>) sv) {
		create_tex_block(body_text, params, numparams, sv, 1, 1, sv->pos.width - 11, global::empty, global::standard_text);
	}, stress_bias < 0 ? static_cast<size_t>(-stress_bias) : 0, stress_bias > 0 ? static_cast<size_t>(stress_bias) : 0, labels, std::integral_constant<size_t,1>());
}

/* std::shared_ptr<uiScrollView> makeEmptyPopup(const std::shared_ptr<uiElement> &parent, const std::wstring &title) {
	std::shared_ptr<uiScrollView> sv;
	make_popup_rect(parent, standard_title(title), [&sv, wp = std::weak_ptr<uiElement>(parent)](const std::shared_ptr<uiDragRect>& dr) {
		sv = std::make_shared<uiScrollView>(MB_BORDER, MB_BORDER + MB_TITLE, MB_WIDTH - MB_BORDER * 2 + 10, MB_HEIGHT - MB_BORDER * 2 - MB_TITLE - MB_BUTTON, dr);
		dr->subelements.push_back(sv);

		dr->add_element<uiButton>(MB_WIDTH / 2 - 150 / 2, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 150, MB_BUTTON - 5, get_simple_string(TX_CLOSE), global::solid_border, global::header_text, [wdr = std::weak_ptr<uiDragRect>(dr), wp](uiButton*b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find(p->subelements.cbegin(), p->subelements.cend(), idr.lock())) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
		}, sf::Keyboard::Return);
	});
	return sv;
} */

template <typename F1, typename F2>
void make_modeless_yes_no_popup_internal(const std::shared_ptr<uiElement> &parent, F1&& title_function, F2&& body_function, const size_t stress_yes, const size_t stress_no, std::function<void(bool)> result, const size_t* const labels) {
	make_popup_rect(parent, std::forward<F1>(title_function), [&body_function, labels, stress_yes, stress_no, &result, wp = std::weak_ptr<uiElement>(parent)](const std::shared_ptr<uiDragRect>& dr) {
		const auto y = dr->add_element<ui_stress_button>(MB_WIDTH / 2 - 100 - MB_BORDER, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[0]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_yes, global::tooltip_text, [result, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			result(true);
		}, sf::Keyboard::Y);

		const auto n = dr->add_element<ui_stress_button>(MB_WIDTH / 2 + MB_BORDER, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[1]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_no, global::tooltip_text, [result, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			result(false);
		}, sf::Keyboard::N);

		auto_apply(std::forward<F2>(body_function), dr, y, n);
	});
}

template <typename F1, typename F2>
bool make_yes_no_popup_internal(const std::shared_ptr<uiElement> &parent, F1&& title_function, F2&& body_function, const size_t stress_yes, const size_t stress_no, const size_t* const labels) {
	event signal;
	bool result = false;

	make_popup_rect(parent, std::forward<F1>(title_function), [&body_function, labels, stress_yes, stress_no, &signal, &result, wp = std::weak_ptr<uiElement>(parent)](const std::shared_ptr<uiDragRect>& dr) {
		const auto y = dr->add_element<ui_stress_button>(MB_WIDTH / 2 - 100 - MB_BORDER, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[0]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_yes, global::tooltip_text, [&signal, &result, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			result = true;
			signal.set();
		}, sf::Keyboard::Y);

		const auto n = dr->add_element<ui_stress_button>(MB_WIDTH / 2 + MB_BORDER, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[1]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_no, global::tooltip_text, [&signal, &result, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			result = false;
			signal.set();
		}, sf::Keyboard::N);

		
		//apply_n(std::forward<F2>(body_function), decltype(number_of_arguments(body_function))(), dr, y, n);
		auto_apply(std::forward<F2>(body_function), dr, y, n);
	});

	event* earray[] = {&signal, &global::quitevent};
	event::wait_for_multiple(earray, 2, false);

	return result;
}

void i18n_modeless_yes_no_popup(size_t title_text, size_t body_text, int stress_bias, const size_t* params, size_t numparams, std::function<void(bool)> results, const IN_P(size_t) labels) {
	make_modeless_yes_no_popup_internal(global::uicontainer, standard_title(get_simple_string(title_text)), [body_text,params, numparams](const std::shared_ptr<uiDragRect> &dr) {
		auto sv = std::make_shared<uiScrollView>(MB_BORDER, MB_BORDER + MB_TITLE, MB_WIDTH - MB_BORDER * 2 + 10, MB_HEIGHT - MB_BORDER * 2 - MB_TITLE - MB_BUTTON, dr);
		dr->subelements.push_back(sv);
		create_tex_block(body_text, params, numparams, sv, 1, 1, sv->pos.width - 11, global::empty, global::standard_text);
		sv->calcTotalHeight();
		resize_popup(dr, sv);
	}, stress_bias < 0 ? static_cast<size_t>(-stress_bias) : 0, stress_bias > 0 ? static_cast<size_t>(stress_bias) : 0, results, labels);
}

bool make_yes_no_popup_i(IN(std::shared_ptr<uiElement>) parent, IN(std::wstring) title, IN(std::function<void(const std::shared_ptr<uiScrollView>&)>)setup_contents, size_t stress_yes, size_t stress_no, const IN_P(size_t) labels, std::integral_constant<size_t, 1>) {
	return make_yes_no_popup_internal(parent, standard_title(title), [&setup_contents](const std::shared_ptr<uiDragRect> &dr) {
		auto sv = std::make_shared<uiScrollView>(MB_BORDER, MB_BORDER + MB_TITLE, MB_WIDTH - MB_BORDER * 2 + 10, MB_HEIGHT - MB_BORDER * 2 - MB_TITLE - MB_BUTTON, dr);
		dr->subelements.push_back(sv);
		setup_contents(sv);
		sv->calcTotalHeight();
		resize_popup(dr, sv);
	}, stress_yes, stress_no, labels);
}

bool make_yes_no_popup_i(IN(std::shared_ptr<uiElement>) parent, IN(std::wstring) title, IN(std::function<void(const std::shared_ptr<uiScrollView>&, const std::shared_ptr<ui_stress_button>&, const std::shared_ptr<ui_stress_button>&)>) setup_contents, size_t stress_yes, size_t stress_no, const IN_P(size_t) labels, std::integral_constant<size_t, 3>) {
	return make_yes_no_popup_internal(parent, standard_title(title), [&setup_contents](IN(std::shared_ptr<uiDragRect>) dr, IN(std::shared_ptr<ui_stress_button>) y, IN(std::shared_ptr<ui_stress_button>) n) {
		auto sv = std::make_shared<uiScrollView>(MB_BORDER, MB_BORDER + MB_TITLE, MB_WIDTH - MB_BORDER * 2 + 10, MB_HEIGHT - MB_BORDER * 2 - MB_TITLE - MB_BUTTON, dr);
		dr->subelements.push_back(sv);
		setup_contents(sv, y, n);
		sv->calcTotalHeight();
		resize_popup(dr, sv);
	}, stress_yes, stress_no, labels);
}

int make_trinary_popup(const std::shared_ptr<uiElement> &parent, const std::wstring& title, const std::function<void(const std::shared_ptr<uiScrollView>&)> &setup_contents, const size_t stress_yes, const size_t stress_maybe, const size_t stress_no, const size_t* const labels) {
	event signal;
	int result = 0;

	make_popup_rect(parent, standard_title(title), [&setup_contents, labels, stress_yes, stress_maybe, stress_no, &signal, &result, wp = std::weak_ptr<uiElement>(parent)](const std::shared_ptr<uiDragRect>& dr) {
		const auto sv = std::make_shared<uiScrollView>(MB_BORDER, MB_BORDER + MB_TITLE, MB_WIDTH - MB_BORDER * 2 + 10, MB_HEIGHT - MB_BORDER * 2 - MB_TITLE - MB_BUTTON, dr);
		dr->subelements.push_back(sv);
		setup_contents(sv);
		sv->calcTotalHeight();

		dr->add_element<ui_stress_button>(MB_WIDTH / 2 - 150 - MB_BORDER, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[0]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_yes, global::tooltip_text, [&signal, &result, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			result = 1;
			signal.set();
		}, sf::Keyboard::Y);

		dr->add_element<ui_stress_button>(MB_WIDTH / 2 - 50, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[1]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_maybe, global::tooltip_text, [&signal, &result, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			result = 0;
			signal.set();
		}, sf::Keyboard::M);

		dr->add_element<ui_stress_button>(MB_WIDTH / 2 + 50 + MB_BORDER, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[2]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_no, global::tooltip_text, [&signal, &result, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) b){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			result = -1;
			signal.set();
		}, sf::Keyboard::N);

		resize_popup(dr, sv);
	});

	event* earray[] = {&signal, &global::quitevent};
	event::wait_for_multiple(earray, 2, false);

	return result;
}

int make_trinary_popup(const std::shared_ptr<uiElement> &parent, const std::wstring& title, const std::function<void(const std::shared_ptr<uiScrollView>&)> &setup_contents, double positive_bias, const size_t* const labels) {
	const size_t stress_yes = positive_bias > 0.0 ? 0 : static_cast<size_t>(-positive_bias * 10.0);
	const size_t stress_no = positive_bias < 0.0 ? 0 : static_cast<size_t>(positive_bias * 10.0);
	const size_t stress_maybe = std::max(stress_yes, stress_no) / 2;
	return make_trinary_popup(parent, title, setup_contents, stress_yes, stress_maybe, stress_no, labels);
}

void modeless_trinary_popup(IN(std::shared_ptr<uiElement>) parent, IN(std::wstring) title, IN(std::function<void(const std::shared_ptr<uiScrollView>&)>) setup_contents, IN(std::function<void(int)>) results, const size_t stress_yes, const size_t stress_maybe, const size_t stress_no, const size_t* const labels) {
	make_popup_rect(parent, standard_title(title), [&setup_contents, results, labels, stress_yes, stress_maybe, stress_no, wp = std::weak_ptr<uiElement>(parent)](const std::shared_ptr<uiDragRect>& dr) {
		const auto sv = std::make_shared<uiScrollView>(MB_BORDER, MB_BORDER + MB_TITLE, MB_WIDTH - MB_BORDER * 2 + 10, MB_HEIGHT - MB_BORDER * 2 - MB_TITLE - MB_BUTTON, dr);
		dr->subelements.push_back(sv);
		setup_contents(sv);
		sv->calcTotalHeight();

		dr->add_element<ui_stress_button>(MB_WIDTH / 2 - 150 - MB_BORDER, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[0]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_yes, global::tooltip_text, [results, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) ){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			results(1);
		}, sf::Keyboard::Y);

		dr->add_element<ui_stress_button>(MB_WIDTH / 2 - 50, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[1]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_maybe, global::tooltip_text, [results, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) ){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			results(0);
		}, sf::Keyboard::M);

		dr->add_element<ui_stress_button>(MB_WIDTH / 2 + 50 + MB_BORDER, MB_HEIGHT - MB_BORDER - MB_BUTTON + 5, 100, MB_BUTTON - 5, get_simple_string(labels[2]), paint_states<2>(global::solid_border, global::disabled_fill), global::header_text, stress_no, global::tooltip_text, [results, wdr = std::weak_ptr<uiDragRect>(dr), wp](IN_P(ui_stress_button::button_type) ){
			global::uiqueue.push([idr = wdr, wp]() {
				if (auto p = wp.lock()) {
					decltype(p->subelements.cend()) it;
					if ((it = std::find_if(p->subelements.cbegin(), p->subelements.cend(), [dr = idr](IN(std::shared_ptr<uiElement>) e) { return weak_equals(e, dr); })) != p->subelements.cend())
						p->subelements.erase(it);
				}
			});
			results(-1);
		}, sf::Keyboard::N);

		resize_popup(dr, sv);
	});
}

#define ACT_MENU_W 200
#define ACT_MENU_H 300

void createChActionMenu(std::shared_ptr<uiElement>& p, int x, int y, char_id id) {
	static const sf::Color half(0, 0, 0, 128);
	static const sf::Color mback(200, 200, 200);

	if (x + ACT_MENU_W > p->pos.width) x = p->pos.width - ACT_MENU_W;
	if (y + ACT_MENU_H > p->pos.height) y = p->pos.height - ACT_MENU_H;

	const std::shared_ptr<uiMenuRect> m = std::make_shared<uiMenuRect>(p, x,y, ACT_MENU_W, ACT_MENU_H, global::solid_border);
	
	std::shared_ptr<uiScrollView> sv;
	m->subelements.push_back(sv = std::make_shared<uiScrollView>(0, 0, ACT_MENU_W, ACT_MENU_H, m));

	if (global::playerid != char_id_t(id)) {
		const std::shared_ptr<ui_button_disable> tmp =
		sv->add_element<ui_button_disable>(5, 5, ACT_MENU_W - 10, 20, get_simple_string(TX_OPT_DEC_WAR), paint_states<2>(global::solid_border, global::disabled_fill), global::standard_text, global::tooltip_text, [menu = std::weak_ptr<uiMenuRect>(m), id](ui_button_disable* b) {
			if (auto m = menu.lock())
				m->close();
			global::actionlist.add_new<nocbwar>(global::playerid, char_id_t(id));
		}, sf::Keyboard::A);

		r_lock l;
		if (! nocbwar(global::playerid, char_id_t(id)).possible( l)) {
			tmp->disable(get_simple_string(TX_NO_WAR_CON));
		}
	} else {
		sv->add_element<uiButton>(5, 5, ACT_MENU_W - 10, 20, get_simple_string(TX_OPT_HOST), global::solid_border, global::standard_text, [menu = std::weak_ptr<uiMenuRect>(m)](uiButton* b) {
			if (auto m = menu.lock())
				m->close();
			SetupEventWindow(global::playerid);
		}, sf::Keyboard::A);
	}

	global::uiqueue.push([ m, p] {
		m->open(p);
	});
}

void uiDragRect::toFront(IN(std::shared_ptr<uiElement>) p) {
	const auto it = std::find(p->subelements.begin(), p->subelements.end(), shared_from_this());
	if (it != p->subelements.cend()) {
		p->subelements.erase(it);
		p->subelements.insert(p->subelements.begin(), shared_from_this());
	}
}

bool uiDragRect::click(int x, int y) {
	if (visible) {
		if (!uiElement::click(x, y)) {
			top()->registerCapture(shared_from_this());
			dragging = true;
			basex = x;
			basey = y;

			global::uiqueue.push([par = parent, wthis = std::weak_ptr<uiDragRect>(std::static_pointer_cast<uiDragRect>(shared_from_this()))]{
				if (auto th = wthis.lock()) {
					if(auto p = par.lock())
						th->toFront(p);
				}
			});
		}
		return true;
	}
	return false;
}

bool uiDragRect::release(int x, int y) {
	if (visible) {
		uiElement::release(x, y);
		if (dragging) {
			dragging = false;
			top()->releaseCapture(shared_from_this());
		}
		return true;
	}
	return false;
}

bool uiDragRect::move(int x, int y) {
	if (visible) {
		uiElement::move(x, y);
		if (dragging) {
			pos.left += x - basex;
			pos.top += y - basey;
			if (pos.left < 0) pos.left = 0;
			if (pos.top < 0) pos.top = 0;
			if (auto p = parent.lock()) {
				if (pos.left + pos.width > p->pos.width) pos.left = p->pos.width - pos.width;
				if (pos.top + pos.height > p->pos.height) pos.top = p->pos.height - pos.height;
			}

			for (IN(auto) i : subelements) {
				i->updatepos();
			}
			redraw();
		}
		return true;
	}
	return false;
}

using namespace std;



void UpdateChIcon( const std::shared_ptr<uiGButton> &b, char_id_t chid, int icosize) {
	if (!valid_ids(chid)) {
		b->setVisible(false);
		return;
	}
	b->setVisible(true);
	
	IN(auto) p = get_object(chid);
	IN(auto) pt = get_object(p.primetitle);

	std::wstring honr(global::w_character_name(chid));

	int titletype = pt.type;

	if (valid_ids(p.primetitle) & (titletype != 0)) {
		honr += TEXT(", ");
		honr += global::get_composed_title(p.primetitle, titletype);
	}
	else {
		titletype = 6;
	}

	b->tt_text = honr;
	if (icosize == 64) {
		b->paint.set_ptr(0, global::bigcrowns_tex.get(6 - titletype, p.gender + (p.died != 0 ? 2 : 0)));
	} else {
		b->paint.set_ptr(0, global::smallcrowns_tex.get(6 - titletype, p.gender + (p.died != 0 ? 2 : 0)));
	}
	b->clickaction = [=](uiGButton *a) {SetupChPane(char_id_t(chid)); };
}

std::shared_ptr<uiGButton> generateButton( char_id_t chid, const std::shared_ptr<uiElement> &parent, int x, int y, bool large) {
	if (!valid_ids(chid)) {
		return parent->add_element<uiGButton>(x, y, 40, 40, global::smallcrowns_tex.get(0, 0), L"NO PERSON", global::tooltip_text, [chid](uiGButton *a) {SetupChPane(chid); });
	}
	IN(auto) p = get_object(chid);
	IN(auto) pt = get_object(p.primetitle);

	std::wstring honr(global::w_character_name(chid));
	int titletype = pt.type;

	if (valid_ids(p.primetitle) & (titletype != 0)) {
		honr += TEXT(", ");
		honr += global::get_composed_title(p.primetitle, titletype);
	} else {
		titletype = 6;
	}

	if (large)
		return parent->add_element<uiGButton>(x, y, 64, 64, global::bigcrowns_tex.get(6 - titletype, p.gender + (p.died != 0 ? 2 : 0)), honr, global::tooltip_text, [ chid](uiGButton *a) {SetupChPane( chid); });
	else
		return parent->add_element<uiGButton>(x, y, 40, 40, global::smallcrowns_tex.get(6 - titletype, p.gender + (p.died != 0 ? 2 : 0)), honr, global::tooltip_text, [ chid](uiGButton *a) {SetupChPane( chid); });
}

std::shared_ptr<uiGButton> generateTButton(title_id tid, const std::shared_ptr<uiElement> &parent, int x, int y, bool large) noexcept {
	return generateTButton(title_id_t(tid), parent, x, y, large);
}
std::shared_ptr<uiGButton> generateTButton(title_id_t tid, const std::shared_ptr<uiElement> &parent, int x, int y, bool large) noexcept {
	if (!valid_ids(tid)) {
		if (large)
			return parent->add_element<uiGButton>(x, y, 64, 64, global::bigcrowns_tex.get(0, 4), L"NO TITLE", global::tooltip_text, [tid](uiGButton *a) {SetupTPane(tid); });
		else
			return parent->add_element<uiGButton>(x, y, 40, 40, global::smallcrowns_tex.get(0, 4), L"NO TITLE", global::tooltip_text, [tid](uiGButton *a) {SetupTPane(tid); });
	}

	if (large)
		return parent->add_element<uiGButton>(x, y, 64, 64, global::bigcrowns_tex.get(6 - get_object(tid).type, 4), global::w_title_name(tid), global::tooltip_text, [ tid](uiGButton *a) {SetupTPane( tid); });
	else
		return parent->add_element<uiGButton>(x, y, 40, 40, global::smallcrowns_tex.get(6 - get_object(tid).type, 4), global::w_title_name(tid), global::tooltip_text, [ tid](uiGButton *a) {SetupTPane( tid); });
}

void UpdateTIcon( const std::shared_ptr<uiGButton> &b, title_id_t id, int icosize) {
	if (!valid_ids(id)) {
		b->setVisible(false);
		return;
	}
	b->setVisible(true);

	b->tt_text = global::w_title_name(id);
	if (icosize == 64) {
		b->paint.set_ptr(0, global::bigcrowns_tex.get(6 - get_object(id).type, 4));
	} else {
		b->paint.set_ptr(0, global::smallcrowns_tex.get(6 - get_object(id).type, 4));
	}
	b->clickaction = [id](uiGButton *a) {SetupTPane( id); };
}

void uiElement::setVisible(bool v) {
	visible = v;
	redraw();
}

bool uiElement::gVisible() { return visible; }


void uiRect::draw(OGLLock & target, int x, int y) {
	if (visible) {
		fill.draw(target, x, y, pos.width, pos.height);
		uiElement::draw(target, x, y);
	}
}

bool uiRect::click(int x, int y) {
	uiVRect::click(x, y);
	return visible;
}

bool uiRect::release(int x, int y) {
	uiVRect::release(x, y);
	return visible;
}

bool uiRect::rclick(int x, int y) {
	uiVRect::rclick(x, y);
	return visible;
}

bool uiRect::rrelease(int x, int y) {
	uiVRect::rrelease(x, y);
	return visible;
}

bool uiRect::move(int x, int y) {
	uiVRect::move(x, y);
	return visible;
}

bool uiInvisibleButton::move(int x, int y) {
	uiElement::move(x, y);
	return true;
}

void uiInvisibleButton::enter() {
	//SetCursor(LoadCursor(NULL, IDC_HAND));
	SetClassLongPtr(wnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_HAND)));
	redraw();
}

void uiInvisibleButton::leave() {
	//SetCursor(LoadCursor(NULL, IDC_ARROW));
	SetClassLongPtr(wnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_ARROW)));
	redraw();
	uiElement::leave();
}

bool uiInvisibleButton::click(int x, int y) {
	uiElement::click(x, y);
	selected = false;
	//SetCursor(LoadCursor(NULL, IDC_ARROW));
	SetClassLongPtr(wnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_ARROW)));
	clickaction(this);
	return true;
}

void uiHLink::enter() {
	if (visible) {
		SetClassLongPtr(wnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_HAND)));
		redraw();
	}
}

void uiHLink::leave() {
	if (visible) {
		SetClassLongPtr(wnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_ARROW)));
		redraw();
	}
	uiElement::leave();
}

bool uiHLink::click(int x, int y) {
	if (visible) {
		uiElement::click(x, y);
		selected = false;
		//SetCursor(LoadCursor(NULL, IDC_ARROW));
		SetClassLongPtr(wnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_ARROW)));
		clickaction(this);
		return true;
	}
	return false;
}

void uiFloatRect::innerPosAdjustment() {
	if (auto p = parent.lock()) {
		pos.left = static_cast<int>((mxoff >= 0 ? mxoff : (p->pos.width + mxoff)) + p->pos.width * mwpercent);
		pos.top = static_cast<int>((myoff >= 0 ? myoff : (p->pos.height + myoff)) + p->pos.height * mhpercent);
	}
}

void uiFloatRect::updatepos() {
	innerPosAdjustment();
	uiElement::updatepos();
	redraw();
}


uiHozContainer::uiHozContainer(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, int ihmargin, IN(std::wstring) str, IN(paint_region) paint, IN(text_format) format) : hmargin(ihmargin), fill(paint), text(str, *(format.font), format.csize), uiVRect(ix, iy, iwidth, iheight, p) {
	text.setColor(format.color);
}

void uiHozContainer::RecalcPos() {
	const sf::FloatRect bnd = const_cast<sf::Font*>(text.getFont())->getTextBounds(text.getString(), text.getCharacterSize());
	const int tmarg = static_cast<int>(bnd.height);

	if (subelements.size() > 1) {
		IN(std::shared_ptr<uiElement>) e = *(subelements.rbegin());
		int totalwidth = -5;
		for (IN(auto) i : subelements) {
			totalwidth += i->pos.width + 5;
		}
		int available = pos.width - hmargin * 2;

		if (available >= totalwidth) {
			int current = hmargin;
			for (IN(auto) i : subelements) {
				i->pos.left = current;
				i->pos.top = tmarg + std::max(0, ((pos.height - tmarg) - i->pos.height) / 2);
				current += i->pos.width + 5;
				i->updatepos();
			}
		} else {
			float ratio = static_cast<float>(available) / static_cast<float>(totalwidth);
			float current = static_cast<float>(hmargin);
			for (IN(auto) i : subelements) {
				i->pos.left = static_cast<int>(current);
				i->pos.top = tmarg + std::max(0, ((pos.height - tmarg) - i->pos.height) / 2);
				current += i->pos.width * ratio;

				i->updatepos();
			}
		}
		
	}
	else if (subelements.size() == 1) {
		(*subelements.begin())->pos.left = hmargin;
		(*subelements.begin())->pos.top = tmarg + std::max(0, ((pos.height - tmarg) - (*subelements.begin())->pos.height) / 2);
		(*subelements.begin())->updatepos();
	}
}

void uiHozContainer::draw(OGLLock & target, int x, int y) {
	if (gVisible()) {
		fill.draw(target, x, y, pos.width, pos.height);
		text.setPosition(x, y);
		target->draw(text);
		uiVRect::draw(target, x, y);
	}
}

uiSimpleText::uiSimpleText(std::weak_ptr<uiElement> p, int x, int y, IN(std::wstring) str, IN(paint_region) paint, IN(text_format) format) : fill(paint), text(str, *(format.font), format.csize), uiVRect(x, y, 1, 1, p) {
	text.setColor(format.color);
	updatebounds();
}

void uiSimpleText::draw(OGLLock & target, int x, int y) {
	if (gVisible()) {
		fill.draw(target, x, y, pos.width, pos.height);

		sf::FloatRect bnd = textBounds(const_cast<sf::Font*>(text.getFont()));
		text.setPosition(static_cast<int> (x - bnd.left + margin), static_cast<int> (y + margin));
		target->draw(text);
		uiVRect::draw(target, x, y);
	}
}

bool uiSimpleText::move(int x, int y) {
	return false;
}

sf::FloatRect uiSimpleText::textBounds(const sf::Font * const f) {
	return const_cast<sf::Font*>(f)->getTextBounds(text.getString(),text.getCharacterSize());
}

void uiSimpleText::updateText(IN(std::wstring) txt) {
	text.setString(txt);
	updatebounds();
}

void uiSimpleText::updatebounds() {
	sf::FloatRect r = textBounds(const_cast<sf::Font*>(text.getFont()));
	pos.width = static_cast<int>(r.width) + margin * 2;
	pos.height = std::max(static_cast<int>(text.getFont()->getLineSpacing(text.getCharacterSize()) * 1.2f), static_cast<int>(r.height + r.top)) + 2 * margin;
}

sf::FloatRect uiCenterdText::textBounds(sf::Font * const f) {
	return f->getTextBounds(text.getString(), text.getCharacterSize());
}

uiCenterdText::uiCenterdText(std::weak_ptr<uiElement> p, int x, int y, int w, IN(std::wstring) str, IN(paint_region) paint, IN(text_format) format) : text(str, *format.font, format.csize), fill(paint), uiElement(x, y, w, 1, p) {
	text.setColor(format.color);
	sf::FloatRect r = textBounds(const_cast<sf::Font*>(text.getFont()));
	pos.height = static_cast<int>(r.height);
}

void uiCenterdText::draw(OGLLock & target, int x, int y) {
	fill.draw(target, x, y, pos.width, pos.height);
	sf::FloatRect bnd = textBounds(const_cast<sf::Font*>(text.getFont()));
	text.setPosition(static_cast<int> (x + (pos.width  - bnd.width)/2), y);
	target->draw(text);
	uiElement::draw(target, x, y);
}

uiTextBlock::uiTextBlock(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, INOUT(std::vector<layoutelement>) layout, IN(paint_region) paint, IN(text_format) format, IN(std::vector<std::wstring>) sourcetext) : fill(paint), uiVRect(ix, iy, iwidth, 0, p) {
	generateLayout(layout, format.font, iwidth, format.csize, sourcetext);
	layouttotext(textelements, layout, format.font, iwidth, format.csize, sourcetext, format.color);
	const float lineheight = format.font->getLineSpacing(format.csize) + format.csize / 4.0f;
	pos.height = static_cast<unsigned int>(layout[layout.size() - 1].position.y + lineheight);
}

uiTextBlock::uiTextBlock(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, INOUT(cvector<layoutelement>) layout, IN(paint_region) paint, IN(text_format) format, IN(cvector<std::wstring>) sourcetext) : fill(paint), uiVRect(ix, iy, iwidth, 0, p) {
	generateLayout(layout, format.font, iwidth, format.csize, sourcetext);
	layouttotext(textelements, layout, format.font, iwidth, format.csize, sourcetext, format.color);
	const float lineheight = format.font->getLineSpacing(format.csize) + format.csize / 4.0f;
	pos.height = static_cast<unsigned int>(layout[layout.size() - 1].position.y + lineheight);
}

void uiTextBlock::addLink(const size_t index, IN(std::vector<layoutelement>) layout,  IN(std::function<void(uiInvisibleButton*)>) action) {
	 for (size_t indx = 0; indx < layout.size(); ++indx) {
		 if (layout[indx].stringindx == index) {
			 const auto bounds = const_cast<sf::Font*>(textelements[indx].getFont())->getTextBounds(textelements[indx].getString(), textelements[indx].getCharacterSize());
			 textelements[indx].setColor(sf::Color::Blue);
			 subelements.push_back(std::make_shared<uiInvisibleButton>(shared_from_this(), static_cast<int>(layout[indx].position.x), static_cast<int>(layout[indx].position.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height), global::whandle, action));
		 }
	 }
}

void uiTextBlock::addLink(const size_t index, IN(cvector<layoutelement>) layout, IN(std::function<void(uiInvisibleButton*)>) action) {
	for (size_t indx = 0; indx < layout.size(); ++indx) {
		if (layout[indx].stringindx == index) {
			const auto bounds = const_cast<sf::Font*>(textelements[indx].getFont())->getTextBounds(textelements[indx].getString(), textelements[indx].getCharacterSize());
			textelements[indx].setColor(sf::Color::Blue);
			subelements.push_back(std::make_shared<uiInvisibleButton>(shared_from_this(), static_cast<int>(layout[indx].position.x), static_cast<int>(layout[indx].position.y), static_cast<int>(bounds.width), static_cast<int>(bounds.height), global::whandle, action));
		}
	}
}

void uiTextBlock::draw(OGLLock & target, int x, int y) {
	if (visible) {
		fill.draw(target, x, y, pos.width, pos.height);
		for (auto &txt : textelements) {
			const sf::Vector2f opos = txt.getPosition();
			txt.setPosition(opos.x + x, opos.y + y);
			target->draw(txt);
			txt.setPosition(opos);
		}
		uiElement::draw(target, x, y);

	}
}

void uiScrollView::calcTotalHeight() {
	totalheight = 0;
	for (const auto &i : subelements) {
		totalheight = std::max(totalheight, i->pos.top + i->pos.height);
	}
	totalheight += margin;

	if (scrolloff > (totalheight - pos.height)) {
		scrolloff = std::max(0, totalheight - pos.height);
	}
}

bool uiScrollView::scroll(int x, int y, float amount) {
	 if (visible && !uiVRect::scroll(x, y + scrolloff, amount) &&(totalheight - pos.height > 0 || scrolloff != 0)) {
		 int diff = static_cast<int>(amount * 20.0f);

		 if (-diff > scrolloff)
			 diff = -scrolloff;
		 else if (scrolloff + diff > totalheight - pos.height)
			 diff = (totalheight - pos.height) - scrolloff;

		 scrolloff += diff;
		 for (auto i : subelements) {
			 i->updatepos();
		 }

		 redraw();
		 return true;
	 }
	 return uiVRect::scroll(x,y,amount);
}

void uiScrollView::posabs(int & x, int & y) {
	x += pos.left;
	y += pos.top - scrolloff;
	if (auto p = parent.lock()) p->posabs(x, y);
}

bool uiScrollView::rclick(int x, int y) {
	return uiVRect::rclick(x, y + scrolloff);;
}

bool uiScrollView::rrelease(int x, int y) {
	return uiVRect::rrelease(x, y + scrolloff);
}

void uiScrollView::reset() {
	subelements.clear();
	scrolloff = 0;
	totalheight = 0;
}

void uiScrollView::draw(OGLLock& target, int x, int y) {
	if (gVisible()) {
		const GLboolean wasenabled = glIsEnabled(GL_SCISSOR_TEST);
		GLfloat oldbox[4];
		if (wasenabled == GL_FALSE)
			glEnable(GL_SCISSOR_TEST);
		else
			glGetFloatv(GL_SCISSOR_BOX, oldbox);

		glScissor(x, top()->pos.height - y - pos.height, pos.width, pos.height);
		uiVRect::draw(target, x, y - scrolloff);

		if (wasenabled == GL_FALSE)
			glDisable(GL_SCISSOR_TEST);
		else
			glScissor(static_cast<GLint>(oldbox[0]), static_cast<GLint>(oldbox[1]), static_cast<GLsizei>(oldbox[2]), static_cast<GLsizei>(oldbox[3]));

		if (totalheight - pos.height > 0) {
			sf::RectangleShape rs(sf::Vector2f(10, 10));
			rs.setFillColor(sf::Color::Green);
			rs.setPosition(static_cast<float>(pos.width + x - 10), static_cast<float>(y));
			target->draw(rs);
			rs.setPosition(static_cast<float>(pos.width + x - 10), static_cast<float>(pos.height + y - 10));
			target->draw(rs);
			rs.setFillColor(sf::Color::Blue);

			float off = (float)(pos.height - 30) * (float)scrolloff / (float)(totalheight - pos.height);
			rs.setPosition(static_cast<float>(pos.width - 10) + x, static_cast<float>(10 + off) + y);
			target->draw(rs);
		}

	}

}

bool uiScrollView::click(int x, int y) {
	if (gVisible() && totalheight - pos.height > 0) {
		float off = (float)(pos.height - 30) * (float)scrolloff / (float)(totalheight - pos.height);
		if (x >= pos.width - 10 && y >= 10 + off && y <= 20 + off) {
			lasty = y; dragging = true;
			top()->registerCapture(shared_from_this());
			return true;
		}
	}
	return uiVRect::click(x , y + scrolloff);
}

bool uiScrollView::release(int x, int y) {
	if (dragging) {
		dragging = false; top()->releaseCapture(shared_from_this());
		return true;
	}
	return uiVRect::release(x, y + scrolloff);
}

bool uiScrollView::move(int x, int y) {
	if (dragging) {
		if (y != lasty) {
			int diff = y - lasty;
			int moveto = scrolloff + diff;

			int nscrolloff = static_cast<int>(((float)(y - 15) / (float)(pos.height - 30) * (float)(totalheight - pos.height)));
			if (nscrolloff < 0) nscrolloff = 0;
			if (nscrolloff >(totalheight - pos.height))  nscrolloff = (totalheight - pos.height);
			diff = nscrolloff - scrolloff;

			scrolloff += diff;

			lasty = y;;
			ready = false;
		}
	}
	return uiVRect::move(x, y + scrolloff);
}

bool uiSlider::click(int x, int y) {
	if (x > ((pos.width - pos.height) / (float)(smax - 1)) * cpos && x - pos.height < ((pos.width - pos.height) / (float)(smax - 1)) * cpos) {
		dragging = true;
		icpos = cpos;
		top()->registerCapture(shared_from_this());
	}
	uiElement::click(x, y);
	return true;
}

bool uiSlider::release(int x, int y) {
	if (dragging) {
		dragging = false; top()->releaseCapture(shared_from_this());
		if (icpos != cpos)
			slideaction(this, cpos);
	}
	uiElement::release(x, y);
	return true;
}

bool uiSlider::move(int x, int y) {
	if (dragging) {
		for (int i = 0; i < smax; i++) {
			if (x >((pos.width - pos.height) / (float)(smax - 1)) * i && x - pos.height < ((pos.width - pos.height) / (float)(smax - 1)) * i) {
				cpos = i;
				break;
			}
		}
		redraw();
	}
	uiElement::move(x, y);
	return true;
}

void uiSlider::draw(OGLLock & target, int x, int y) {
	sf::RectangleShape bar(sf::Vector2f(static_cast<float>(pos.width), 4.0f));
	bar.setFillColor(sf::Color::Black);
	bar.setPosition(static_cast<float>(x), static_cast<float>(y + pos.height / 2 - 2));
	target->draw(bar);
	sf::RectangleShape rs(sf::Vector2f(static_cast<float>(pos.height), static_cast<float>(pos.height)));
	rs.setFillColor(sf::Color::Green);
	rs.setPosition(((pos.width - pos.height) / (float)(smax - 1)) * cpos + x, static_cast<float>(y));
	target->draw(rs);
}

paint_rect tt_back(sf::Color::Black);

void uiSimpleTooltip::init_shared_tooltip(std::weak_ptr<uiElement> p, IN(text_format) format) {
	if (auto pr = p.lock()) {
		shared_tooltip = make_shared<uiSimpleText>(p, 0, 0, TEXT(""), tt_back, format);
		shared_tooltip->setVisible(false);
		shared_tooltip->margin = 6;
		pr->subelements.push_back(shared_tooltip);
	}
}

uiSimpleTooltip::uiSimpleTooltip(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(std::wstring) str, IN(text_format) format, bool ibelow) :
	uiVRect(ix, iy, iwidth, iheight, p), tt_text(str), below(ibelow) {
	if (!shared_tooltip) {
	}
	updatepos();

}

void uiSimpleTooltip::update_tooltip(IN(std::wstring) text) {
	 tt_text = text;
}

void uiSimpleTooltip::enter() {
	 if (gVisible()) {
		 shared_tooltip->updateText(tt_text);
		 active = static_pointer_cast<uiSimpleTooltip>(shared_from_this());

		 int absx = 0;
		 int absy = 0;
		 posabs(absx, absy);
		 if (!below) {
			 if (absx + pos.width + shared_tooltip->pos.width > top()->pos.width)
				 absx = absx - shared_tooltip->pos.width;
			 else
				 absx = absx + pos.width;
			 if (absy + shared_tooltip->pos.height > top()->pos.height)
				 absy = top()->pos.height - shared_tooltip->pos.height;
		 } else {
			 if (absy + pos.height + shared_tooltip->pos.height > top()->pos.height)
				 absy = absy - shared_tooltip->pos.height;
			 else
				 absy = absy + pos.height;
			 if (absx + shared_tooltip->pos.width > top()->pos.width)
				 absx = top()->pos.width - shared_tooltip->pos.width;
		 }
		 shared_tooltip->pos.left = absx;
		 shared_tooltip->pos.top = absy;
	 }

	 shared_tooltip->setVisible(tt_text.length() != 0 && gVisible());
}

void uiSimpleTooltip::leave() {
	 if (active.lock() == shared_from_this()) {
		 shared_tooltip->setVisible(false);
		 active.reset();
	 }
	 uiElement::leave();
}

void uiSimpleTooltip::updatepos() {
	uiVRect::updatepos();
}

bool uiSimpleTooltip::move(int x, int y) {
	uiVRect::move(x, y);
	return visible;
}

void uiIcon::draw(OGLLock& target, int x, int y) {
	if (visible) {
		sf::Sprite image(*tex, texbox);
		image.setPosition(static_cast<float>(x), static_cast<float>(y));
		target->draw(image);
	}
}

uiPropList::uiPropList(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, IN(paint_region) paint, IN(text_format) format, IN(std::vector<std::wstring>) iproperties) : fill(paint), uiElement(ix, iy, iwidth, 0, p), properties(iproperties), ctext("", *format.font, format.csize) {
	ctext.setColor(format.color);
	lineheight = format.font->getLineSpacing(format.csize);
	pos.height = static_cast<int>(lineheight * properties.size());
	values.resize(properties.size());
}

void uiPropList::draw(OGLLock& target, int x, int y) {
	fill.draw(target, x, y, pos.width, pos.height);
	float maxwidth = 0;
	for (unsigned int i = 0; i < properties.size(); i++) {
		ctext.setString(properties[i]);
		ctext.setPosition(x, y + static_cast<int>(i * lineheight));
		maxwidth = std::max(maxwidth, ctext.textBounds().width);
		target->draw(ctext);
	}
	for (unsigned int i = 0; i < properties.size(); i++) {
		ctext.setString(values[i]);
		ctext.setPosition(x + static_cast<int>(maxwidth) + 20, y + static_cast<int>(i * lineheight));
		target->draw(ctext);
	}
	uiElement::draw(target, x, y);
}

void uiBar::draw(OGLLock & target, int x, int y) {
	fill.draw(target, x, y, barpos, pos.height);

	sf::RectangleShape rs(sf::Vector2f(static_cast<float>(pos.width), static_cast<float>(pos.height)));
	rs.setOutlineColor(sf::Color::Black);
	rs.setOutlineThickness(1);
	rs.setFillColor(sf::Color(0, 0, 0, 0));
	rs.setPosition(static_cast<float>(x), static_cast<float>(y));
	target->draw(rs);

	uiElement::draw(target, x, y);
}

bool uiBar::click(int x, int y) {
	barpos = x;
	clickaction(this, static_cast<double>(x)/static_cast<double>(pos.width));
	redraw();
	return true;
}

ui_ch_hlink::ui_ch_hlink(std::weak_ptr<uiElement> p, int x, int y, char_id id) : uiHLink(p, x, y, global::w_character_name(char_id_t(id)), global::empty, global::standard_text, global::whandle, [ id](uiHLink* h) {SetupChPane(char_id_t(id)); }) {
}
/*
uiButton::uiButton(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(std::string) btext, IN(paint_region) paint, IN(text_format) format, IN(std::function<void(uiButton*)>) iclickaction, sf::Keyboard::Key defkey) :
	uiRect(p, ix, iy, iwidth, iheight, paint), clickaction(iclickaction), bndkey(defkey), text(btext, *format.font, format.csize) {
	text.setColor(format.color);
}

bool uiButton::keypress(sf::Keyboard::Key k) {
	 if (visible && k == bndkey) {
		 clickaction(this);
		 return true;
	 } else {
		 return uiRect::keypress(k);
	 }
}

void uiButton::draw(OGLLock& target, int x, int y) {
	if (visible) {
		uiRect::draw(target, x, y);
		sf::FloatRect bnd = text.textBounds();
		if (leftalign)
			text.setPosition(static_cast<int> (x - bnd.left) + 10, static_cast<int> (y - bnd.top + (pos.height - bnd.height) / 2));
		else if (!txtmode)
			text.setPosition(static_cast<int> (x - bnd.left + (pos.width - (bnd.width)) / 2), static_cast<int> (y - bnd.top + (pos.height - bnd.height) / 2));
		else
			text.setPosition(static_cast<int> (x - bnd.left), static_cast<int> (y - bnd.top));

		target->draw(text);
	}
}

bool uiButton::click(int x, int y) {
	if (visible) {
		uiElement::click(x, y);
		clickaction(this);
		return true;
	}
	else {
		return false;
	}
}/**/

/*bool ui_button_disable::keypress(sf::Keyboard::Key k) {
	 if (visible && k == bndkey) {
		 if (enabled)
			 clickaction(this);
		 return true;
	 } else {
		 return uiSimpleTooltip::keypress(k);
	 }
}

void ui_button_disable::draw(OGLLock& target, int x, int y) {
	 static const sf::Color gry(128, 128, 128);
	 if (visible) {
		 if (enabled) {
			 fill.draw(target, x, y, pos.width, pos.height);
			 text.setColor(tcolor);
		 } else {
			 dfill.draw(target, x, y, pos.width, pos.height);
			 text.setColor(gry);
		 }

		 sf::FloatRect bnd = text.textBounds();
		 if (leftalign)
			 text.setPosition(static_cast<int> (x - bnd.left) + 10, static_cast<int> (y - bnd.top + (pos.height - bnd.height) / 2));
		 else if (!txtmode)
			 text.setPosition(static_cast<int> (x - bnd.left + (pos.width - (bnd.width)) / 2), static_cast<int> (y - bnd.top + (pos.height - bnd.height) / 2));
		 else
			 text.setPosition(static_cast<int> (x - bnd.left), static_cast<int> (y - bnd.top));

		 target->draw(text);
	 }
}

bool ui_button_disable::click(int x, int y) {
	 if (visible) {
		 uiElement::click(x, y);
		 if(enabled)
			 clickaction(this);
	
		 return true;
	 } else {
		 return false;
	 }
}

void ui_button_disable::enable() {
	 enabled = true;
	 update_tooltip("");
}

void ui_button_disable::disable(const std::string & reason) {
	 enabled = false;
	 if (tt_text.length() == 0) {
		 update_tooltip(reason);
	 } else {
		 tt_text += "\n";
		 tt_text += reason;
	 }
}/**/

void uiDropDown::OpenSelect() {
	int absx = 0;
	int absy = 0;
	posabs(absx, absy);
	obox->pos.left = absx;
	obox->pos.top = absy + pos.height;

	 if (auto p = parent.lock()) {
		 const auto top = p->top();
		 global::uiqueue.push([top = p->top(), obox = this->obox]{
			 obox->open(top);
		 });
	 }
}

void uiDropDown::updatepos() {
	int absx = 0;
	int absy = 0;
	posabs(absx, absy);
	obox->pos.left = absx;
	obox->pos.top = absy + pos.height;
	uiButton::updatepos();
}

uiDropDown::uiDropDown(std::weak_ptr<uiElement> par, int x, int y, int w, int h, IN(paint_region) paint, IN(text_format) format, IN(std::vector<std::wstring>) options,  IN(std::function<void(uiDropDown*, int)>) ichooseaction) : chooseaction(ichooseaction),
	 uiButton(par, x, y, w, h, options[0], paint, format, [this](uiButton* b) {this->OpenSelect(); }) {
	 if (auto p = par.lock()) {
		 obox = std::make_shared<uiMenuRect>(p->top(), x, y + h, w, static_cast<int>(h*options.size()), paint);
		 leftalign = true;

		 for (unsigned int i = 0; i < options.size(); i++) {
			 const auto b = obox->add_element<uiButton>(0, h*i, w, h, options[i], paint, format, [this, i](uiButton *b) {
				 this->text.setString(b->text.getString());
				 this->chosen = i; this->chooseaction(this, i); this->obox->close(); this->redraw(); });
			 b->leftalign = true;
		 }
	 }

	 num_options = static_cast<int>(options.size());
	 obox->pos.height = pos.height * num_options;
}

uiDropDown::uiDropDown(std::weak_ptr<uiElement> par, int x, int y, int w, int h, IN(paint_region) paint, IN(text_format) format,  IN(std::function<void(uiDropDown*, int)>) ichooseaction) : chooseaction(ichooseaction),
uiButton(par, x, y, w, h, TEXT(""), paint, format, [this](uiButton* b) {this->OpenSelect(); }) {
	if (auto p = par.lock()) {
		obox = std::make_shared<uiMenuRect>(p->top(), x, y + h, w, 0, paint);
		leftalign = true;
	}

	num_options = 0;
}

uiDropDown::uiDropDown(std::weak_ptr<uiElement> par, int x, int y, int w, int h, IN(paint_region) paint, IN(text_format) format) : chooseaction([](uiDropDown*, int){}), uiButton(par, x, y, w, h, TEXT(""), paint, format, [this](uiButton* b) {this->OpenSelect(); }) {
	if (auto p = par.lock()) {
		obox = std::make_shared<uiMenuRect>(p->top(), x, y + h, w, 0, paint);
		leftalign = true;
	}

	num_options = 0;
}

void uiDropDown::reset_options() {
	num_options = 0;
	obox->pos.height = pos.height * num_options;
	//text.setString("");
	obox->subelements.clear();
}

void uiDropDown::add_option(IN(std::wstring) str, int value) {
	const auto b = obox->add_element<uiButton>(0, pos.height*num_options, pos.width, pos.height, str, *paint.fill, text_format(text.getCharacterSize(), const_cast<sf::Font*>(text.getFont()), stdcolor), [this, value](uiButton *b) {
		this->text.setString(b->text.getString());
		this->chosen = value; this->chooseaction(this, value); this->obox->close(); this->redraw(); });
	b->leftalign = true;
	if (num_options == 0) {
		//chosen = value;
		//text.setString(str);
	}
	++num_options;
	obox->pos.height = pos.height * num_options;
}

void uiDropDown::add_option(IN(std::wstring) str, IN(std::function<void()>) f) {
	const auto b = obox->add_element<uiButton>(0, pos.height*num_options, pos.width, pos.height, str, *paint.fill, text_format(text.getCharacterSize(), const_cast<sf::Font*>(text.getFont()), stdcolor), [this, f](uiButton *b) {
		this->text.setString(b->text.getString());
		f();
		this->obox->close();
	});
	b->leftalign = true;
	if (num_options == 0) {
		//chosen = 0;
		//text.setString(str);
	}
	++num_options;
	obox->pos.height = pos.height * num_options;
}

void uiPanes::init(unsigned int n) {
	for (unsigned int i = 0; i != n; ++i) {
		std::shared_ptr<uiScrollView> pane = std::make_shared<uiScrollView>(0, 0, pos.width, pos.height, shared_from_this());
		if (i > 0) {
			pane->setVisible(false);
		}
		panes.push_back(pane);
		subelements.push_back(pane);
	}
}

void uiPanes::update_size() {
	for (IN(auto) p : panes) {
		p->pos.width = pos.width;
		p->pos.height = pos.height;
	}
}

void uiPanes::activate_pane(unsigned int n) {
	for (IN(auto) p : panes) {
		p->setVisible(false);
	}
	panes[n]->setVisible(true);
}

void uiPanes::activate_pane(IN(std::shared_ptr<uiScrollView>) p) {
	for (IN(auto) q : panes) {
		q->setVisible(false);
	}
	p->setVisible(true);
}

void uiPanes::deactivate_panes() {
	for (IN(auto) q : panes) {
		q->setVisible(false);
	}
}

void uiTabs::init(IN(text_format) format, IN(paint_region) paint, IN(std::vector<std::wstring>) btext) {
	 sf::FloatRect bnd = format.font->getTextBounds(btext[0], format.csize);
	 bheight = static_cast<int>(bnd.height) + 5;
	 int bwidth = static_cast<int>(pos.width / btext.size());
	 for (unsigned int i = 0; i < btext.size(); i++) {
		 std::shared_ptr<uiScrollView> pane = std::make_shared<uiScrollView>(0, bheight, pos.width, pos.height - bheight, shared_from_this());
		 if (i > 0) {
			 pane->setVisible(false);
		 }
		 const std::shared_ptr<uiButton> bttn = std::make_shared<uiButton>(shared_from_this(), bwidth * i, 0, bwidth, bheight, btext[i], paint, format, [tptr = this, wpane = std::weak_ptr<uiVRect>(pane)](uiButton *b) {
			 for (auto &i : tptr->panes) {
				 i->setVisible(false);
			 }
			 if(auto pane = wpane.lock())
				pane->setVisible(true); tptr->redraw();
		 });
		 panes.push_back(pane);
		 subelements.push_back(pane);
		 subelements.push_back(bttn);
	 }

}

void uiTabs::update_tab_sizes() {
	for (IN(auto) tb : panes) {
		tb->pos.width = pos.width;
		tb->pos.height = pos.height - bheight;
		tb->calcTotalHeight();
	}
}

uiTabs::uiTabs(int ix, int iy, int iwidth, int iheight, std::weak_ptr<uiElement> p) : uiVRect(ix, iy, iwidth, iheight, p) {
}

uiPanes::uiPanes(int ix, int iy, int iwidth, int iheight, std::weak_ptr<uiElement> p) : uiVRect(ix, iy, iwidth, iheight, p) {
}

bool uiHLink::move(int x, int y) {
	uiElement::move(x, y);
	return true;
}

void uiHLink::updateAction(const std::function<void(uiHLink*)>& act) {
	 clickaction = act;
}

sf::FloatRect sf::iText::textBounds() {
	 return const_cast<sf::Font*>(this->getFont())->getTextBounds(this->getString(), this->getCharacterSize(), (this->getStyle() & sf::Text::Bold) != 0);
}


void uiMenuRect::close() {
	 global::uiqueue.push([par = parent, th = shared_from_this()] {
		 if (auto p = par.lock()) {
			 auto it = std::find(p->subelements.cbegin(), p->subelements.cend(), th);
			 if (it != p->subelements.cend()) {
				 p->subelements.erase(it);
			 }
		 }
		 remove_from_menus(std::static_pointer_cast<uiMenuRect>(th));
	 });
}

void uiMenuRect::open(IN(std::shared_ptr<uiElement>) disp_parent) {
	if (std::find(disp_parent->subelements.cbegin(), disp_parent->subelements.cend(), shared_from_this()) == disp_parent->subelements.cend())
		disp_parent->subelements.insert(disp_parent->subelements.begin(), shared_from_this());
	add_to_menus(std::static_pointer_cast<uiMenuRect>(shared_from_this()));
}

void uiMenuRect::move_to(int x, int y) {
	pos.left = std::max(0,x);
	pos.top = std::max(0, y);
	if (auto p = parent.lock()) {
		if (pos.top + pos.height > p->pos.height)
			pos.top = p->pos.height - pos.height;
		if (pos.left + pos.width > p->pos.width)
			pos.left = p->pos.width - pos.width;
	}
	for (IN(auto) i : subelements) {
		i->updatepos();
	}
}

bool uiMenuRect::click(int x, int y) {
	menu_clicked = true;
	return uiRect::click(x, y);
}

bool uiMenuRect::rclick(int x, int y) {
	menu_clicked = true;
	return uiRect::rclick(x, y);
}

void uiMenuRect::clickoff() {
	 close();
}

uiCheckBox::uiCheckBox(std::weak_ptr<uiElement> p, int ix, int iy, IN(std::wstring) caption, IN(text_format) caption_format, IN(std::wstring) str, IN(text_format) tt_format, bool t, IN(std::function<void(uiCheckBox*)>) iclickaction) :
	uiSimpleTooltip(p, ix, iy, 1, 1, str, tt_format), def_tt_text(str), clickaction(iclickaction), text(caption, *caption_format.font, caption_format.csize), disabledcolor(caption_format.disabled_color), stdcolor(caption_format.color), toggled(t), enabled(true) {

	text.setColor(caption_format.color);
	auto r = caption_format.font->getTextBounds(caption, caption_format.csize);
	const static int margin = 2;

	pos.left = ix;
	pos.top = iy;
	pos.width = 24 + 5 + static_cast<int>(r.width) + margin * 2;
	pos.height = std::max(24, static_cast<int>(r.height) + margin * 2);
	voff = (24 - (static_cast<int>(r.height) + margin * 2)) / 2;
}

void uiCheckBox::draw(OGLLock & target, int x, int y) {
	 if (gVisible()) {
		 int state = (toggled ? 0 : 1) + (enabled ? 0 : 2);

		 sf::Sprite image(*global::checkbox.tex, sf::IntRect(24 * state,0,24,24));
		 image.setPosition(static_cast<float>(x), static_cast<float>(y));
		 target->draw(image);

		 text.setPosition(static_cast<int> (x + 24 + 5), static_cast<int> (y + (voff > 0 ? voff : 0)));
		 target->draw(text);

		 uiSimpleTooltip::draw(target, x, y);
	 }
}

bool uiCheckBox::click(int x, int y) {
	 if (gVisible()) {
		 toggled = !toggled;
		 uiElement::click(x, y);
		 clickaction(this);
		 return true;
	 } else {
		 return false;
	 }
}

uiTextBox::uiTextBox(std::weak_ptr<uiElement> p, int x, int y, int width, IN(std::string) str, IN(paint_region) paint, IN(text_format) format,
					 IN(std::function<void(uiTextBox*, const sf::String&)>) t, int m) : fill(paint), text(str, *format.font, format.csize), uiElement(x, y, width, 1, p), margin(m), textupdate(t) {
	text.setColor(format.color);
	pos.height = static_cast<int>(format.font->getLineSpacing(format.csize) + margin * 2);
}

bool uiTextBox::keypress(sf::Keyboard::Key k) {
	if (!hasfocus)
		return false;

	 if (k == sf::Keyboard::Left) {
		 if (cursorposition > 0)
			 --cursorposition;
	 } else if (k == sf::Keyboard::Right) {
		 cursorposition = std::min(cursorposition + 1, static_cast<unsigned int>(text.getString().getSize()));
	 }
	 return true;
}

bool uiTextBox::textinput(unsigned int cvalue) {
	if (!hasfocus)
		return false;

	 TCHAR ch = TCHAR(cvalue);
	 if (ch == TEXT('\b')) {
		 if (cursorposition > 0) {
			 text.setString(text.getString().substring(0, cursorposition - 1) + text.getString().substring(cursorposition));
			 --cursorposition;
		 }
	 } else if (ch == 127) {
		text.setString(text.getString().substring(0, cursorposition) + text.getString().substring(cursorposition + 1));
	} else {
		 if (cursorposition >= text.getString().getSize()) {
			 text.setString(text.getString() + ch);
		 } else {
			 text.setString(text.getString().substring(0, cursorposition) + ch + text.getString().substring(cursorposition));
		 }
		 ++cursorposition;
	 }
	 textupdate(this, text.getString());
	 return true;
}

sf::Clock uiTextBox::blinktimer;

void uiTextBox::draw(OGLLock & target, int x, int y) {
	fill.draw(target, x, y, pos.width, pos.height);

	 if (hasfocus) {
		 sf::RectangleShape cur(sf::Vector2f(1.0f, static_cast<float>(pos.height - margin*2)));
		 sf::Color ccolor = text.getColor();
		 float elapsed = static_cast<float>(std::abs((blinktimer.getElapsedTime().asMilliseconds() % 800) - 400)) / 800.0f;
		 ccolor.a = static_cast<sf::Uint8>(255.0f * (elapsed + 0.5f));
		 cur.setFillColor(ccolor);
		 cur.setPosition(static_cast<float>(x + const_cast<sf::Font*>(text.getFont())->getTextBounds(text.getString().substring(0, cursorposition), text.getCharacterSize()).width + margin), static_cast<float>(y+margin));
		 target->draw(cur);
	 }

	 text.setPosition(static_cast<int> (x  + margin), static_cast<int> (y + margin));
	 target->draw(text);

	 uiElement::draw(target, x, y);
}

bool uiTextBox::click(int x, int y) {
	 const sf::String& boxtext = text.getString();
	 const auto font = text.getFont();

	 hasfocus = true;
	 int xoff = margin;
	 sf::String temptext;

	 for (size_t indx = 0; indx < boxtext.getSize(); ++indx) {
		 temptext += boxtext[indx];
		 const auto bounds = const_cast<sf::Font*>(font)->getTextBounds(temptext, text.getCharacterSize());
		 if (x < (static_cast<int>(bounds.width) + margin - xoff) / 2 + xoff) {
			 cursorposition = static_cast<unsigned int>(indx);
			 return true;
		 }
		 xoff = static_cast<int>(bounds.width) + margin;
	 }

	 cursorposition = static_cast<unsigned int>(boxtext.getSize());
	 return true;
}

void uiTextBox::clickoff() {
	hasfocus = false;
}

uiCountDown::uiCountDown(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, unsigned int start, unsigned int end, IN(paint_region) paint, const std::wstring & l, IN(text_format) format, const  std::function<void(uiCountDown*)>& func) :  uiElement(ix, iy, iwidth, iheight, p), start_date(start), fill(paint), end_date(end), finish(func), text(l, *format.font, format.csize) {
	text.setColor(format.color);
}

void uiCountDown::draw(OGLLock & target, int x, int y) {
	if (start_date < end_date) {
		const float barpos = (global::currentday >= start_date ? static_cast<float>(pos.width) : 0.0f) *
			(end_date >= global::currentday ? (1.0f - static_cast<float>(end_date - global::currentday) / static_cast<float>(end_date - start_date)) : 1.0f);
		global::solid_border.draw(target, x, y, pos.width, pos.height);
		fill.draw(target, x, y, static_cast<int>(barpos), pos.height);
		//sf::RectangleShape bar(sf::Vector2f(barpos, static_cast<float>(pos.height)));
		//bar.setFillColor(bar_color);
		//bar.setPosition(static_cast<float>(x), static_cast<float>(y));
		//target->draw(bar);

		sf::RectangleShape rs(sf::Vector2f(static_cast<float>(pos.width), static_cast<float>(pos.height)));
		rs.setOutlineColor(sf::Color::Black);
		rs.setOutlineThickness(1);
		rs.setFillColor(sf::Color::Transparent);
		rs.setPosition(static_cast<float>(x), static_cast<float>(y));
		target->draw(rs);

		if (global::currentday >= end_date) {
			finish(this);
			start_date = end_date;
		}

	} else {
		//sf::RectangleShape rs(sf::Vector2f(static_cast<float>(pos.width), static_cast<float>(pos.height)));
		//rs.setOutlineColor(sf::Color::Black);
		//rs.setOutlineThickness(1);
		//rs.setFillColor(bar_color);
		//rs.setPosition(static_cast<float>(x), static_cast<float>(y));
		//target->draw(rs);
		fill.draw(target, x, y, pos.width, pos.height);
	}

	text.setPosition(x + 3, y + 3);
	target->draw(text);

	sf::iText date_text(w_day_to_string(end_date), *text.getFont(), text.getCharacterSize());
	date_text.setColor(text.getColor());
	date_text.setPosition(static_cast<int>(text.getLocalBounds().width) + x + 8 + 10, y + 3);
	target->draw(date_text);

	uiElement::draw(target, x, y);
}

void ui_tt_icon::draw(OGLLock & target, int x, int y) {
	if (visible) {
		fill.draw(target, x, y, pos.width, pos.height);
		uiSimpleTooltip::draw(target, x, y);
	}
}

ui_stress_button::ui_stress_button(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(std::wstring) btext, IN(paint_states<total_states>) pn, IN(text_format) format, size_t stress, IN(text_format) ttformat, IN(std::function<void(this_type*)>) iclickaction, sf::Keyboard::Key defkey) :
	button_type(p, x, y, w, h, btext, pn, format, 
		stress != 0 ? get_p_string(TX_STRESS_TT, &stress, 1) : TEXT(""),
		ttformat, [iclickaction, stress](IN_P(button_type) b) { global::playerstress += static_cast<short>(stress); iclickaction(b); }, defkey) {
	if (global::playerstress + static_cast<short>(stress) > MAX_PLAYER_STRESS && stress != 0) {
		disable(get_simple_string(TX_INSUFFICIENT_STR));
	}
}
