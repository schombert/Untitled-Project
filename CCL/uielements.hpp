#pragma once
#include "globalhelpers.h"

struct layoutelement {
	unsigned int stringindx;
	size_t firstchar;
	size_t endchar;
	sf::Vector2f position;
};

struct text_format {
	int csize;
	sf::Font* __restrict font;
	sf::Color color;
	sf::Color disabled_color = sf::Color(128,128,128);
	
	text_format() {};
	text_format(int cz, sf::Font* f, IN(sf::Color) c) : csize(cz), font(f), color(c) {}

	void init(IN_P(sf::Font) f, IN(sf::Color) c, int sz) {
		csize = sz;
		font = f;
		color = c;
	}
};

class paint_region {
public:
	virtual void draw(IN(OGLLock) target, int x, int y, int width, int height) const {}
};

using no_fill = paint_region;

class paint_rect : public paint_region {
public:
	sf::Color fill;
	sf::Color border;
	int bordersize;

	void init(IN(sf::Color) f) {
		fill = f;
		bordersize = 0;
	}

	void init(IN(sf::Color) f, IN(sf::Color) b, int bsize) {
		fill = f;
		border = b;
		bordersize = bsize;
	}

	paint_rect() {}
	paint_rect(IN(sf::Color) f) : fill(f), bordersize(0) {}
	paint_rect(IN(sf::Color) f, IN(sf::Color) b, int bsize) : fill(f), border(b), bordersize(bsize) {}

	virtual void draw(IN(OGLLock) target, int x, int y, int width, int height) const  {
		sf::RectangleShape rs(sf::Vector2f(static_cast<float>(width), static_cast<float>(height)));
		if (bordersize > 0) {
			rs.setOutlineColor(sf::Color::Black);
			rs.setOutlineThickness(static_cast<float>(bordersize));
		}
		rs.setFillColor(fill);
		rs.setPosition(static_cast<float>(x), static_cast<float>(y));
		target->draw(rs);
	}
};


class texture_rect : public paint_region {
public:
	const sf::Texture* __restrict tex;

	void init(IN_P(sf::Texture) t) {
		tex = t;
	}

	virtual void draw(IN(OGLLock) target, int x, int y, int width, int height) const {
		sf::Sprite image(*tex);
		image.setPosition(static_cast<float>(x), static_cast<float>(y));
		const auto bnd = image.getLocalBounds();
		image.setScale(static_cast<float>(width) / bnd.width, static_cast<float>(height) / bnd.height);
		target->draw(image);
	}
};

template<int x, int y, int w, int h>
class texture_fragment : public texture_rect {
public:
	const static sf::IntRect view;

	virtual void draw(IN(OGLLock) target, int x, int y, int width, int height) const {
		sf::Sprite image(*tex, view);
		image.setPosition(static_cast<float>(x), static_cast<float>(y));
		const auto bnd = image.getLocalBounds();
		image.setScale(static_cast<float>(width) / bnd.width, static_cast<float>(height) / bnd.height);
		target->draw(image);
	}
};

template<int x, int y, int w, int h>
const sf::IntRect texture_fragment<x,y,w,h>::view(x,y,w,h);

template<int w, int h, int xs, int ys>
class texture_array {
protected:
	class inner_texture_rect : public paint_region {
	public:
		const texture_array<w,h,xs,ys>* __restrict container;
		const sf::IntRect*__restrict view;

		void init(IN(texture_array<w,h,xs,ys>) c, IN(sf::IntRect) v) {
			container = &c;
			view = &v;
		}

		virtual void draw(IN(OGLLock) target, int x, int y, int width, int height) const {
			sf::Sprite image(*(container->tex), *view);
			image.setPosition(static_cast<float>(x), static_cast<float>(y));
			const auto bnd = image.getLocalBounds();
			image.setScale(static_cast<float>(width) / bnd.width, static_cast<float>(height) / bnd.height);
			target->draw(image);
		}
	};

	template <int n>
	void texture_array_cn() {
		rects[n] = sf::IntRect((n % xs) * w, static_cast<int>(n / xs) * h, w, h);
		textures[n].init(*this, rects[n]);
		texture_array_cn<n - 1>();
	}

	template<>
	void texture_array_cn<0>() {
		rects[0] = sf::IntRect(0, 0, w, h);
		textures[0].init(*this, rects[0]);
	}

public:
	const sf::Texture* __restrict tex;
	inner_texture_rect textures[xs*ys];
	sf::IntRect rects[xs*ys];

	void init(IN_P(sf::Texture) t) {
		tex = t;
	}

	texture_array() {
		texture_array_cn<xs*ys - 1>();
	}

	const paint_region& get(int x, int y) {
		return textures[x + y*xs];
	}
};

template<unsigned int n>
class paint_states : public paint_region {
public:
	unsigned int state = 0;
	const paint_region* states[n];

	template<typename ... PARAM>
	explicit paint_states(const PARAM & ... params) : states{&params ...} {
	}

	paint_states(const paint_states<n>& source) = default;

