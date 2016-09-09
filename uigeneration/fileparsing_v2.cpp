#include "globalhelpers.h"
#include "fileparsing_v2.h"
#include <windows.h>
#include <locale>

constexpr int spaces_to_tab = 4;
std::string index_values;


int strip_leading_tabs(char* fdata, size_t &offset, size_t file_size) {
	int spcount = 0;
	int tbcount = 0;
	while (offset < file_size) {
		if (fdata[offset] == ' ')
			++spcount;
		else if (fdata[offset] == '\t')
			++tbcount;
		else
			break;
		++offset;
	}
	return tbcount + (spcount / spaces_to_tab);
}

bool parse_identifier(char* fdata, size_t &offset, size_t file_size, INOUT(std::string) result) {
	size_t temp_offset = offset;

	while (temp_offset < file_size) {
		if (isalpha(fdata[temp_offset]) || isdigit(fdata[temp_offset]) || fdata[temp_offset] == '_' || fdata[temp_offset] == '-' || fdata[temp_offset] == '.') {
			result += fdata[temp_offset];
			++temp_offset;
		} else if (fdata[temp_offset] == ':' || fdata[temp_offset] == ' ' || fdata[temp_offset] == '\t') {
			break;
		}  else {
			return false;
		}
	}

	offset = temp_offset;
	return true;
}

bool parse_colon(char* fdata, size_t &offset, size_t file_size) {
	if (offset < file_size && fdata[offset] == ':') {
		offset++;
		return true;
	}
	return false;
}

void strip_newlines(char* fdata, size_t &offset, size_t file_size) {
	while ((fdata[offset] == '\r' || fdata[offset] == '\n') && offset < file_size)
		++offset;
}

std::string parse_remaining(char* fdata, size_t &offset, size_t file_size) {
	std::string ln;

	while (offset < file_size) {
		if (fdata[offset] == '\r' || fdata[offset] == '\n' || fdata[offset] == '#')
			break;
		ln += fdata[offset];
		++offset;
	}

	while (offset < file_size) {
		if (fdata[offset] == '\r' || fdata[offset] == '\n')
			break;
		++offset;
	}

	while (ln.size() > 0) {
		if (ln.back() == ' ' || ln.back() == '\t') {
			ln.pop_back();
		} else {
			break;
		}
	}

	return ln;
}

std::vector<parse_option>* enter_identifier(parse_ident id, INOUT(std::vector<parse_option>) c) {
	if (!(id == parse_ident(""))) {
		for (IN(auto) opt : c) {
			if (opt.node && opt.node->first == id)
				return &(opt.node->second);
		}
	}

	c.emplace_back(id);
	return &(c.back().node->second);
}

void add_to_subnode(char* fdata, size_t &offset, size_t file_size, INOUT(std::vector<std::pair<int, std::vector<parse_option>*>>) stack) {
	std::string ident;
	size_t temp_off = offset;
	if (parse_identifier(fdata, offset, file_size, ident)) {
		strip_leading_tabs(fdata, offset, file_size);
		if (parse_colon(fdata, offset, file_size)) {
			strip_leading_tabs(fdata, offset, file_size);

			parse_ident id(ident);
			
			const auto ptr = enter_identifier(id, *(stack.back().second));
			stack.emplace_back(stack.back().first + 1, ptr);

			strip_leading_tabs(fdata, offset, file_size);
			add_to_subnode(fdata, offset, file_size, stack);
			return;
		}
	}

	offset = temp_off;
	std::string righthand = parse_remaining(fdata, offset, file_size);

	if (righthand.length() > 0)
		stack.back().second->emplace_back(righthand);
}

void parse_line(char* fdata, size_t &offset, size_t file_size, INOUT(std::vector<std::pair<int, std::vector<parse_option>*>>) stack) {
	strip_newlines(fdata, offset, file_size);
	int tab_count = strip_leading_tabs(fdata, offset, file_size);

	while (tab_count < stack.back().first)
		stack.pop_back();

	add_to_subnode(fdata, offset, file_size, stack);
}

void parse_string(char* fdata, size_t &offset, size_t file_size, INOUT(std::vector<parse_option>) results) {
	std::vector<std::pair<int, std::vector<parse_option>*>> stack;
	stack.emplace_back(0, &results);
	while (offset < file_size)
		parse_line(fdata, offset, file_size, stack);
}

void parse_v2(TCHAR* filename, INOUT(std::vector<parse_option>) results) {
	HANDLE hfile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);

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

		size_t offset = 0;
		parse_string(fdata, offset, sz.LowPart, results);

		delete[] fdata;
	} else {
		OutputDebugStringA("ERROR CANNOT FIND FILE\r\n");
		OutputDebugString(filename); OutputDebugStringA("\r\n");
	}
}