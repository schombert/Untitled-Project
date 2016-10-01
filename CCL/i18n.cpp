#include "globalhelpers.h"
#include "i18n.h"
#include "datamanagement.hpp"
#include "structs.hpp"
#include <Windows.h>
#include "ChPane.h"
#include "TPane.h"
#include "ProvPane.h"
#include "RelPane.h"
#include "CulPane.h"

flat_map<std::wstring, size_t> label_to_index;
std::vector<text_fn> text_records;

std::wstring strbuffer;
using str_ref = string_ref_t<std::wstring, &strbuffer>;

void clear_label_numbers() {
	label_to_index.clear();
	label_to_index.shrink_to_fit();
}

flat_map<std::wstring, size_t>& label_index() {
	return label_to_index;
}


text_fn identity_from_string(IN(std::wstring) str) {
	return[s = str_ref(str)](IN(handle_text_result) r, const size_t*, size_t) { r(h_link_ident(), s.get()); };
}

std::wstring format_double(double v) {
	wchar_t buffer[128];
	swprintf_s(buffer, TEXT("%.1f"), v);
	return std::wstring(buffer);
}

std::wstring format_double_ex(double v) {
	wchar_t buffer[128];
	swprintf_s(buffer, TEXT("%.3f"), v);
	return std::wstring(buffer);
}

std::wstring format_float(float v) {
	wchar_t buffer[128];
	swprintf_s(buffer, TEXT("%.1f"), v);
	return std::wstring(buffer);
}

#define SPARAM(x) (x < len ? params[x] : 0)

std::wstring bracketed_substring(IN(std::wstring) str, INOUT(size_t) index) {
	++index;
	const auto start = index;
	int bracket_count = 1;
	do  {
		if (str[index] == '[')
			++bracket_count;
		if (str[index] == ']')
			--bracket_count;
		++index;
	} while (bracket_count > 0 && index < str.size());

	return str.substr(start, (index - start) - 1);
}

text_fn function_from_string(IN(std::wstring) str);

text_fn text_function_from_string(IN(std::wstring) str, size_t index) {
	if (index >= str.size()) {
		static const auto terminator = [](IN(handle_text_result) r, const size_t* params, size_t len) {};
		return terminator;
	}

	if (str[index] != '[') {
		const size_t start = index;
		while (index < str.size() && str[index] != '[') {
			++index;
		}
		if (index == str.size()) {
			return[istr = str_ref(str.substr(start, index - start))](IN(handle_text_result) r, const size_t* params, size_t len) { r(h_link_ident(), istr.get()); };
		} else {
			return[istr = str_ref(str.substr(start, index - start)), remainder = text_function_from_string(str, index)](IN(handle_text_result) r, const size_t* params, size_t len) {
				r(h_link_ident(), istr.get());
				remainder(r, params, len);
			};
		}
	} else {
		const auto fstr = bracketed_substring(str, index);
		if (index == str.size()) {
			return function_from_string(fstr);
		} else {
			return[f = function_from_string(fstr), remainder = text_function_from_string(str, index)](IN(handle_text_result) r, const size_t* params, size_t len) {
				f(r, params, len);
				remainder(r, params, len);
			};
		}
	}
}

#define TF_ERROR			-1
#define TF_TABLE_DEF		0
#define TF_TO_UPPER			1
#define TF_TABLE_LOOKUP		2
#define TF_NAME				3
#define TF_GENDER			4
#define TF_TITLE_FULL_NAME	5
#define TF_PRIMARY_TITLE	6
#define TF_PROVINCE			7
#define TF_DATE				8
#define TF_HLINK			9
#define TF_INTEGER			10
#define TF_PLURAL			11
#define TF_REFERENCE		12
#define TF_TO_LOWER			13
#define TF_TITLE			14
#define TF_FLOAT_1			15
#define TF_NAME_AND_TITLE	16
#define TF_RECURSIVE		17
#define TF_RELIGION			18
#define TF_CULTURE			19
#define TF_FLOAT_3			20