	virtual void draw(IN(OGLLock) target, int x, int y, int width, int height) const {
		if (state < n) {
			states[state]->draw(target, x, y, width, height);
		} else {
			states[0]->draw(target, x, y, width, height);
		}
	}

	void set_state(unsigned int s) {
		state = s;
	}
	void set_ptr(unsigned int s, IN(paint_region) p) {
		if (s < n) {
			states[s] = &p;
		} else {
			states[0] = &p;
		}
	}
	void set_ptr(unsigned int s, IN_P(paint_region) p) {
		set_ptr(s, *p);
	}
};

template<>
class paint_states<1> : public paint_region {
public:
	const paint_region* __restrict fill;

	paint_states(IN(paint_region) f) : fill(&f) {}
	paint_states(IN_P(paint_region) f) : fill(f) {
		if (!f)
			abort();
	}

	virtual void draw(IN(OGLLock) target, int x, int y, int width, int height) const {
		fill->draw(target, x, y, width, height);
	}

	void set_state(unsigned int s) {
	}
	void set_ptr(unsigned int s, IN(paint_region) p) {
		fill = &p;
	}
	void set_ptr(unsigned int s, IN_P(paint_region) p) {
		set_ptr(s, *p);
	}
};

template<>
class paint_states<0> {
};

inline bool rectcontains(const sf::IntRect &ir, const int x, const int y);

namespace sf {
	class iText : public Text {
	using Text::Text;
	public:
		inline void setPosition(int x, int y) {
			Text::setPosition(static_cast<float>(x), static_cast<float>(y));
		};
		sf::FloatRect textBounds();
	};
}

class uiElement : public std::enable_shared_from_this<uiElement> {
	public:
		std::vector<std::shared_ptr<uiElement>> subelements;
		sf::IntRect pos;
		std::weak_ptr<uiElement> parent;

		uiElement(int ix, int iy, int iwidth, int iheight, std::weak_ptr<uiElement> p) : parent(p), pos(ix,iy,iwidth,iheight), selected(false) {
		};

		std::shared_ptr<uiElement> top();
		virtual void posabs(int &x, int &y);
		void poslocal(int &x, int &y, IN(std::shared_ptr<uiElement>) element);

		virtual void draw(OGLLock &target, int x = 0, int y = 0);
		virtual bool click(int x, int y);
		virtual bool release(int x, int y);
		virtual bool rclick(int x, int y);
		virtual bool rrelease(int x, int y);
		virtual bool move(int x, int y);
		virtual bool keypress(sf::Keyboard::Key k);
		virtual bool textinput(unsigned int cvalue);
		virtual void enter();
		virtual void leave();
		virtual void clickoff();
		virtual bool scroll(int x, int y, float amount);
		virtual void updatepos() { for (auto &i : subelements) i->updatepos(); };
		virtual void redraw() { if (auto p = parent.lock()) p->redraw(); };
		void registerCapture(IN(std::weak_ptr<uiElement>) element);
		void releaseCapture(IN(std::weak_ptr<uiElement>) element);

		std::vector<std::weak_ptr<uiElement>> capture;
		bool selected;
		bool visible = true;

		void setVisible(bool v);
		bool gVisible();

		sf::IntRect largest_free_rect(int maxx = 1, int maxy = 1);

		template <typename KIND, typename ... ARGS>
		std::shared_ptr<KIND> add_element(ARGS&& ... args) {
			const auto elem = std::make_shared<KIND>(shared_from_this(), std::forward<ARGS>(args) ...);
			subelements.push_back(elem);
			return elem;
		}
};

template <typename T>
T rect_area(IN(sf::Rect<T>) source) {
	return source.width * source.height;
}

sf::IntRect largest_rect(decltype(std::declval<uiElement>().subelements.begin()) begin, decltype(std::declval<uiElement>().subelements.end()) end, IN(sf::IntRect) base, int minx, int miny);
void make_point_visible(glm::dvec2 texture_point, IN(OGLLock) mainwin);
void make_vec_visible(glm::dvec3 source, IN(OGLLock) mainwin);

using uiVRect = uiElement;

class uiRect : public uiVRect {
	public:
	const paint_region& __restrict fill;

	uiRect(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(paint_region) paint) : uiVRect(ix, iy, iwidth, iheight, p), fill(paint) {};
	virtual void draw(OGLLock &target, int x, int y);;
	virtual bool click(int x, int y);
	virtual bool release(int x, int y);
	virtual bool rclick(int x, int y);
	virtual bool rrelease(int x, int y);
	virtual bool move(int x, int y);
};

class uiInvisibleButton : public uiElement {
public:
	std::function<void(uiInvisibleButton*)> clickaction;
	HWND wnd;

	uiInvisibleButton(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, HWND w, const std::function<void(uiInvisibleButton*)>& iclickaction) : uiElement(ix, iy, iwidth, iheight, p), wnd(w), clickaction(iclickaction) {};
	virtual bool move(int x, int y);
	virtual void enter();
	virtual void leave();
	virtual bool click(int x, int y);
		
};

