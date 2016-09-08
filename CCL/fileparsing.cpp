#include "globalhelpers.h"
#include "fileparsing.h"

std::string ParseTerm(char* istr, unsigned int &start, unsigned int size) {
	std::string building;
	bool quoteterm = false;

	if (start < size && istr[start] == '\"') {
		quoteterm = true;
		start++;
	}

	while (start < size) {
		if ((!quoteterm && (istr[start] == ' ' || istr[start] == '}' || istr[start] == '=' || istr[start] == '\r' || istr[start] == '\n' || istr[start] == '\t')) ||
			(quoteterm && (istr[start] == '\"' || istr[start] == '\r' || istr[start] == '\n'))) {
			if (istr[start] == '=' || istr[start] == '}')
				start--;
			return building;
		} else {
			building += istr[start];
		}
		start++;
	}

	return building;
}

void ParseListCh(std::unique_ptr<prse_list> &results, char* istr, unsigned int &start, unsigned int size, bool(*exclude)(std::string&)) {
	int state = 0;
	bool assoc = false;
	std::string left;
	while (start < size) {
		if (state == 0 || state == 2) {
			if (istr[start] == ' ' || istr[start] == '\r' || istr[start] == '\n' || istr[start] == '\t') {

			} else if (istr[start] == '}') {
				if (left.size() != 0) {
					//push raw value
					//results.push_back(left);
					results->list.emplace_back(nullptr, std::make_unique<prse_value>(left), nullptr);
				}
				return;
			} else if (istr[start] == '#') {
				state = 1;
			} else if (istr[start] == '=') {
				// ASSOC
				assoc = true;
			} else if (istr[start] == '{') {
				// LIST

				start++;
				std::unique_ptr<prse_list> newlist = std::make_unique<prse_list>();
				ParseListCh(newlist, istr, start, size, exclude);
				if (exclude == NULL || !exclude(left)) {
					newlist->value = left;
					results->list.emplace_back(nullptr, nullptr, std::move(newlist));
				}
				left = "";
				assoc = false;
			} else {
				// TERM
				if (left.size() != 0 && !assoc) {
					//push raw value
					results->list.emplace_back(nullptr, std::make_unique<prse_value>(left), nullptr);
				}

				std::string nstr = ParseTerm(istr, start, size);
				if (assoc) {
					//results.push_back(pair<string, string>(left, nstr));
					results->list.emplace_back(std::make_unique<prse_assoc>(left, nstr), nullptr, nullptr);
					assoc = false;
					left = "";
				} else {
					left = nstr;
				}
			}
		} else if (state == 1) { //comment
			if (istr[start] == '\r' || istr[start] == '\n') {
				state = 0;
			}
		}

		start++;
	}
}

void ParseCKFile(TCHAR* filename, std::unique_ptr<prse_list>  &results, bool(*exclude)(std::string&)) {
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
		unsigned int start = 0;
		if (read)
			ParseListCh(results, fdata, start, sz.LowPart, exclude);
		delete[] fdata;
	} else {
		OutputDebugStringA("ERROR CANNOT FIND FILE\r\n");
		OutputDebugString(filename); OutputDebugStringA("\r\n");
	}
}