int char_to_tf_number(wchar_t ch) {
	switch (ch) {
	case L'U': return TF_TO_UPPER;
	case L'u': return TF_TO_LOWER;
	case L'X': return TF_TABLE_LOOKUP;
	case L'N': return TF_NAME;
	case L'n': return TF_NAME_AND_TITLE;
	case L'G': return TF_GENDER;
	case L'T': return TF_TITLE_FULL_NAME;
	case L't': return TF_TITLE;
	case L'P': return TF_PRIMARY_TITLE;
	case L'L': return TF_PROVINCE;
	case L'D': return TF_DATE;
	case L'R': return TF_REFERENCE;
	case L'H': return TF_HLINK;
	case L'#': return TF_INTEGER;
	case L'f': return TF_FLOAT_1;
	case L'F': return TF_FLOAT_3;
	case L'r': return TF_RELIGION;
	case L'c': return TF_CULTURE;
	case L'M': return TF_PLURAL;
	case L'x': return TF_RECURSIVE;
	case L'|': return TF_TABLE_DEF;
	default: return TF_ERROR;
	}
}

size_t wstr_to_sz(IN(std::wstring) str) {
	return static_cast<size_t>(_wtoll(str.c_str()));
}

text_fn function_from_string(IN(std::wstring) str) {
	if (str.size() <= 1) {
		OutputDebugStringA("Encountered bracketed function that was too small\r\n");
		return [](IN(handle_text_result) r, const size_t* params, size_t len) {};
	}

	size_t index = 2;

	switch (char_to_tf_number(str[0])) {
	case TF_ERROR:
		OutputDebugStringA("Encountered unknown text function type\r\n");
		return [](IN(handle_text_result) r, const size_t* params, size_t len) {};
	case TF_TABLE_DEF:
	{
		std::vector<str_ref> tablecontents;
		const auto sz = str.length();
		std::wstring tscratch;

		index = 1;
		while (index < sz) {
			if (str[index] == TEXT('|')) {
				tablecontents.emplace_back(tscratch);
				tscratch.clear();
			} else {
				tscratch += str[index];
			}
			++index;
		}
		if (tscratch.size() > 0) {
			tablecontents.emplace_back(tscratch);
			tscratch.clear();
		}

		return[contents = std::move(tablecontents)](IN(handle_text_result) r, const size_t* params, size_t len) {
			const auto i = SPARAM(0);
			if (i < contents.size())
				r(h_link_ident(), contents[i].get());
			else
				r(h_link_ident(), std::wstring(TEXT("NULL-")) + std::to_wstring(i));
		};
	}
	case TF_HLINK:
	{
		unsigned int parameter_a = 0;
		if (str[index] == TEXT('C')) {
			parameter_a = 1;
		} else if (str[index] == TEXT('P')) {
			parameter_a = 2;
		} else if (str[index] == TEXT('T')) {
			parameter_a = 3;
		} else if (str[index] == TEXT('R')) {
			parameter_a = 4;
		} else if (str[index] == TEXT('U')) {
			parameter_a = 5;
		}

		++index;
		++index; //skip ","

		std::wstring scratch;
		const auto sz = str.length();
		while (index < sz && str[index] != TEXT(',')) {
			scratch += str[index];
			++index;
		}
		if (index < sz)
			++index; //eat ','

		int parameter_b = _wtoi(scratch.c_str());

		switch (parameter_a) {
		case 1:
			return[f = text_function_from_string(str, index), parameter_b](IN(handle_text_result) r, const size_t* params, size_t len) {
				const auto toh = [&r, hl = h_link_ident(char_id_t(static_cast<char_id>(SPARAM(parameter_b))))](h_link_ident i, IN(std::wstring) str) {
					r(hl, str);
				};
				f(toh, params, len);
			};
		case 2:
			return[f = text_function_from_string(str, index), parameter_b](IN(handle_text_result) r, const size_t* params, size_t len) {
				const auto toh = [&r, hl = h_link_ident(prov_id_t(static_cast<prov_id>(SPARAM(parameter_b))))](h_link_ident i, IN(std::wstring) str) {
					r(hl, str);
				};
				f(toh, params, len);
			};
		case 3:
			return[f = text_function_from_string(str, index), parameter_b](IN(handle_text_result) r, const size_t* params, size_t len) {
				const auto toh = [&r, hl = h_link_ident(title_id_t(static_cast<title_id>(SPARAM(parameter_b))))](h_link_ident i, IN(std::wstring) str) {
					r(hl, str);
				};
				f(toh, params, len);
			};
		case 4:
			return[f = text_function_from_string(str, index), parameter_b](IN(handle_text_result) r, const size_t* params, size_t len) {
				const auto toh = [&r, hl = h_link_ident(rel_id_t(static_cast<rel_id>(SPARAM(parameter_b))))](h_link_ident i, IN(std::wstring) str) {
					r(hl, str);
				};
				f(toh, params, len);
			};
		case 5:
			return[f = text_function_from_string(str, index), parameter_b](IN(handle_text_result) r, const size_t* params, size_t len) {
				const auto toh = [&r, hl = h_link_ident(cul_id_t(static_cast<cul_id>(SPARAM(parameter_b))))](h_link_ident i, IN(std::wstring) str) {
					r(hl, str);
				};
				f(toh, params, len);
			};
		default:
			return text_function_from_string(str, index);
		}
		break;
	}
	case TF_NAME:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if(parameter_b < len)
				r(h_link_ident(char_id_t(static_cast<char_id>(params[parameter_b]))),
					global::w_character_name(char_id_t(static_cast<char_id>(params[parameter_b]))));
		};
	case TF_GENDER:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (parameter_b < len)
				r(h_link_ident(), std::to_wstring(detail::people[params[parameter_b]].gender));
		};
	case TF_TITLE_FULL_NAME:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (parameter_b < len)
				r(h_link_ident(title_id_t(static_cast<title_id>(params[parameter_b]))),
					global::w_title_name(title_id_t(static_cast<title_id>(params[parameter_b]))));
		};
	case TF_PRIMARY_TITLE:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (parameter_b < len)
				r(h_link_ident(title_id_t(detail::people[params[parameter_b]].primetitle)),
					global::w_title_name(detail::people[params[parameter_b]].primetitle));
		};
	case TF_PROVINCE:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (parameter_b < len)
				r(h_link_ident(prov_id_t(static_cast<prov_id>(params[parameter_b]))),
					global::w_province_name(prov_id_t(static_cast<prov_id>(params[parameter_b]))));
		};
	case TF_DATE:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			r(h_link_ident(), w_day_to_string(static_cast<unsigned int>(SPARAM(parameter_b))));
		};
	case TF_INTEGER:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			r(h_link_ident(), std::to_wstring(dumb_cast<__int64>(SPARAM(parameter_b))));
		};
	case TF_TITLE:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (parameter_b < len)
				r(h_link_ident(title_id_t(static_cast<title_id>(params[parameter_b]))),
					str_to_wstr(detail::titles[params[parameter_b]].rname.get()));
		};
	case TF_FLOAT_1:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			r(h_link_ident(), format_double(dumb_cast<double>(SPARAM(parameter_b))));
		};
	case TF_NAME_AND_TITLE:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (parameter_b < len)
				r(h_link_ident(char_id_t(static_cast<char_id>(params[parameter_b]))),
					global::get_expanded_name(char_id_t(static_cast<char_id>(params[parameter_b]))));
		};
	case TF_RELIGION:
		return [parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (parameter_b < len)
				r(h_link_ident(rel_id_t(static_cast<rel_id>(params[parameter_b]))),
					str_to_wstr(detail::religions[params[parameter_b]].name.get()));
		};
	case TF_CULTURE:
		return[parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (parameter_b < len)
				r(h_link_ident(cul_id_t(static_cast<cul_id>(params[parameter_b]))),
					str_to_wstr(detail::cultures[params[parameter_b]].name.get()));
		};
	case TF_FLOAT_3:
		return[parameter_b = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
			r(h_link_ident(), format_double_ex(dumb_cast<double>(SPARAM(parameter_b))));
		};
	case TF_TO_UPPER:
		return[f = text_function_from_string(str, index)](IN(handle_text_result) r, const size_t* params, size_t len) {
			const auto tou = [&r](h_link_ident i, IN(std::wstring) str) {
				std::wstring original_str = str;
				if (original_str.size() > 0)
					original_str[0] = towupper(original_str[0]);
				r(i, original_str);
			};
			f(tou, params, len);
		};
	case TF_TO_LOWER:
		return[f = text_function_from_string(str, index)](IN(handle_text_result) r, const size_t* params, size_t len) {
			const auto tol = [&r](h_link_ident i, IN(std::wstring) str) {
				std::wstring original_str = str;
				if (original_str.size() > 0)
					original_str[0] = towlower(original_str[0]);
				r(i, original_str);
			};
			f(tol, params, len);
		};
	case TF_RECURSIVE:
	case TF_REFERENCE:
	{
		auto it = label_to_index.find(str.substr(index));
		if (it != label_to_index.end()) {
			return [parameter_a = static_cast<unsigned int>(it->second)](IN(handle_text_result) r, const size_t* params, size_t len) {
				text_records[parameter_a](r, params, len);
			};
		} else {
			return [parameter_a = wstr_to_sz(str.substr(index))](IN(handle_text_result) r, const size_t* params, size_t len) {
				if(parameter_a < len)
					text_records[params[parameter_a]](r, params, len);
			};
		}
	}
	case TF_TABLE_LOOKUP:
	{
		//eat label
		std::wstring tscratch;
		const auto sz = str.length();
		while (index < sz && str[index] != TEXT(',')) {
			tscratch += str[index];
			++index;
		}
		if (index < sz)
			++index; //eat ','

		auto it = label_to_index.find(tscratch);
		if (it != label_to_index.end()) {
			if (str[index] != '[') {
				return[table_param = wstr_to_sz(str.substr(index)), parameter_a = static_cast<unsigned int>(it->second)](IN(handle_text_result) r, const size_t* params, size_t len) {
					if (table_param < len)
						text_records[parameter_a](r, &params[table_param], 1);
				};
			} else {
				return[f = text_function_from_string(str, index), parameter_a = static_cast<unsigned int>(it->second)](IN(handle_text_result) r, const size_t* params, size_t len) {
					std::wstring result;
					f([&result](h_link_ident i, IN(std::wstring) str) { result += str; }, params, len);
					size_t iresult = wstr_to_sz(result);
					text_records[parameter_a](r, &iresult, 1);
				};
			}
		} else {
			return [](IN(handle_text_result) r, const size_t* params, size_t len) {};
		}
		
	}
	case TF_PLURAL:
	{
		std::wstring scratch;
		const auto sz = str.length();
		while (index < sz && str[index] != TEXT(',')) {
			scratch += str[index];
			++index;
		}
		if (index < sz)
			++index; //eat ','
		const int parameter_a = _wtoi(scratch.c_str());
		scratch.clear();

		// first text string
		const auto sz2 = str.length();
		while (index < sz2 && str[index] != TEXT(',')) {
			scratch += str[index];
			++index;
		}
		if (index < sz)
			++index; //eat ','

		return [singular = str_ref(scratch), plural = str_ref(str.substr(index)), parameter_a](IN(handle_text_result) r, const size_t* params, size_t len) {
			if (SPARAM(parameter_a) == 1)
				r(h_link_ident(), singular.get());
			else
				r(h_link_ident(), plural.get());
		};
	}
	default:
		return [](IN(handle_text_result) r, const size_t* params, size_t len) {};
	}
}