class uiFloatRect : public uiVRect {
public:
	int mxoff, myoff;
	double mwpercent, mhpercent;
	uiFloatRect(int xoff, int yoff, double wpercent, double hpercent, int width, int height, std::weak_ptr<uiElement> p) : uiVRect(0, 0, width, height, p),
		mxoff(xoff), myoff(yoff), mwpercent(wpercent), mhpercent(hpercent) {
		innerPosAdjustment();
	};
	void innerPosAdjustment();
	virtual void updatepos();
};

class uiHozContainer : public uiVRect {
public:
	int hmargin;
	sf::iText text;
	const paint_region& __restrict fill;

	uiHozContainer(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, int ihmargin, IN(std::wstring) str, IN(paint_region) paint, IN(text_format) format);
	void RecalcPos();
	virtual void draw(OGLLock &target, int x, int y);
};

class uiSimpleText : public uiVRect {
protected:
	sf::iText text;
public:
	const paint_region& __restrict fill;
	int margin = 0;

	uiSimpleText(std::weak_ptr<uiElement> p, int x, int y, IN(std::wstring) str, IN(paint_region) paint, IN(text_format) format);

	virtual void draw(OGLLock &target, int x, int y);
	virtual bool move(int x, int y);
	sf::FloatRect textBounds(const sf::Font* const f);
	void updateText(IN(std::wstring) txt);
	void updatebounds();
};

class uiTextBox : public uiElement {
private:
	static sf::Clock blinktimer;
public:
	sf::iText text;
	const paint_region& __restrict fill;
	int margin;
	unsigned int cursorposition;
	bool hasfocus = true;

	std::function<void(uiTextBox*, const sf::String&)> textupdate;

	uiTextBox(std::weak_ptr<uiElement> p, int x, int y, int width, IN(std::string) str, IN(paint_region) paint, IN(text_format) format, IN(std::function<void(uiTextBox*, const sf::String&)>) t, int m = 2);;
	virtual bool keypress(sf::Keyboard::Key k);
	virtual bool textinput(unsigned int cvalue);
	virtual void draw(OGLLock &target, int x, int y);
	virtual bool click(int x, int y);
	virtual void clickoff();
};


class uiHLink : public uiSimpleText {
public:
	std::function<void(uiHLink*)> clickaction;
	HWND wnd;

	uiHLink(std::weak_ptr<uiElement> p, int x, int y, IN(std::wstring) str, IN(paint_region) paint, IN(text_format) format, HWND w, IN(std::function<void(uiHLink*)>) act = [](uiHLink*){}) : uiSimpleText(p, x, y, str, paint, format), wnd(w), clickaction(act) {
		text.setColor(sf::Color::Blue);
	};
	virtual bool move(int x, int y);
	void updateAction(const std::function<void(uiHLink*)> &act);
	virtual void enter();
	virtual void leave();
	virtual bool click(int x, int y);
};

class ui_ch_hlink : public uiHLink {
public:
	ui_ch_hlink(std::weak_ptr<uiElement> p, int x, int y, char_id id);
};

class uiCenterdText : public uiElement{
public:
	sf::iText text;
	const paint_region& __restrict fill;

	uiCenterdText(std::weak_ptr<uiElement> p, int x, int y, int w, IN(std::wstring) str, IN(paint_region) paint, IN(text_format) format);
	sf::FloatRect textBounds(sf::Font* const f);
	virtual void draw(OGLLock &target, int x, int y);;
};

class uiTextBlock : public uiVRect {
public:
	cvector<sf::Text> textelements;
	const paint_region& __restrict fill;

	uiTextBlock(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, INOUT(std::vector<layoutelement>) layout, IN(paint_region) paint, IN(text_format) format, IN(std::vector<std::wstring>) sourcetext);
	uiTextBlock(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, INOUT(cvector<layoutelement>) layout, IN(paint_region) paint, IN(text_format) format, IN(cvector<std::wstring>) sourcetext);
	void addLink(const size_t index, IN(std::vector<layoutelement>) layout,  IN(std::function<void(uiInvisibleButton*)>) action);
	void addLink(const size_t index, IN(cvector<layoutelement>) layout, IN(std::function<void(uiInvisibleButton*)>) action);
	virtual void draw(OGLLock &target, int x, int y);;
};

class uiScrollView : public uiVRect {
public:
	//sf::RenderTexture tex;
	int scrolloff = 0;
	int totalheight = 0;
	int margin = 0;
	bool ready = false;

	uiScrollView(int ix, int iy, int iwidth, int iheight, std::weak_ptr<uiElement> p) : uiVRect(ix, iy, iwidth, iheight, p) {
		//if (!tex.create(iwidth, iheight))
		//	OutputDebugStringA("T-CREATE FAILED SV");
	};
	uiScrollView(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight) : uiVRect(ix, iy, iwidth, iheight, p) {
	};


	void calcTotalHeight();
	virtual void draw(OGLLock &target, int x, int y);;

