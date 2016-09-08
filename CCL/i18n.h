#pragma once
#include "globalhelpers.h"
#include "uielements.hpp"
#include "i18n_defines.h"

struct h_link_ident {
	unsigned char type = 0;
	union _iu {
		char_id_t c;
		title_id_t t;
		prov_id_t p;
		rel_id_t r;
		cul_id_t u;
		_iu() : c() {};
	} ident;
	h_link_ident() {};
	h_link_ident(char_id_t c) :type(1) { ident.c = c; };
	h_link_ident(prov_id_t c) :type(2) { ident.p = c; };
	h_link_ident(title_id_t c) :type(3) { ident.t = c; };
	h_link_ident(rel_id_t c) :type(4) { ident.r = c; };
	h_link_ident(cul_id_t c) :type(5) { ident.u = c; };
};

using handle_text_result = std::function<void(h_link_ident, const std::wstring&)>;
using text_fn = std::function<void(const handle_text_result&, const size_t*, size_t)>;

extern std::vector<text_fn> text_records;
extern flat_map<std::wstring, size_t> label_to_index;


void init_label_numbers();
void clear_label_numbers();
void construct_tb_record(IN(std::wstring) str);
void load_text_file(TCHAR* fname);
flat_map<std::wstring, size_t>& label_index();
std::wstring get_simple_string(size_t id);
std::wstring get_p_string(size_t id, const size_t* params, size_t count);

std::wstring w_day_to_string(unsigned int iv) noexcept;
std::wstring w_day_to_string_short(unsigned int iv) noexcept;
std::shared_ptr<uiTextBlock> create_tex_block(size_t id, const size_t* params, size_t count, std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, IN(paint_region) paint, IN(text_format) format) noexcept;
std::shared_ptr<uiTextBlock> create_tex_block(size_t id, std::weak_ptr<uiElement> p, int ix, int iy, int iwidth, IN(paint_region) paint, IN(text_format) format) noexcept;
int get_linear_ui(size_t id, const size_t* params, size_t count, IN(std::shared_ptr<uiElement>) p, int ix, int iy, IN(paint_region) paint, IN(text_format) format) noexcept;
int get_linear_ui(size_t id, IN(std::shared_ptr<uiElement>) p, int ix, int iy, IN(paint_region) paint, IN(text_format) format) noexcept;
std::wstring format_double(double v);
std::wstring format_float(float v);