void construct_tb_record(INOUT(std::wstring) str) {
	size_t init_index = 0;
	size_t end_index = 0;

	const auto sz = str.size();

	//get label
	for (; end_index < sz && str[end_index] != TEXT(':'); ++end_index) {
	}
	size_t indx;
	auto it = label_to_index.find(str.substr(0, end_index));
	if (it == label_to_index.end()) {
		indx = label_to_index.size();
		label_to_index[str.substr(0, end_index)] = indx;
		OutputDebugStringA("MISSING LABEL: ");
		OutputDebugStringW(str.substr(0, end_index).c_str());
		OutputDebugStringA("\r\n");
		return;
	} else {
		indx = it->second;
	}
	if(text_records.size() <= indx)
		text_records.resize(indx + 1);

	++end_index; // eat ':'

	for (size_t x = end_index; x < sz; ++x) {
		if (str[x] == TEXT('~'))
			str[x] = TEXT(' ');
	}

	/*std::vector<text_fn> results;

	for (; end_index < sz; ++end_index) {
		if (str[end_index] == TEXT('[')) {
			if(init_index != end_index)
				results.emplace_back(identity_from_string(str.substr(init_index, end_index - init_index)));
			++end_index;

			results.emplace_back(function_from_string(str, end_index));

			while (str[end_index] == TEXT(']') && end_index < sz) {
				++end_index;
			}
			--end_index;

			init_index = end_index+1;
		}
	}

	if (init_index != end_index) {
		results.emplace_back(identity_from_string(str.substr(init_index, end_index - init_index)));
	} */

	text_records[indx] = text_function_from_string(str, end_index);
	
}