	int lasty; bool dragging = false;
	virtual bool click(int x, int y);
	virtual bool release(int x, int y);
	virtual bool move(int x, int y);
	virtual bool scroll(int x, int y, float amount);

	virtual void posabs(int &x, int &y);
	virtual bool rclick(int x, int y);
	virtual bool rrelease(int x, int y);

	void reset();
};

class uiSlider : public uiElement {
public:
	int cpos;
	int smax;
	int icpos;
	bool dragging;
	std::function<void(uiSlider*, int)> slideaction;;

	uiSlider(int x, int y, int w, int h, std::weak_ptr<uiElement> p, int maxscroll, const std::function<void(uiSlider*, int)> &sa) : uiElement(x, y, w, h, p), slideaction(sa), smax(maxscroll), cpos(0), dragging(false) {}

	virtual bool click(int x, int y);
	virtual bool release(int x, int y);
	virtual bool move(int x, int y);
	virtual void draw(OGLLock &target, int x, int y);
};

class uiSimpleTooltip : public uiVRect{
public:
	static std::shared_ptr<uiSimpleText> shared_tooltip;
	static std::weak_ptr<uiSimpleTooltip> active;
	static void init_shared_tooltip(std::weak_ptr<uiElement> p, IN(text_format) format);

	bool below;
	std::wstring tt_text;

	uiSimpleTooltip(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(std::wstring) str, IN(text_format) format, bool ibelow = true);
	uiSimpleTooltip(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(text_format) format, bool ibelow = true) : uiSimpleTooltip(p, ix, iy, iwidth, iheight, L"", format, ibelow) {}

	void update_tooltip(IN(std::wstring) text);
	virtual void enter();
	virtual void leave();
	virtual void updatepos();
	virtual bool move(int x, int y);
};


class uiDragRect : public uiRect {
public:
	bool dragging = false;
	int basex = 0;
	int basey = 0;

	uiDragRect(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(paint_region) paint) :  uiRect(p, x, y, w, h, paint) {};

	void toFront(IN(std::shared_ptr<uiElement>) par);
	virtual bool click(int x, int y);
	virtual bool release(int x, int y);
	virtual bool move(int x, int y);
};

class uiMenuRect : public uiRect {
public:
	uiMenuRect(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(paint_region) paint) :  uiRect(p, x, y, w, h, paint) {};
	void close();
	void open(IN(std::shared_ptr<uiElement>) parent);
	void move_to(int x, int y);
	virtual bool click(int x, int y);
	virtual bool rclick(int x, int y);
	virtual void clickoff();
};

void createChActionMenu(std::shared_ptr<uiElement> &p, int x, int y, char_id id);

class uiCheckBox : public uiSimpleTooltip {
public:
	sf::iText text;
	std::wstring def_tt_text;
	int voff;
	bool toggled;
	bool enabled;
	sf::Color stdcolor;
	sf::Color disabledcolor;

	std::function<void(uiCheckBox*)> clickaction;

	uiCheckBox(std::weak_ptr<uiElement> p, int ix, int iy, IN(std::wstring) caption, IN(text_format) caption_format, IN(std::wstring) str, IN(text_format) tt_format, bool toggled, IN(std::function<void(uiCheckBox*)>) iclickaction);
	uiCheckBox(std::weak_ptr<uiElement> p, int ix, int iy, IN(std::wstring) caption, IN(text_format) caption_format, IN(text_format) tt_format, bool toggled, IN(std::function<void(uiCheckBox*)>) iclickaction) : uiCheckBox(p, ix, iy, caption, caption_format, L"", tt_format, toggled, iclickaction) {};
	virtual void draw(OGLLock &target, int x, int y);
	virtual bool click(int x, int y);

	void enable() {
		enabled = true;
		update_tooltip(def_tt_text);
		text.setColor(stdcolor);
	};

	void disable(IN(std::wstring) reason) {
		if (enabled) {
			enabled = false;
			text.setColor(disabledcolor);
			update_tooltip(reason);
		} else {
			tt_text += TEXT("\n");
			tt_text += reason;
		}
	};
};

class ui_tt_icon : public uiSimpleTooltip {
public:
	IN(paint_region) fill;

	std::function<void(uiCheckBox*)> clickaction;

	ui_tt_icon(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(paint_region) f, IN(std::wstring) str, IN(text_format) tt_format) :
		uiSimpleTooltip(p, ix, iy, iwidth, iheight, str, tt_format), fill(f) { };
	virtual void draw(OGLLock &target, int x, int y);
};


class uiIcon : public uiVRect {
public:
	sf::Texture *tex;
	sf::IntRect texbox;

	uiIcon(int ix, int iy, int iwidth, int iheight, std::weak_ptr<uiElement> p, IN_P(sf::Texture) t, IN(sf::IntRect) box) : uiVRect(ix, iy, iwidth, iheight, p), tex(t), texbox(box) {}

	virtual void draw(OGLLock &target, int x, int y);;
};

