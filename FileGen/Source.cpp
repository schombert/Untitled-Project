#include <Windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <ctype.h>

std::string get_label(const std::string &ln) {
	const auto sz = ln.size();
	size_t end_index = 0;
	for (; end_index < sz && ln[end_index] != TEXT(':'); ++end_index) {
	}
	return ln.substr(0, end_index);
}

static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(isspace))));
	return s;
}

void write_tb_string(HANDLE file, const std::string &str, int tabs) {
	std::string towrite = std::string(tabs, '\t') + str + "\r\n";
	WriteFile(file, towrite.c_str(), static_cast<DWORD>(towrite.length()), nullptr, nullptr);
}

void generate_text_header(TCHAR* fname) {
	HANDLE hfile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	std::vector<std::string> labels;

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
					buffer = ltrim(buffer);
					if (buffer.size() > 0) {
						std::string l = get_label(buffer);
						if (l.size() > 0)
							labels.push_back(std::move(l));
					}
					buffer.clear();
				}
			} else {
				buffer += fdata[start];
			}
			++start;
		}

		delete[] fdata;


		HANDLE hfile2 = CreateFile(TEXT("i18n_defines.h"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, NULL);
		size_t cnt = 0;
		WriteFile(hfile2, "#pragma once\r\n", 14, nullptr, nullptr);
		WriteFile(hfile2, "#include \"globalhelpers.h\"\r\n", 28, nullptr, nullptr);
		WriteFile(hfile2, "\r\n", 2, nullptr, nullptr);
		//#pragma once
		//#include "globalhelpers.h"
		for (const auto& s : labels) {
			WriteFile(hfile2, "#define TX_", 11, nullptr, nullptr);
			WriteFile(hfile2, s.c_str(), static_cast<DWORD>(s.size()), nullptr, nullptr);
			WriteFile(hfile2, " ", 1, nullptr, nullptr);
			std::string num = std::to_string(cnt++);
			WriteFile(hfile2, num.c_str(), static_cast<DWORD>(num.size()), nullptr, nullptr);
			WriteFile(hfile2, "\r\n", 2, nullptr, nullptr);
		}
		CloseHandle(hfile2);

		hfile2 = CreateFile(TEXT("i18n_init.cpp"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, NULL);
		WriteFile(hfile2, "#include \"globalhelpers.h\"\r\n", 28, nullptr, nullptr);
		WriteFile(hfile2, "#include \"i18n.h\"\r\n", 19, nullptr, nullptr);
		WriteFile(hfile2, "\r\n", 2, nullptr, nullptr);

		//WriteFile(hfile2, "#define LABEL_TO_STR(x) #x\r\n", 28, nullptr, nullptr);
		//WriteFile(hfile2, "#define LIZEI(x) L##x\r\n", 23, nullptr, nullptr);
		//WriteFile(hfile2, "#define LIZE(x) LIZEI(x)\r\n", 26, nullptr, nullptr);
		//WriteFile(hfile2, "#define ADD_LABEL(x) label_to_index[LIZE(LABEL_TO_STR(x))] = TX_##x\r\n", 69, nullptr, nullptr);
		//WriteFile(hfile2, "\r\n", 2, nullptr, nullptr);

		WriteFile(hfile2, "void init_label_numbers() {\r\n", 29, nullptr, nullptr);
		//ADD_LABEL(YES);
		for (const auto& s : labels) {
			write_tb_string(hfile2, std::string("label_to_index[L\"") + s + "\"] = TX_" + s + ";", 1);
			//WriteFile(hfile2, "\tADD_LABEL(", 11, nullptr, nullptr);
			//WriteFile(hfile2, s.c_str(), static_cast<DWORD>(s.size()), nullptr, nullptr);
			//WriteFile(hfile2, ");\r\n", 4, nullptr, nullptr);
		}
		WriteFile(hfile2, "}", 1, nullptr, nullptr);

		CloseHandle(hfile2);
	} else {
		OutputDebugStringA("ERROR CANNOT FIND FILE\r\n");
		OutputDebugString(fname); OutputDebugStringA("\r\n");
	}
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	int nCmdShow
	) {

	generate_text_header(TEXT("text.txt"));
}