// trim from start
static inline std::wstring &ltrim(std::wstring &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from start
static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::wstring &rtrim(std::wstring &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::wstring &trim(std::wstring &s) {
	return ltrim(rtrim(s));
}

std::string get_label(IN(std::string) ln) {
	const auto sz = ln.size();
	size_t end_index = 0;
	for (; end_index < sz && ln[end_index] != TEXT(':'); ++end_index) {
	}
	return ln.substr(0, end_index);
}

void load_text_file(TCHAR * fname) {
	HANDLE hfile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	if (hfile != INVALID_HANDLE_VALUE) {

		LARGE_INTEGER sz;
		DWORD bytes;

		GetFileSizeEx(hfile, &sz);
		//detect and erase BOM
		unsigned __int8 bom[3] = "\0\0";
		if (ReadFile(hfile, bom, 3, &bytes, NULL)) {
			if (bom[0] != 239 || bom[1] != 187 || bom[2] != 191) {
				SetFilePointer(hfile, 0, NULL, FILE_BEGIN);
			} else {
				sz.LowPart -= 3;
			}
		}

		char* fdata = new char[sz.LowPart + 1];
		fdata[sz.LowPart] = 0;

		const BOOL read = ReadFile(hfile, fdata, sz.LowPart, &bytes, NULL);
		CloseHandle(hfile);
		
		size_t start = 0;
		std::string buffer;

		while (start <= sz.LowPart) {
			if (start == sz.LowPart || fdata[start] == '\r' || fdata[start] == '\n') {
				if (buffer.size() > 0) {
					std::wstring line = str_to_wstr(buffer);
					trim(line);
					//OutputDebugStringA("LINE: ");
					//OutputDebugStringW(line.c_str());
					//OutputDebugStringA("\r\n");
					if (line.size() > 0) {
						construct_tb_record(line);
					}
					buffer.clear();
				}
			} else {
				buffer += fdata[start];
			}
			++start;
		}

		delete[] fdata;
	} else {
		OutputDebugStringA("ERROR CANNOT FIND FILE\r\n");
		OutputDebugString(fname); OutputDebugStringA("\r\n");
	}
}


std::wstring get_simple_string(size_t id) {
	return get_p_string(id, nullptr, 0);
}

std::wstring get_p_string(size_t id, const size_t* params, size_t count) {
	std::wstring result;
	text_records[id]([&result](h_link_ident, IN(std::wstring) s) { result += s; }, params, count);
	return result;
}

std::wstring w_day_to_string(unsigned int iv) noexcept {
	const boost::gregorian::date d = boost::gregorian::date(1400, boost::gregorian::Jan, 1) + boost::gregorian::days(iv);
	size_t params[3] = {d.day(),d.month(),d.year() - 1400ui64};
	return get_p_string(TX_DATE, params, 3);
}

std::wstring w_day_to_string_short(unsigned int iv) noexcept {
	const boost::gregorian::date d = boost::gregorian::date(1400, boost::gregorian::Jan, 1) + boost::gregorian::days(iv);
	size_t params[3] = {d.day(), d.month(), d.year() - 1400ui64};
	return get_p_string(TX_S_DATE, params, 3);
}

int create_tex_block(size_t id, IN(std::shared_ptr<uiElement>) p, int ix, int iy, int iwidth, IN(paint_region) paint, IN(text_format) format) noexcept {
	return create_tex_block(id, nullptr, 0, p, ix, iy, iwidth, paint, format);
}

int create_tex_block(size_t id, const size_t * params, size_t count, IN(std::shared_ptr<uiElement>) p, int ix, int iy, int iwidth, IN(paint_region) paint, IN(text_format) format) noexcept {
	cvector<layoutelement> layout;

	cvector<std::wstring> tstrings;
	cvector<h_link_ident> links;

	text_records[id]([&tstrings, &links](h_link_ident i, IN(std::wstring) s) { links.emplace_back(i); tstrings.emplace_back(s); }, params, count);

	const auto tb = p->add_element<uiTextBlock>(ix, iy, iwidth, layout, paint, format, tstrings);

	for (size_t i = 0; i < links.size(); ++i) {
		if(links[i].type == 1)
			tb->addLink(i, layout, [t = links[i].ident.c](uiInvisibleButton*) { SetupChPane(char_id_t(t)); });
		else if (links[i].type == 2)
			tb->addLink(i, layout, [t = links[i].ident.p](uiInvisibleButton*) { SetupProvPane(prov_id_t(t)); });
		else if (links[i].type == 3)
			tb->addLink(i, layout, [t = links[i].ident.t](uiInvisibleButton*) { SetupTPane(title_id_t(t)); });
		else if (links[i].type == 4)
			tb->addLink(i, layout, [t = links[i].ident.r](uiInvisibleButton*) { SetupRelPane(rel_id_t(t)); });
		else if (links[i].type == 5)
			tb->addLink(i, layout, [t = links[i].ident.u](uiInvisibleButton*) { SetupCulPane(cul_id_t(t)); });
	}

	return tb->pos.height;
}

int get_linear_ui(size_t id, const size_t* params, size_t count, IN(std::shared_ptr<uiElement>) p, int ix, int iy, IN(paint_region) paint, IN(text_format) format) noexcept {
	int current_x = ix;

	text_records[id]([&current_x, &p, ix, iy, &paint, &format](h_link_ident i, IN(std::wstring) s) {
		if (s.length() > 0) {
			if (i.type == 0) {
				const auto r = p->add_element<uiSimpleText>(current_x, iy, s, paint, format);
				current_x += r->pos.width;
			} else if (i.type == 1) {
				const auto r = p->add_element<uiHLink>(current_x, iy, s, paint, format, global::whandle, [t = i.ident.c](uiHLink* h) { SetupChPane(t); });
				current_x += r->pos.width;
			} else if (i.type == 2) {
				const auto r = p->add_element<uiHLink>(current_x, iy, s, paint, format, global::whandle, [t = i.ident.p](uiHLink* h) { SetupProvPane(t); });
				current_x += r->pos.width;
			} else if (i.type == 3) {
				const auto r = p->add_element<uiHLink>(current_x, iy, s, paint, format, global::whandle, [t = i.ident.t](uiHLink* h) { SetupTPane(t); });
				current_x += r->pos.width;
			} else if (i.type == 4) {
				const auto r = p->add_element<uiHLink>(current_x, iy, s, paint, format, global::whandle, [t = i.ident.r](uiHLink* h) { SetupRelPane(t); });
				current_x += r->pos.width;
			} else if (i.type == 5) {
				const auto r = p->add_element<uiHLink>(current_x, iy, s, paint, format, global::whandle, [t = i.ident.u](uiHLink* h) { SetupCulPane(t); });
				current_x += r->pos.width;
			}
		}

	}, params, count);

	return current_x;
}

int get_linear_ui(size_t id, IN(std::shared_ptr<uiElement>) p, int ix, int iy, IN(paint_region) paint, IN(text_format) format) noexcept {
	return get_linear_ui(id, nullptr, 0, p, ix, iy, paint, format);
}