class uiPropList : public uiElement {
public:
	sf::iText ctext;
	std::vector<std::wstring> properties;
	std::vector<std::wstring> values;
	float lineheight;
	const paint_region& __restrict fill;

	uiPropList(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, IN(paint_region) paint, IN(text_format) format, IN(std::vector<std::wstring>) iproperties);
	virtual void draw(OGLLock &target, int x, int y);;
};

class uiBar : public uiElement {
public:
	unsigned int barpos = 0;
	std::function<void(uiBar*, double)> clickaction;
	const paint_region& __restrict fill;

	uiBar(int ix, int iy, int iwidth, int iheight, std::weak_ptr<uiElement> p, IN(paint_region) paint, const std::function<void(uiBar*, double)>& func) : uiElement(ix, iy, iwidth, iheight, p), fill(paint), clickaction(func) {};
	uiBar(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(paint_region) paint, const std::function<void(uiBar*, double)>& func) : uiElement(ix, iy, iwidth, iheight, p), fill(paint), clickaction(func) {};

	virtual void draw(OGLLock &target, int x, int y);;
	virtual bool click(int x, int y);;
};

class uiCountDown : public uiElement {
public:
	unsigned int start_date = 0;
	unsigned int end_date = 0;
	const paint_region& __restrict fill;

	std::function<void(uiCountDown*)> finish;
	sf::iText text;

	uiCountDown(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, unsigned int start, unsigned int end, IN(paint_region) paint, const std::wstring &l, IN(text_format) format, const std::function<void(uiCountDown*)>& func);

	virtual void draw(OGLLock &target, int x, int y);
};

class uiTabs : public uiVRect {
public:
	std::vector<std::shared_ptr<uiScrollView>> panes;
	int bheight = 0;

	void init(IN(text_format) format, IN(paint_region) paint, IN(std::vector<std::wstring>) btext);
	uiTabs(int ix, int iy, int iwidth, int iheight, std::weak_ptr<uiElement> p);
	uiTabs(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight) : uiTabs(ix, iy, iwidth, iheight, p) {};
	void update_tab_sizes();
};

class uiPanes : public uiVRect {
public:
	std::vector<std::shared_ptr<uiScrollView>> panes;

	void init(unsigned int n);
	void update_size();
	uiPanes(int ix, int iy, int iwidth, int iheight, std::weak_ptr<uiElement> p);
	uiPanes(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight) : uiPanes(ix, iy, iwidth, iheight, p) {};
	void activate_pane(unsigned int n);
	void deactivate_panes();
	void activate_pane(IN(std::shared_ptr<uiScrollView>) p);
};


template<typename T>
class gb_base : public uiVRect {
public:
	gb_base(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight) : uiVRect(ix, iy, iwidth, iheight, p) {};
	void click_cleanup() {}
};

template<>
class gb_base<std::true_type> : public uiSimpleTooltip {
public:
	const std::wstring def_tt_text;
	gb_base(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(std::wstring) str, IN(text_format) format) : def_tt_text(str), uiSimpleTooltip(p, ix, iy, iwidth, iheight, str, format, true) {};
	gb_base(std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, int iheight, IN(text_format) format) : def_tt_text(TEXT("")), uiSimpleTooltip(p, ix, iy, iwidth, iheight, TEXT(""), format, true) {};
	void click_cleanup() {
		shared_tooltip->setVisible(false);
	}
};

template<int n>
class gb_no_feature {
public:
	using has_feature = std::false_type;
	class members {
	public:
		members() {}
		members(IN_P(gb_base<std::true_type>) base) {}
	};
	template <typename T>
	static bool enabled(IN_P(T) t) { return true; }
	template<typename T>
	static bool toggled(IN_P(T) t) { return false; }
	template<typename T>
	static bool focused(IN_P(T) t) { return false; }
};

class gb_disable {
public:
	using has_feature = std::true_type;
	class members {
	public:
		bool enabled = true;
		gb_base<std::true_type>* const __restrict baseptr;

		members(IN_P(gb_base<std::true_type>) base) : baseptr(base) {};

		void enable() {
			enabled = true;
			baseptr->update_tooltip(baseptr->def_tt_text);
		};

		void disable(IN(std::wstring) reason) {
			if (enabled) {
				enabled = false;
				baseptr->update_tooltip(reason);
			} else {
				baseptr->tt_text += TEXT("\n");
				baseptr->tt_text += reason;
			}
		};
	};

	template<typename T>
	static bool enabled(IN_P(T) t) { return t->enabled; }
};

class gb_toggle {
public:
	using has_feature = std::true_type;
	class members {
	public:
		bool toggled = false;

		bool get_state() const { return toggled; }
		void set_state(const bool state) { toggled = state; }
		void toggle() { toggled = !toggled; }
	};

	template<typename T>
	static bool toggled(IN_P(T) t) { return t->toggled; }
};

class gb_rollover {
public:
	using has_feature = std::true_type;

	class members {
	public:
		bool focused = false;

