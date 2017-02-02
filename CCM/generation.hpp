#pragma once
#include "globalhelpers.h"
#include "structs.hpp"
#include <windows.h>
#include <SFML\Graphics.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include "sqlite3.h"
#include <memory>

using namespace std;

bool file_exists(const wchar_t* szPath) noexcept;
bool file_exists(const char* szPath) noexcept;

void generate_data(const char* szPath);
void load_save(const char *zFilename);
void load_save_no_ui(const char *zFilename);

int distsq(int x1, int y1, int x2, int y2);
void GetProvData(sqlite3* db, int id, std::string &culture, std::string &religion, std::string &holdername, std::string &hdynasty, int &titletype, int &gender);