		bool ro_active() const { return focused; }
		void ro_enter()  { focused = true; }
		void ro_leave() { focused = false; }
	};

	template<typename T>
	static bool focused(IN_P(T) t) { return t->focused; }
};

class gb_text {
public:
	using has_feature = std::true_type;
	class members {
	public:
		members(IN(std::wstring) tx, IN(text_format) format) : text(tx,*format.font,format.csize), stdcolor(format.color), disabledcolor(format.disabled_color) {}
		sf::iText text;
		sf::Color stdcolor;
		sf::Color disabledcolor;

		bool txtmode = false;
		bool leftalign = false;

		void draw_text(IN(OGLLock) target, int x, int y, int height, int width, bool enabled) {
			if (enabled)
				text.setColor(stdcolor);
			else
				text.setColor(disabledcolor);
			sf::FloatRect bnd = text.textBounds();
			if (leftalign)
				text.setPosition(static_cast<int> (x - bnd.left) + 10, static_cast<int> (y - bnd.top + (height - bnd.height) / 2));
			else if (!txtmode)
				text.setPosition(static_cast<int> (x - bnd.left + (width - (bnd.width)) / 2), static_cast<int> (y - bnd.top + (height - bnd.height) / 2));
			else
				text.setPosition(static_cast<int> (x - bnd.left), static_cast<int> (y - bnd.top));

			target->draw(text);
		}
	};

};

template <typename TOOLTIP, typename ROLLOVER, typename DISABLE, typename TEXTTYPE, typename TOGGLE>
class ui_gb : public DISABLE::members, public ROLLOVER::members, public TEXTTYPE::members, public TOGGLE::members, public gb_base<typename type_or<TOOLTIP, typename DISABLE::has_feature>::type> {
private:
	void _draw_text(IN(OGLLock) target, int x, int y, std::true_type) {
		draw_text(target, x, y, pos.height, pos.width, DISABLE::enabled(this));
	}
	void _draw_text(IN(OGLLock) target, int x, int y, std::false_type) {
	}
	void _toggle(std::true_type) {
		toggle();
	}
	void _toggle(std::false_type) {}
	void _enter_dispatch(std::true_type) {
		if (DISABLE::enabled(this))
			ro_enter();
	}
	void _enter_dispatch(std::false_type) {}
	void _leave_dispatch(std::true_type) {
		if (DISABLE::enabled(this))
			ro_leave();
	}
	void _leave_dispatch(std::false_type) {}

public:
	using this_type = ui_gb<TOOLTIP, ROLLOVER, DISABLE, TEXTTYPE, TOGGLE>;
	using base_type = gb_base<typename type_or<TOOLTIP, typename DISABLE::has_feature>::type>;

	std::function<void(this_type*)> clickaction;
	sf::Keyboard::Key bndkey;

	const static int total_states = (TOGGLE::has_feature() ? 2 : 1) * (ROLLOVER::has_feature() ? 2 : 1) + (DISABLE::has_feature() ? 1 : 0);
	const static unsigned int base_states = (TOGGLE::has_feature() ? 2 : 1);
	const static unsigned int disabled_state = base_states * (ROLLOVER::has_feature() ? 2 : 1);
	
	paint_states<total_states> paint;

	ui_gb(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(paint_states<total_states>) pn, IN(std::function<void(this_type*)>) iclickaction, sf::Keyboard::Key defkey = sf::Keyboard::KeyCount)
		: paint(pn), base_type(p,x,y,w,h), bndkey(defkey), clickaction(iclickaction) {}
	ui_gb(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(std::wstring) btext, IN(paint_states<total_states>) pn, IN(text_format) format, IN(std::function<void(this_type*)>) iclickaction, sf::Keyboard::Key defkey = sf::Keyboard::KeyCount)
		: paint(pn), TEXTTYPE::members(btext, format), base_type(p, x, y, w, h), bndkey(defkey), clickaction(iclickaction) { }
	ui_gb(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(paint_states<total_states>) pn, IN(text_format) ttformat, IN(std::function<void(this_type*)>) iclickaction, sf::Keyboard::Key defkey = sf::Keyboard::KeyCount)
		: paint(pn), base_type(p, x, y, w, h, ttformat), DISABLE::members(static_cast<base_type*>(this)), bndkey(defkey), clickaction(iclickaction) {}
	ui_gb(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(std::wstring) btext, IN(paint_states<total_states>) pn, IN(text_format) format, IN(text_format) ttformat, IN(std::function<void(this_type*)>) iclickaction, sf::Keyboard::Key defkey = sf::Keyboard::KeyCount)
		: paint(pn), TEXTTYPE::members(btext, format), DISABLE::members(static_cast<base_type*>(this)),  base_type(p, x, y, w, h, ttformat), bndkey(defkey), clickaction(iclickaction) { }
	ui_gb(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(paint_states<total_states>) pn, IN(std::wstring) tt_text, IN(text_format) ttformat, IN(std::function<void(this_type*)>) iclickaction, sf::Keyboard::Key defkey = sf::Keyboard::KeyCount)
		: paint(pn), base_type(p, x, y, w, h, tt_text, ttformat), DISABLE::members(static_cast<base_type*>(this)),  bndkey(defkey), clickaction(iclickaction) {}
	ui_gb(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(std::wstring) btext, IN(paint_states<total_states>) pn, IN(text_format) format, IN(std::wstring) tt_text, IN(text_format) ttformat, IN(std::function<void(this_type*)>) iclickaction, sf::Keyboard::Key defkey = sf::Keyboard::KeyCount)
		: paint(pn), TEXTTYPE::members(btext, format), DISABLE::members(static_cast<base_type*>(this)), base_type(p, x, y, w, h, tt_text, ttformat), bndkey(defkey), clickaction(iclickaction) { }

	virtual void draw(OGLLock& target, int x, int y) {
		if (visible) {
			if (DISABLE::enabled(this)) {
				paint.set_state((TOGGLE::toggled(this) ? 1 : 0) + base_states * (ROLLOVER::focused(this) ? 1 : 0));
			} else {
				paint.set_state(disabled_state);
			}
			paint.draw(target,x,y,pos.width,pos.height);
			_draw_text(target, x, y, TEXTTYPE::has_feature());
		}
	}
	virtual bool keypress(sf::Keyboard::Key k) {
		if (DISABLE::enabled(this) && visible && k == bndkey) {
			_toggle(TOGGLE::has_feature());
			clickaction(this);
			return true;
		} else {
			return base_type::keypress(k);
		}
	}
	virtual bool click(int x, int y) {
		if (DISABLE::enabled(this) && visible) {
			base_type::click(x, y);
			base_type::click_cleanup();
			_toggle(TOGGLE::has_feature());
			clickaction(this);
			return true;
		} else {
			return false;
		}
	}
	virtual void enter() {
		_enter_dispatch(ROLLOVER::has_feature());
		base_type::enter();
	}
	virtual void leave() {
		_leave_dispatch(ROLLOVER::has_feature());
		base_type::leave();
	}
};

using uiButton = ui_gb<std::false_type, gb_no_feature<0>, gb_no_feature<1>, gb_text, gb_no_feature<2>>;
using uiROButton = ui_gb<std::true_type, gb_rollover, gb_no_feature<0>, gb_no_feature<1>, gb_no_feature<2>>;
using ui_button_disable = ui_gb<std::false_type, gb_no_feature<0>, gb_disable, gb_text, gb_no_feature<1>>;
using ui_toggle_button = ui_gb<std::true_type, gb_no_feature<0>, gb_no_feature<1>, gb_no_feature<2>, gb_toggle>;
using uiGButton = ui_gb<std::true_type, gb_no_feature<0>, gb_no_feature<1>, gb_no_feature<2>, gb_no_feature<3>>;

class ui_stress_button : public ui_gb<std::true_type, gb_no_feature<0>, gb_disable, gb_text, gb_no_feature<1>> {
public:
	using button_type = ui_gb<std::true_type, gb_no_feature<0>, gb_disable, gb_text, gb_no_feature<1>>;
	ui_stress_button(std::weak_ptr<uiElement> p, int x, int y, int w, int h, IN(std::wstring) btext, IN(paint_states<total_states>) pn, IN(text_format) format, size_t stress, IN(text_format) ttformat, IN(std::function<void(this_type*)>) iclickaction, sf::Keyboard::Key defkey = sf::Keyboard::KeyCount);
};

class uiDropDown : public uiButton {
public:
	std::function<void(uiDropDown*, int)> chooseaction;
	bool selecting = false;
	std::shared_ptr<uiMenuRect> obox;
	int chosen = 0;
	
	int num_options = 0;

	void OpenSelect();
	void reset_options();
	void add_option(IN(std::wstring) str, int value);
	void add_option(IN(std::wstring) str, IN(std::function<void()>) f);

	uiDropDown(std::weak_ptr<uiElement> par, int x, int y, int w, int h, IN(paint_region) paint, IN(text_format) format, IN(std::vector<std::wstring>) options,  IN(std::function<void(uiDropDown*, int)>) ichooseaction);
	uiDropDown(std::weak_ptr<uiElement> par, int x, int y, int w, int h, IN(paint_region) paint, IN(text_format) format,  IN(std::function<void(uiDropDown*, int)>) ichooseaction);
	uiDropDown(std::weak_ptr<uiElement> par, int x, int y, int w, int h, IN(paint_region) paint, IN(text_format) format);

	virtual void updatepos();
};

extern const size_t yes_no_array[2];
extern const size_t accept_decline_array[2];
extern const size_t proceed_cancel_array[2];
extern const size_t options_array[2];
extern const size_t accept_postpone_decline_array[3];
extern const size_t approve_abstain_oppose_array[3];

bool make_yes_no_popup_i(IN(std::shared_ptr<uiElement>) parent, IN(std::wstring) title, IN(std::function<void(const std::shared_ptr<uiScrollView>&)>)setup_contents, size_t stress_yes, size_t stress_no, const IN_P(size_t) labels, std::integral_constant<size_t,1>);
bool make_yes_no_popup_i(IN(std::shared_ptr<uiElement>) parent, IN(std::wstring) title, IN(std::function<void(const std::shared_ptr<uiScrollView>&, const std::shared_ptr<ui_stress_button>&, const std::shared_ptr<ui_stress_button>&)>) setup_contents, size_t stress_yes, size_t stress_no, const IN_P(size_t) labels, std::integral_constant<size_t, 3>);

template<typename T>
bool make_yes_no_popup(IN(std::shared_ptr<uiElement>) parent, IN(std::wstring) title, IN(T) setup_contents, size_t stress_yes, size_t stress_no, const IN_P(size_t) labels = yes_no_array) {
	return make_yes_no_popup_i(parent, title, setup_contents, stress_yes, stress_no, labels, decltype(number_of_arguments(setup_contents))());
}

template<typename T>
bool make_yes_no_popup(IN(std::shared_ptr<uiElement>) parent, IN(std::wstring) title, IN(T) setup_contents, double bias_in_favor, const IN_P(size_t) labels = yes_no_array) {
	return make_yes_no_popup_i(parent, title, setup_contents, bias_in_favor > 0.0 ? 0 : static_cast<size_t>(-bias_in_favor * 10.0), bias_in_favor < 0.0 ? 0 : static_cast<size_t>(bias_in_favor * 10.0), labels, decltype(number_of_arguments(setup_contents))());
}

void open_window_centered(IN(std::shared_ptr<uiDragRect>) win);
void open_window_tiled(IN(std::shared_ptr<uiDragRect>) win, bool to_front = true);

int make_trinary_popup(const std::shared_ptr<uiElement> &parent, const std::wstring& title, const std::function<void(const std::shared_ptr<uiScrollView>&)> &setup_contents, const size_t stress_yes, const size_t stress_maybe, const size_t stress_no, const size_t* const labels = accept_postpone_decline_array);
int make_trinary_popup(const std::shared_ptr<uiElement> &parent, const std::wstring& title, const std::function<void(const std::shared_ptr<uiScrollView>&)> &setup_contents, double positive_bias, const size_t* const labels = accept_postpone_decline_array);
// void message_popup(const std::shared_ptr<uiElement> &parent, const std::wstring &title,  const std::function<void(const std::shared_ptr<uiScrollView>&)> &setup_contents);
void modeless_trinary_popup(IN(std::shared_ptr<uiElement>) parent, IN(std::wstring) title, IN(std::function<void(const std::shared_ptr<uiScrollView>&)>) setup_contents, IN(std::function<void(int)>) results , const size_t stress_yes, const size_t stress_maybe, const size_t stress_no, const size_t* const labels = accept_postpone_decline_array);

void init_message_box();
void message_popup(const std::wstring &title, IN(std::function<void(const std::shared_ptr<uiElement>&)>) setup_contents);

void i18n_message_popup(size_t title_text, size_t body_text, const size_t* params = nullptr, size_t numparams = 0);
bool i18n_yes_no_popup(size_t title_text, size_t body_text, int stress_bias, const size_t* params = nullptr, size_t numparams = 0, const IN_P(size_t) labels = yes_no_array);
void i18n_modeless_yes_no_popup(size_t title_text, size_t body_text, int stress_bias, const size_t* params, size_t numparams, std::function<void(bool)> results, const IN_P(size_t) labels = yes_no_array);

class actionbase;

void UpdateTIcon( const std::shared_ptr<uiGButton> &b, title_id_t id, int icosize);
std::shared_ptr<uiGButton> generateTButton(title_id_t tid, const std::shared_ptr<uiElement> &parent, int x, int y, bool large) noexcept;
std::shared_ptr<uiGButton> generateTButton(title_id tid, const std::shared_ptr<uiElement> &parent, int x, int y, bool large) noexcept;
std::shared_ptr<uiGButton> generateButton( char_id_t chid, const std::shared_ptr<uiElement> &parent, int x, int y, bool large);
void UpdateChIcon( const std::shared_ptr<uiGButton> &b, char_id_t chid, int icosize);
void makeBattleResults( title_id winnertitle, title_id losertitle, int winnerloss, int loserloss);
void makeSeigeResults( title_id winnertitle, title_id losertitle, prov_id prov, unsigned int duration);
void makeWarDeclaration( title_id titleA, title_id titleB, unsigned char type = 1);
void makePeaceDeclaration( title_id titleA, title_id titleB);
void close_menus();

class offer;
class alternative;

void make_relationship_announcement( char_id a, char_id b, bool hate, bool gained);
std::shared_ptr<uiButton> character_selection_menu(IN(std::shared_ptr<uiElement>) parent, int x, int y, int w, int h, IN(std::wstring) text, IN(paint_region) paint, IN(text_format) format,  IN(std::function<void(cvector<char_id_t>&)>) get_list, IN(std::function<void(char_id_t)>) choice_made);