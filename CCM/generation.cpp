#include "globalhelpers.h"
#include "structs.hpp"
#include "generation.hpp"
#include <windows.h>
#include "sqlite3.h"
#include <vector>
#include <string>
#include <regex>
#include <deque>
#include <tuple>
#include "structs.hpp"
#include <amp_math.h>
#include <list>
#include "SOIL.h"
#include "prov_control.h"
#include "living_data.h"
#include "datamanagement.hpp"
#include "fileparsing.h"
#include "laws.h"
#include "prov_control.h"
#include "wardata.h"
#include "court.h"

using namespace boost::gregorian;

#define START_DATE 867*372
#define START_YEAR_TX "867"

using namespace std;

size_t first_county = 1;
size_t first_non_county = SIZE_MAX;
flat_map<int, prov_id> pnumremappings;
flat_map<std::string, title_id> tnumremappings;
flat_map<__int64, dyn_id> dnumremappings;

cul_id str_to_cid(IN(std::string) str) {
	for (cul_id indx = 0; indx != detail::cultures.size(); ++ indx) {
		if (detail::cultures[indx].name.get() == str)
			return indx;
	}
	return 0;
}

rel_id str_to_rid(IN(std::string) str) {
	for (rel_id indx = 0; indx != detail::religions.size(); ++indx) {
		if (detail::religions[indx].name.get() == str)
			return indx;
	}
	return 0;
}

void LoadClimates() {
	std::unique_ptr<prse_list>  results = std::make_unique<prse_list>() ;
	ParseCKFile(L"map\\climate.txt", results, NULL);
	for (const auto &val : results->list) {
		if (val.list) {
			//pair<string, vector<boost::any>> lst = boost::any_cast<pair<string, vector<boost::any>>>(val);
			unsigned char clim = 0;
			if (val.list->value == "normal_winter")
				clim = 1;
			else if (val.list->value == "severe_winter")
				clim = 2;
			for (const auto &vx : val.list->list) {
				if (vx.value) {
					int st = stol(vx.value->value, NULL, 10);
					if (detail::provinces.size() <= pnumremappings[st])
						detail::provinces.resize(pnumremappings[st] + 1);
					detail::provinces[pnumremappings[st]].climate = clim;
				}
			}
		}
	}
}

void LoadWater() {
	std::unique_ptr<prse_list>  results = std::make_unique<prse_list>();
	ParseCKFile(L"map\\default.map", results, NULL);
	for (const auto& val : results->list) {
		if (val.list) {
			//pair<string, vector<boost::any>> lst = boost::any_cast<pair<string, vector<boost::any>>>(val);
			if (val.list->value == "sea_zones") {
				if (val.list->list.size() >= 2){
					if (val.list->list[0].value && val.list->list[1].value) {
						int st = stol(val.list->list[0].value->value, NULL, 10);
						int ed = stol(val.list->list[1].value->value, NULL, 10);
						while (st <= ed){
							if (detail::provinces.size() <= pnumremappings[st])
								detail::provinces.resize(pnumremappings[st] + 1);
							detail::provinces[pnumremappings[st]].pflags |= PROV_FLAG_WATER;
							st++;
						}
					}
				}
			}
			else if (val.list->value == "major_rivers") {
				for (const auto &vx : val.list->list) {
					if (vx.value) {
						int st = stol(vx.value->value, NULL, 10);
						if (detail::provinces.size() <= pnumremappings[st])
							detail::provinces.resize(pnumremappings[st] + 1);
						detail::provinces[pnumremappings[st]].pflags |= PROV_FLAG_WATER;
					}
				}
			}

		}
	}
}

void clrhlper(flat_map<__int32, int> &colortoterrain, int red, int green, int blue, int indx) {
	__int8 colors[4] = { (__int8)red, (__int8)green, (__int8)blue, (__int8)0 };
	__int32 *cptr = (__int32*)colors;
	colortoterrain.emplace(*cptr, indx);
}

void LoadTerrain( flat_map<__int32, prov_id> &colortoprov, unsigned char* provs, unsigned int pw, unsigned int ph, unsigned int pch, unsigned char* terrain, int tw, int tch) {
	flat_map<__int32, int> colortoterrain;

	clrhlper(colortoterrain, 69, 91, 186, 0);
	clrhlper(colortoterrain, 86, 124, 27, 1);
	clrhlper(colortoterrain, 0, 86, 6, 2);
	clrhlper(colortoterrain, 65, 42, 17, 3);
	clrhlper(colortoterrain, 155, 155, 155, 4);
	clrhlper(colortoterrain, 255, 255, 255, 5);
	clrhlper(colortoterrain, 206, 169, 9, 6);

	clrhlper(colortoterrain, 130, 158, 75, 7);
	clrhlper(colortoterrain, 138, 11, 26, 8);
	clrhlper(colortoterrain, 13, 96, 62, 9);
	clrhlper(colortoterrain, 40, 180, 149, 10);
	clrhlper(colortoterrain, 86, 46, 0, 11);
	clrhlper(colortoterrain, 112, 74, 31, 12);
	clrhlper(colortoterrain, 255, 186, 0, 13);

	_int8 colors[4] = { 0, 0, 0, 0 };
	__int32 *cptr = (__int32*)colors;

	//sf::Vector2u size = provs.getSize();


	for (unsigned int y = 0; y < ph; y++) {
		for (unsigned int x = 0; x < pw; x++) {
			sf::Color ct = get_color_from_raw(provs, x, y, pw, pch);//provs.getPixel(x, y);
			colors[0] = ct.r; colors[1] = ct.g; colors[2] = ct.b;
			if (colortoprov.count(*cptr) > 0) {
				int prov = colortoprov[*cptr];
				ct = get_color_from_raw(terrain, x, y, tw, tch);//terrain.getPixel(x, y);
				colors[0] = ct.r; colors[1] = ct.g; colors[2] = ct.b;
				if (colortoterrain.count(*cptr) > 0) {
					int tinx = colortoterrain[*cptr];

					(detail::provinces[prov].terrain[tinx])++;
				}
			}
		}
	}
}


int distsq(int x1, int y1, int x2, int y2) {
	return ((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2));
}


void makeConnections(){
	//for (unsigned int i = 0; i < provinces.size(); i++) {
		//for (auto &cnb : provinces[i].connections) {
			//float dist = static_cast<float>(distsq(provinces[i].bounds.left + provinces[i].bounds.width / 2, provinces[i].bounds.top + provinces[i].bounds.height / 2,
			//	provinces[cnb.dest].bounds.left + provinces[cnb.dest].bounds.width / 2, provinces[cnb.dest].bounds.top + provinces[cnb.dest].bounds.height / 2));
			//dist = concurrency::fast_math::sqrt(dist);
			//cnb.distance = dist;

			//if (b.provinceA == i) {
			//	c.dest = b.provinceB;
			//	provinces[i].connections.push_back(c);
			//}
			//else if (b.provinceB == i) {
			//	c.dest = b.provinceA;
			//	provinces[i].connections.push_back(c);
			//}
		//}
	//}
}

void LoadAjac(INOUT(flat_multimap<edge, std::vector<sf::Vector2<short>>>) borders) {

	HANDLE hfile = CreateFile(L"map\\adjacencies.csv", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);

	std::regex parseline("^([0-9]+);([0-9]+);.*$");
	std::smatch m;

	if (hfile != INVALID_HANDLE_VALUE) {
		DWORD bytes;
		std::string cline("");

		LARGE_INTEGER sz;
		GetFileSizeEx(hfile, &sz);
		//detect and erase BOM
		unsigned __int8 bom[3] = "\0\0";
		if (ReadFile(hfile, bom, 3, &bytes, nullptr)) {
			if (bom[0] != 239 || bom[1] != 187 || bom[2] != 191) {
				SetFilePointer(hfile, 0, nullptr, FILE_BEGIN);
			} else {
				sz.LowPart -= 3;
			}
		}

		char* fdata = new char[sz.LowPart + 1];
		fdata[sz.LowPart] = 0;

		OutputDebugStringA("BEGIN READ\r\n");

		const BOOL read = ReadFile(hfile, fdata, sz.LowPart, &bytes, nullptr);
		CloseHandle(hfile);

		if (read) {
			std::string istr(fdata, sz.LowPart);

			OutputDebugStringA("BEGIN REGEX\r\n");


			size_t strt = 0;
			size_t end = 0;
			while (strt != std::string::npos) {
				end = istr.find_first_of("\r\n", strt);
				if (end == std::string::npos) {
					end = istr.size();
				}

				if (std::regex_match(istr.cbegin() + strt, istr.cbegin() + end, m, parseline)) {
					prov_id p1 = pnumremappings[stol(m[1].str(), nullptr, 10)];
					prov_id p2 = pnumremappings[stol(m[2].str(), nullptr, 10)];

					emplace_vv_unique(global::province_connections, p1, p2);
					emplace_vv_unique(global::province_connections, p2, p1);

					
					borders.emplace(edge(p1, p2), std::vector<sf::Vector2<short>>());
					//OutputDebugStringA((std::to_string(p1) + "<=>" + std::to_string(p2) + "\r\n").c_str());
				}

				strt = istr.find_first_not_of("\r\n", end);
			}
		}

		delete[] fdata;

		OutputDebugStringA("END READ\r\n");
	}
	else {
		OutputDebugStringA("ERROR CANNOT FIND FILE\r\n");
	}
}

void LoadProv(INOUT(flat_map<__int32, prov_id>) colortoprov) {
	HANDLE hfile = CreateFile(L"map\\definition.csv", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);

	std::regex parseline("^([0-9]+);([0-9]+);([0-9]+);([0-9]+);([^;]*);.*$");
	std::smatch m;

	__int8 colors[4] = { 0, 0, 0, 0 };
	__int32 *cptr = (__int32*)colors;

	if (hfile != INVALID_HANDLE_VALUE) {
		DWORD bytes;
		std::string cline("");

		LARGE_INTEGER sz;
		GetFileSizeEx(hfile, &sz);
		//detect and erase BOM
		unsigned __int8 bom[3] = "\0\0";
		if (ReadFile(hfile, bom, 3, &bytes, nullptr)) {
			if (bom[0] != 239 || bom[1] != 187 || bom[2] != 191) {
				SetFilePointer(hfile, 0, nullptr, FILE_BEGIN);
			} else {
				sz.LowPart -= 3;
			}
		}

		char* fdata = new char[sz.LowPart + 1];
		fdata[sz.LowPart] = 0;

		OutputDebugStringA("BEGIN READ\r\n");

		const BOOL read = ReadFile(hfile, fdata, sz.LowPart, &bytes, nullptr);
		CloseHandle(hfile);

		if (read) {
			std::string istr(fdata, sz.LowPart);

			OutputDebugStringA("BEGIN REGEX\r\n");

			colors[0] = sf::Color::White.r;
			colors[1] = sf::Color::White.g;
			colors[2] = sf::Color::White.b;
			colortoprov.emplace(*cptr, 0);

			size_t strt = 0;
			size_t end = 0;
			while (strt != std::string::npos) {
				end = istr.find_first_of("\r\n", strt);
				if (end == std::string::npos) {
					end = istr.size();
				}

				if (std::regex_match(istr.cbegin() + strt, istr.cbegin() + end, m, parseline)) {
					int base = atoi(m[1].str().c_str());
					if (base >= MAX_PROVS) {
						break;
					}

					int prov = pnumremappings[base]; //OutputDebugStringA(m[1].str().c_str()); OutputDebugStringA(":");
					colors[0] = static_cast<char>(atoi(m[2].str().c_str()));// OutputDebugStringA(m[2].str().c_str()); OutputDebugStringA(":");
					colors[1] = static_cast<char>(atoi(m[3].str().c_str())); //OutputDebugStringA(m[3].str().c_str()); OutputDebugStringA(":");
					colors[2] = static_cast<char>(atoi(m[4].str().c_str())); //OutputDebugStringA(m[4].str().c_str()); OutputDebugStringA(":");
					colortoprov.emplace(*cptr, prov);

					if (detail::provinces.size() <= prov)
						detail::provinces.resize(prov + 1);

					detail::provinces[prov].name = sref(m[5].str()); //OutputDebugStringA(m[5].str().c_str()); OutputDebugStringA("\r\n");
				}

				strt = istr.find_first_not_of("\r\n", end);
			}

			OutputDebugStringA("END READ\r\n");
		}
		delete[] fdata;
	}
	else {
		OutputDebugStringA("ERROR CANNOT FIND FILE\r\n");
	}
}

void clrtoarray(_int8 dcolors[8], sf::Color a, sf::Color b){
	if (a.r < b.r || (a.r == b.r && a.g < b.g) || (a.r == b.r && a.g == b.g && a.b < b.b)){
		dcolors[0] = a.r;
		dcolors[1] = a.g;
		dcolors[2] = a.b;
		dcolors[3] = b.r;
		dcolors[4] = b.g;
		dcolors[5] = b.b;
	}
	else {
		dcolors[0] = b.r;
		dcolors[1] = b.g;
		dcolors[2] = b.b;
		dcolors[3] = a.r;
		dcolors[4] = a.g;
		dcolors[5] = a.b;
	}
}


inline int _sign(int x) noexcept {
	if (x > 0) return 1;
	if (x < 0) return -1;
	return 0;
}

void MakeIntersects(v_vector<pair<int, int>> &intersect, sf::IntRect &bounds, const std::vector<std::vector<std::vector<sf::Vector2f>>> &borders) {
	//intersect.resize(bounds.height);
	intersect.index.resize(bounds.height, 0);
	for (const auto &obord : borders) {
		for (const auto &bord : obord) {
			for (unsigned int i = 0; i < bord.size(); i++) {
				if (i != 0) {
					if (bord[i - 1].y > bord[i].y) {
						int ix = static_cast<int>(bord[i].x);
						int iy = static_cast<int>(bord[i].y);
						int tx = static_cast<int>(bord[i - 1].x);
						int ty = static_cast<int>(bord[i - 1].y);

						while (iy < ty) {
							auto sg = _sign(tx - ix);
							intersect.add_to_row(iy - bounds.top, pair<int, int>(ix, sg));
							//InsertPair(intersect[iy - bounds.top], pair<int, int>(ix, sg));
							ix += sg;
							++iy;
						}

					}
				}
				if (i != bord.size() - 1) {
					if (bord[i + 1].y > bord[i].y) {
						int ix = static_cast<int>(bord[i].x);
						int iy = static_cast<int>(bord[i].y);
						int tx = static_cast<int>(bord[i + 1].x);
						int ty = static_cast<int>(bord[i + 1].y);

						while (iy < ty) {
							auto sg = _sign(tx - ix);
							intersect.add_to_row(iy - bounds.top, pair<int, int>(ix, sg));
							//InsertPair(intersect[iy - bounds.top], pair<int, int>(ix, sg));
							ix += sg;
							++iy;
						}
					}
				}
			}
		}
	}
	for (int i = 0; i < bounds.height; ++i) {
		const auto bound = intersect.get_row(static_cast<unsigned int>(i));
		std::sort(bound.first, bound.second, [](std::pair<int, int>& a, std::pair<int, int>& b) {return a.first < b.first; });
	}
}

sf::Color safegetcolor(unsigned char* provs, sf::Vector2u &size, int x, int y, int ch) {
	if (x < 0 || y < 0 || x >= static_cast<int>(size.x) || y >= static_cast<int>(size.y))
		return sf::Color::White;
	//return provs.getPixel(x, y);
	return get_color_from_raw(provs, x, y, size.x, ch);
}

pair<pair<int, int>, pair<int, int>> dpair(int a, int b, int c, int d) {
	return pair<pair<int, int>, pair<int, int>>(pair<int, int>(a, b), pair<int, int>(c, d));
}

void appendpair(flat_map<__int64, std::vector<pair<pair<int, int>, pair<int, int>>>> &segments, __int64 i, const pair<pair<int, int>, pair<int, int>> &p) {
	if (segments.find(i) == segments.cend())
		segments.emplace(i, std::vector<pair<pair<int, int>, pair<int, int>>>());
	segments[i].push_back(p);
}


sf::IntRect bounds(const std::vector<std::vector<std::vector<sf::Vector2f>>> &borders, const size_t start, const size_t end) {
	sf::IntRect ret;
	int minx = 0; int miny = 0; int maxx = 0; int maxy = 0;
	bool first = true;
	for (size_t i = start; i < end; ++i) {
		for (const auto &bvec : borders[i]) {
			for (const auto &k : bvec) {
				if (first) {
					minx = maxx = static_cast<int>(k.x);
					miny = maxy = static_cast<int>(k.y);
					first = false;
				}
				else {
					if (static_cast<int>(k.x) < minx)
						minx = static_cast<int>(k.x);
					if (static_cast<int>(k.x) > maxx)
						maxx = static_cast<int>(k.x);
					if (static_cast<int>(k.y) < miny)
						miny = static_cast<int>(k.y);
					if (static_cast<int>(k.y) > maxy)
						maxy = static_cast<int>(k.y);
				}
			}
		}
	}

	ret.left = minx;
	ret.top = miny;
	ret.width = maxx - minx;
	ret.height = maxy - miny;
	return ret;
}

void MakeBorders(flat_map<__int32, prov_id> &ctp, sf::Vector2u &size, INOUT(flat_multimap<edge, std::vector<sf::Vector2<short>>>) borders, flat_map<__int64, std::vector<pair<pair<int, int>, pair<int, int >> >> &segments, flat_map<edge, std::vector<std::vector<sf::Vector2f>>>& fullborders){
	int pw; int ph; int pch;
	unsigned char* provs = SOIL_load_image("map2\\mapN2.png", &pw, &ph, &pch, SOIL_LOAD_AUTO);

	if (!provs) {
		OutputDebugStringA("IMG ERR: ");
		OutputDebugStringA(SOIL_last_result());
		OutputDebugStringA("\r\n");
		return;
	}

	int tw; int th; int tch;
	unsigned char* terrain = SOIL_load_image("map2\\mapT.png", &tw, &th, &tch, SOIL_LOAD_AUTO);
	
	//sf::Image provs;
	//load_soil_image(provs, "map2\\provinces.bmp");
	//load_soil_image(provs, "map2\\mapN2.bmp");
	//provs.loadFromFile("map\\provinces.bmp");
	//size = provs.getSize();
	//sf::Image terrain;
	//load_soil_image(terrain, "map2\\terrain.bmp");
	//load_soil_image(terrain, "map2\\mapT.bmp");
	//terrain.loadFromFile("map\\terrain.bmp");

	size.x = pw;
	size.y = ph;

	OutputDebugStringA((std::string("IMG SIZE: ") + std::to_string(size.x) + "," + std::to_string(size.y) + "\r\n").c_str());

	OutputDebugStringA("BEGIN TERRAIN\r\n");
	LoadTerrain(ctp, provs, pw, ph, pch, terrain, tw, tch);

	_int8 colors[4] = { 0, 0, 0, 0 };
	__int32 *cptr = (__int32*)colors;

	_int8 dcolors[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	__int64 *dcptr = (__int64*)dcolors;

	//unordered_map<__int64, vector<pair<pair<int, int>, pair<int, int>>>> segments;
	sf::Color a;
	sf::Color b;
	sf::Color c;
	sf::Color d;

	/*char buffer[65];
	_itoa_s(size.x, buffer, 64, 10);
	OutputDebugStringA(buffer);
	OutputDebugStringA(",");

	_itoa_s(size.y, buffer, 64, 10);
	OutputDebugStringA(buffer);*/
	OutputDebugStringA("BEGIN BORDERS\r\n");


	for (int j = -1; j < (int)size.y; j++){
		for (int i = -1; i < (int)size.x; i++){
			a = safegetcolor(provs, size, i, j, pch);
			b = safegetcolor(provs, size, i + 1, j, pch);
			c = safegetcolor(provs, size, i, j + 1, pch);
			d = safegetcolor(provs, size, i + 1, j + 1, pch);
			if (a == b && a == c && a == d) { // same, no connections

			}
			else if (a == b && a == c)  { //two colors
				clrtoarray(dcolors, a, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 3, i * 2 + 3, j * 2 + 2));
			}
			else if (c == b && c == d) {
				clrtoarray(dcolors, a, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 1));
			}
			else if (c == a && a == d) {
				clrtoarray(dcolors, b, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 3, j * 2 + 2, i * 2 + 2, j * 2 + 1));
			}
			else if (b == a && b == d) {
				clrtoarray(dcolors, b, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 3));
			}
			else if (c == a && b == d) {
				clrtoarray(dcolors, b, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 1, i * 2 + 2, j * 2 + 3));
			}
			else if (b == a && c == d) {
				clrtoarray(dcolors, b, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 3, j * 2 + 2));
			}
			else if (a == d && b == c) {
				colors[0] = a.r;
				colors[1] = a.g;
				colors[2] = a.b;
				auto it = ctp.find(*cptr);
				if (it != ctp.cend() && P_IS_WATER(detail::provinces[(it->second)].pflags)) {
					clrtoarray(dcolors, a, b);
					appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 3));
					appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 1, i * 2 + 3, j * 2 + 2));
				}
				else {
					clrtoarray(dcolors, a, b);
					appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 1, i * 2 + 1, j * 2 + 2));
					appendpair(segments, *dcptr, dpair(i * 2 + 3, j * 2 + 2, i * 2 + 2, j * 2 + 3));
				}
			}
			else if (b == a) { // three colors
				clrtoarray(dcolors, a, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, d, b);
				appendpair(segments, *dcptr, dpair(i * 2 + 3, j * 2 + 2, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, c, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 3, i * 2 + 2, j * 2 + 2));
			}
			else if (b == d) {
				clrtoarray(dcolors, a, b);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 1, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, c, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 3, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, a, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 2));
			}
			else if (c == d) {
				clrtoarray(dcolors, a, b);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 1, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, a, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, b, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 3, j * 2 + 2, i * 2 + 2, j * 2 + 2));
			}
			else if (a == c) {
				clrtoarray(dcolors, a, b);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 1, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, c, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 3, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, b, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 3, j * 2 + 2, i * 2 + 2, j * 2 + 2));
			}
			else if (a == d) {
				clrtoarray(dcolors, a, b);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 1, i * 2 + 3, j * 2 + 2));
				clrtoarray(dcolors, a, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 3));
			}
			else if (c == b) {
				clrtoarray(dcolors, c, a);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 1));
				clrtoarray(dcolors, c, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 3, i * 2 + 3, j * 2 + 2));
			}
			else { //four colors
				clrtoarray(dcolors, a, b);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 1, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, c, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 2, j * 2 + 3, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, a, c);
				appendpair(segments, *dcptr, dpair(i * 2 + 1, j * 2 + 2, i * 2 + 2, j * 2 + 2));
				clrtoarray(dcolors, b, d);
				appendpair(segments, *dcptr, dpair(i * 2 + 3, j * 2 + 2, i * 2 + 2, j * 2 + 2));
			}
		}
	}

	OutputDebugStringA("#SEGMENTS: ");
	char buffer[65];
	_itoa_s(static_cast<int>(segments.size()), buffer, 64, 10);
	OutputDebugStringA(buffer);
	OutputDebugStringA("\r\n");

	OutputDebugStringA("COMBINING SEGMENTS\r\n");

	for (auto t : segments) {
		prov_id loca = 0;
		prov_id locb = 0;

		*dcptr = t.first;
		colors[0] = dcolors[0]; colors[1] = dcolors[1]; colors[2] = dcolors[2];

		{
			auto it = ctp.find(*cptr);
			if (it != ctp.cend()) {
				loca = it->second;
			}

			colors[0] = dcolors[3]; colors[1] = dcolors[4]; colors[2] = dcolors[5];
			it = ctp.find(*cptr);
			if (it != ctp.cend()) {
				locb = it->second;
			}
		}

		OutputDebugStringA("S");

		unsigned int place = 0;
		while (place < t.second.size()) {
			std::vector<sf::Vector2<short>> bo;

			int firstx = t.second[place].first.first;
			int firsty = t.second[place].first.second;

			int currentx = t.second[place].second.first;
			int currenty = t.second[place].second.second;

			std::list<sf::Vector2<short>> tempdq;
			tempdq.push_back(sf::Vector2<short>(static_cast<short>(firstx), static_cast<short>(firsty)));
			tempdq.push_back(sf::Vector2<short>(static_cast<short>(currentx), static_cast<short>(currenty)));

			t.second[place].first.first = -10;
			t.second[place].second.first = -10;
			for (unsigned int inx = place + 1; inx < t.second.size(); inx++) {
				if (t.second[inx].first.first == firstx && t.second[inx].first.second == firsty) {
					firstx = t.second[inx].second.first;
					firsty = t.second[inx].second.second;
					tempdq.push_front(sf::Vector2<short>(static_cast<short>(firstx), static_cast<short>(firsty)));
					t.second[inx].first.first = -10;
					t.second[inx].second.first = -10;
					inx = place;
				}
				else if (t.second[inx].second.first == firstx && t.second[inx].second.second == firsty) {
					firstx = t.second[inx].first.first;
					firsty = t.second[inx].first.second;
					tempdq.push_front(sf::Vector2<short>(static_cast<short>(firstx), static_cast<short>(firsty)));
					t.second[inx].first.first = -10;
					t.second[inx].second.first = -10;
					inx = place;
				}

				if (t.second[inx].first.first == currentx && t.second[inx].first.second == currenty) {
					currentx = t.second[inx].second.first;
					currenty = t.second[inx].second.second;
					tempdq.push_back(sf::Vector2<short>(static_cast<short>(currentx), static_cast<short>(currenty)));
					t.second[inx].first.first = -10;
					t.second[inx].second.first = -10;
					inx = place;
				}
				else if (t.second[inx].second.first == currentx && t.second[inx].second.second == currenty) {
					currentx = t.second[inx].first.first;
					currenty = t.second[inx].first.second;
					tempdq.push_back(sf::Vector2<short>(static_cast<short>(currentx), static_cast<short>(currenty)));
					t.second[inx].first.first = -10;
					t.second[inx].second.first = -10;
					inx = place;
				}
			}

			while (place < t.second.size() && t.second[place].first.first == -10) {
				place++;
			}

			if (tempdq.size() > 0) {
				auto it = tempdq.begin();
				auto pptr = tempdq.begin();

				short px = it->x;
				short py = it->y;

				++it;
				size_t count = 0;

				
				bo.emplace_back(px, py);

				while (it != tempdq.end()) {
					px = bo.back().x;
					py = bo.back().y;

					if (px != it->x || py != it->y) {
						const auto pval = fast_math::atan2(py - pptr->y, px - pptr->x);
						const auto nval = fast_math::atan2(it->y - pptr->y, it->x - pptr->x);
						if (nval < pval - 0.00001f || nval > pval + 0.00001f) {
							pptr = it;
							//b.borderline.emplace_back(px, py);
							count = 0;
						} 
						bo.emplace_back(it->x, it->y);
						
					}
					
					if (count > 20) {
						pptr = it;
						//b.borderline.emplace_back(px, py);
						count = 0;
					}
					//px = it->x;
					//py = it->y;
					++count;
					++it;
				}

				//b.borderline.emplace_back(tempdq.back());
			}

			fullborders[edge(loca, locb)].emplace_back(tempdq.cbegin(), tempdq.cend());

			emplace_vv_unique(global::province_connections, loca, locb);
			emplace_vv_unique(global::province_connections, locb, loca);

			
			borders.emplace(edge(loca, locb), std::move(bo));
		}
	}

	OutputDebugStringA("\r\n#BORDERS: ");
	_itoa_s(static_cast<int>(borders.size()), buffer, 64, 10);
	OutputDebugStringA(buffer);
	OutputDebugStringA("\r\n");

	OutputDebugStringA("END BORDERS\r\n");

	SOIL_free_image_data(provs);
	SOIL_free_image_data(terrain);
}


bool file_exists(const wchar_t* szPath) noexcept {
	DWORD dwAttrib = GetFileAttributes(szPath);
	return dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

bool file_exists(const char* szPath) noexcept {
	DWORD dwAttrib = GetFileAttributesA(szPath);
	return dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

int loadOrSaveDb(sqlite3 *pInMemory, const char *zFilename, bool isSave){
	int rc;                   /* Function return code */
	sqlite3 *pFile;           /* Database connection opened on zFilename */
	sqlite3_backup *pBackup;  /* Backup object used to copy data */
	sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
	sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */

	rc = sqlite3_open(zFilename, &pFile);
	if (rc == SQLITE_OK){

		pFrom = (isSave ? pInMemory : pFile);
		pTo = (isSave ? pFile : pInMemory);

		pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
		if (pBackup){
			(void)sqlite3_backup_step(pBackup, -1);
			(void)sqlite3_backup_finish(pBackup);
		}
		rc = sqlite3_errcode(pTo);
	}

	(void)sqlite3_close(pFile);
	return rc;
}

int startdate(smatch &m){
	return (372 * (stol(m[1].str(), NULL, 10)) + (stol(m[2].str(), NULL, 10) - 1) * 12 + (stol(m[3].str(), NULL, 10) - 1));
}

int startdate(cmatch &m){
	return (372 * (stol(m[1].str(), NULL, 10)) + (stol(m[2].str(), NULL, 10) - 1) * 12 + (stol(m[3].str(), NULL, 10) - 1));
}

flat_map<__int64, char_id> char_id_remapping;
flat_set<__int64> bad_ids = {33250, 160217, 166715, 166718, 166754, 195075, 200238, 200240};

char_id get_rm_char_id(__int64 charid) {
	if (charid == 0 || bad_ids.count(charid) != 0)
		return 0;
	if (char_id_remapping.count(charid) == 0) {
		char_id_remapping[charid] = static_cast<char_id>(detail::people.size());
		detail::people.emplace_back();
	}
	return char_id_remapping[charid];
}

struct other_t_vals {
	cul_id c_id = culture::NONE;
	rel_id r_id = religion::NONE;
	prov_id capital =  province::NONE;
	title_id t_leige = title::NONE;
};


void bindHolderPvals(title_id t_id, const std::unique_ptr<prse_list> &results, INOUT(flat_map<title_id, other_t_vals>) other_v, IN(w_lock) l) {
	std::regex parseyear("^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$");
	std::cmatch m;


	int holder = 0;

	for (const auto& val : results->list) {
		//OutputDebugStringA("h");
		if (val.list) {

			if (std::regex_match(val.list->value.c_str(), m, parseyear)) {
				if (startdate(m) <= START_DATE) {
					for (const auto &v2 : val.list->list) {
						if (v2.assoc) {

							if (v2.assoc->left == "liege" /*|| vinner.first == "de_jure_liege"*/) {

								if (v2.assoc->right == "0" || v2.assoc->right == "x") {
									other_v[t_id].t_leige = 0;
								} else {
									title_id lg = tnumremappings[v2.assoc->right];// getRowID(getrid);


									if (lg != 0) {
										other_v[t_id].t_leige = lg;
										//sqlite3_bind_int64(tinsert, 2, lg);
									} else {
										other_v[t_id].t_leige = 0;
										//sqlite3_bind_null(tinsert, 2);
									}
								}
							}
							else if (v2.assoc->left == "holder") {
								if (v2.assoc->right == "x") {
									detail::titles[t_id].holder = 0;
								} 
								else {
									detail::titles[t_id].holder = get_rm_char_id(stoi(v2.assoc->right));
								}
							}
						}
					}
				}
			}
		}
	}

	if (valid_ids(detail::titles[t_id].holder)) {
		global::holdertotitle.insert(detail::titles[t_id].holder, title_id_t(t_id), l);
	}

	//if (county && holder == 0)
		//sqlite3_bind_int(tinsert, 2, rand()%10000);
}

struct extra_ch_vals {
	cul_id c = culture::NONE;
	rel_id r = religion::NONE;
};

void bindCharPvals(INOUT(person) p, INOUT(extra_ch_vals) ev, INOUT(__int64) father, INOUT(__int64) mother, INOUT(__int64) spouse, INOUT(unsigned int) spousedate, INOUT(std::string) name, const std::unique_ptr<prse_list> &results, __int64 charid, unsigned int dte = 0) {
	std::regex parseyear("^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$");
	std::cmatch m;

	for (const auto& val : results->list) {
		//OutputDebugStringA("h");
		if (val.assoc) {
			//pair<string, string> v = boost::any_cast<pair<string, string>>(val);
			if (val.assoc->left == "culture") {
				ev.c = str_to_cid(val.assoc->right);
			}
			else if (val.assoc->left == "religion") {
				ev.r = str_to_rid(val.assoc->right);
			}
			else if (val.assoc->left == "name") {
				name = val.assoc->right;
			}
			else if (val.assoc->left == "dynasty") {
				p.dynasty = dnumremappings[stol(val.assoc->right, NULL, 10)];
			}
			else if (val.assoc->left == "father") {
				father = stol(val.assoc->right, NULL, 10);
			}
			else if (val.assoc->left == "mother") {
				mother = stol(val.assoc->right, NULL, 10);
			}
			else if (val.assoc->left == "female") {
				p.gender = person::FEMALE;
			}
			else if (val.assoc->left == "birth") {
				p.born = dte;
			}
			else if (val.assoc->left == "death") {
				p.died = dte;
			}
			else if (val.assoc->left == "add_spouse") {
				if (dte > spousedate && stol(val.assoc->right, NULL, 10) != 0) {
					spouse = stol(val.assoc->right, NULL, 10);
					spousedate = dte;
				}

				// spouses.push_back(pair<int, int>(charid, stol(val.assoc->right, NULL, 10)));
			}
		}
		else if (val.list) {
			//pair<string, vector<boost::any>> v = boost::any_cast<pair<string, vector<boost::any>>>(val);
			if (std::regex_match(val.list->value.c_str(), m, parseyear)) {
				if (startdate(m) <= START_DATE) {
					date d(static_cast<unsigned short>(stol(m[1].str()) + 1400),
						static_cast<unsigned short>(stol(m[2].str())),
						static_cast<unsigned short>(stol(m[3].str())));
					days dval = d - date(1400, Jan, 1);
					bindCharPvals(p, ev, father, mother, spouse, spousedate, name, val.list, charid, dval.days());
				}
			}
		}
	}
}

void bindPvals(INOUT(province) p, const std::unique_ptr<prse_list> &results, bool &hastitle) {

	std::regex parseyear("^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$$");
	std::cmatch m;


	for (const auto &val : results->list) {
		//OutputDebugStringA("h");
		if (val.assoc) {
			

			if (val.assoc->left == "culture") {
				p.culture = str_to_cid(val.assoc->right);
			}
			else if (val.assoc->left == "religion") {
				p.religion = str_to_rid(val.assoc->right);
			}
			else if (val.assoc->left == "title") {
				sqlite_int64 tid = tnumremappings[val.assoc->right];

				if (tid != 0) {
					hastitle = true;
					// sqlite3_bind_int64(tinsert, 1, tid);
				}
			}
		}
		else if (val.list) {
			//pair<string, vector<boost::any>> v = boost::any_cast<pair<string, vector<boost::any>>>(val);
			if (std::regex_match(val.list->value.c_str(), m, parseyear)) {
				if (startdate(m) <= START_DATE) {
					bindPvals(p, val.list, hastitle);
				}
			}
		}
	}
}

float genTax(province &l, prov_id provn) {
	const static float factors[] = {0.0f, 1.0f, 0.8f, 0.1f, 0.05f, 0.0f, 0.3f, 0.7f, 1.2f, 0.0f, 0.7f, 0.15f, 0.15f, 0.8f};
	float ttax = 0.1f;
	float totalpix = 0.0f;
	for (int i = 0; i < 14; i++) {
		totalpix += l.terrain[i];
	}
	if (totalpix > 0) {
		for (int i = 0; i < 14; i++) {
			ttax += factors[i] * l.terrain[i] / totalpix;
		}
	} else {
		ttax += 0.1f;
	}
	if (l.climate == 0) ttax *= .9f;
	if (l.climate == 1) ttax *= .7f;
	if (l.climate == 2) ttax *= .4f;

	int wcon = 0;
	for (auto pr = global::province_connections.get_row(provn); pr.first != pr.second; ++pr.first) {
		if (P_IS_WATER(detail::provinces[*pr.first].pflags))
			wcon++;
	}

	if (wcon > 0) {
		ttax *= 1.3f;
		ttax += 0.1f;
	}
	return ttax;
}

void number_provinces() {
	std::wregex parsefname(L"^.*?([0-9]+) - ([^.]*)\\.txt$");
	std::wcmatch m;

	//flat_map<prov_id, bool> padded;

	int lastp = 0;
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(L"history\\provinces\\*.txt", &data);
	int newid = 0;
	pnumremappings.emplace(0, 0);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (std::regex_match(data.cFileName, m, parsefname)) {
				int prov = _wtoi(m[1].str().c_str());

				std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
				ParseCKFile((TCHAR*)(std::wstring(L"history\\provinces\\") + data.cFileName).c_str(), results, NULL);

				for (const auto &val : results->list) {
					if (val.assoc) {
						if (val.assoc->left == "title") {
							const int mp = tnumremappings[val.assoc->right];
							newid = std::max(newid, mp);
							pnumremappings.emplace(prov, mp);
							if (detail::provinces.size() <= mp)
								detail::provinces.resize(mp+1);
							detail::provinces[mp].pflags = (PROV_FLAG_HASTITLE);
						}
					}
				}
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}

	for (int i = 1; i < MAX_PROVS; ++i) {
		if (pnumremappings.count(i) == 0) {
			// global::provinces[newid + 1].pflags = 0;
			pnumremappings.emplace(i, ++newid);
		}
	}
}

void count_provinces() {
	std::wregex parsefname(L"^.*?([0-9]+) - ([^.]*)\\.txt$");
	std::wcmatch m;

	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(L"history\\provinces\\*.txt", &data);

	first_non_county = 1;
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (std::regex_match(data.cFileName, m, parsefname)) {
				int prov = _wtoi(m[1].str().c_str());
				++first_non_county;
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}
}

void number_titles() {
	WIN32_FIND_DATA data;
	std::wregex parsefname(L"([^.]*)\\.txt$");
	std::wcmatch m;

	HANDLE hFind = FindFirstFile(L"history\\titles\\*.txt", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			//OutputDebugString(data.cFileName); OutputDebugStringA("\r\n");
			if (std::regex_match(data.cFileName, m, parsefname)) {
				std::string name = wstr_to_str(m[1].str());

				if (name[0] != 'b') {
					if (name[0] == 'c') {
						tnumremappings[name] = static_cast<title_id>(first_county++);
					} else {
						tnumremappings[name] = static_cast<title_id>(first_non_county++);
					}
				}
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}
}

void BuildProvDB() {


	OutputDebugStringA("MAKING PROV DB\r\n");


	HANDLE hFind;
	WIN32_FIND_DATA data;

	std::wregex parsefname(L"^.*?([0-9]+) - ([^.]*)\\.txt$");
	std::wcmatch m;



	hFind = FindFirstFile(L"history\\provinces\\*.txt", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			//wprintf(L"%s\n", data.cFileName);
			if (std::regex_match(data.cFileName, m, parsefname)) {
				prov_id prov = pnumremappings[_wtoi(m[1].str().c_str())];
				detail::provinces[prov].name = sref(wstr_to_str(m[2].str()));

				std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
				ParseCKFile((TCHAR*)(std::wstring(L"history\\provinces\\") + data.cFileName).c_str(), results, NULL);

				bool hastitle = false;
				bindPvals(detail::provinces[prov], results, hastitle);

				detail::provinces[prov].tax = genTax(detail::provinces[prov], prov);

				

			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}


}

bool dateexclude(std::string &str) {
	std::regex parseyear("^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$");
	std::smatch m;

	if (std::regex_match(str, m, parseyear)) {
		if (startdate(m) <= START_DATE) {
			return false;
		}
		else {
			return true;
		}
	}
	return false;
}

bool titleexclude(std::string &str) {
	if (str.compare("allow") == 0)
		return true;
	if (str.size() >= 2 && str[0] == 'b' && str[1] == '_')
		return true;
	return false;
}



//vector<province> provinces(1438);
//vector<border> borders;

void BuildDynasties() {
	detail::dynasties.clear();
	OutputDebugStringA("MAKING Dynasties DB\r\n");


	detail::dynasties.emplace_back();
	INOUT(auto) lb = detail::dynasties.back();
	lb.name = sref("lowborn");
	lb.culture = culture::NONE;


	//vector<boost::any> results;
	dyn_id nextd = 1;

	std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
	ParseCKFile(L"common\\dynasties\\00_dynasties.txt", results);
	for (const auto &in : results->list) {
		if (in.list) {
			//pair<string, vector<boost::any>> v = boost::any_cast<pair<string, vector<boost::any>>>(in);
			int di = stol(in.list->value);
			dnumremappings[di] = nextd;

			detail::dynasties.emplace_back();
			INOUT(auto) d = detail::dynasties.back();
			d.culture = culture::NONE;

			++nextd;

			for (const auto &inn : in.list->list) {
				if (inn.assoc) {
					//pair<string, string> v2 = boost::any_cast<pair<string, string>>(inn);
					if (inn.assoc->left == "name") {
						d.name = sref(inn.assoc->right);
					}
					if (inn.assoc->left == "culture"){
						d.culture = str_to_cid(inn.assoc->right);
					}
				}
			}

			
		}
	}

}



void BuildReligionDB() {
	detail::religions.resize(1);

	std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
	ParseCKFile((TCHAR*)std::wstring(L"common\\religions\\00_religions.txt").c_str(), results, nullptr);

	for (const auto& val : results->list) {
		if (val.list) {
			sref valtxt(val.list->value);

			for (const auto &itm : val.list->list) {
				if (itm.list && itm.list->value.compare("male_names") !=0 && itm.list->value.compare("female_names") != 0) {

					detail::religions.emplace_back();
					INOUT(auto) r = detail::religions.back();

					r.group = valtxt;
					r.name = sref(itm.list->value);
				}
			}
		}
	}
}

void BuildCultureDB() {
	detail::cultures.resize(1);

	std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
	ParseCKFile((TCHAR*)std::wstring(L"common\\cultures\\00_cultures.txt").c_str(), results, nullptr);

	for (const auto& val : results->list) {
		if (val.list) {
			sref valname(val.list->value);

			for (const auto &itm : val.list->list) {
				if (itm.list) {
					detail::cultures.emplace_back();

					INOUT(auto) c = detail::cultures.back();
					c.group = valname;
					c.name = sref(itm.list->value);
					c.prefix = sref("of");
					

					for (const auto &subitm : itm.list->list) {
						if (subitm.assoc && subitm.assoc->left.compare("from_dynasty_prefix") == 0) {
							c.prefix = sref(subitm.assoc->right);
						} else if (subitm.list && subitm.list->value.compare("male_names") == 0) {
							for (const auto& names : subitm.list->list) {
								if (names.value) {
									c.mnames.emplace_back(names.value->value.substr(0, names.value->value.find('_')));
								}
							}
						} else if (subitm.list && subitm.list->value.compare("female_names") == 0) {
							for (const auto& names : subitm.list->list) {
								if (names.value) {
									c.fnames.emplace_back(names.value->value.substr(0, names.value->value.find('_')));
								}
							}
						}
					}

					
				}
			}
		}
	}

}

void BuildCharDB() {
	OutputDebugStringA("MAKING CHAR DB\r\n");

	HANDLE hFind;
	WIN32_FIND_DATA data;

	hFind = FindFirstFile(L"history\\characters\\*.txt", &data);
	if (hFind != INVALID_HANDLE_VALUE) {

		do {

			OutputDebugString(data.cFileName); OutputDebugStringA("\r\n");

			std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
			ParseCKFile((TCHAR*)(std::wstring(L"history\\characters\\") + data.cFileName).c_str(), results, dateexclude);

			for (const auto& val : results->list) {
				if (val.list) {

					//get_rm_char_id(__int64 charid)
					__int64 father = 0;
					__int64 mother = 0;
					__int64 spouse = 0;
					unsigned int spousedate = 0;
					person p;
					p.born = max_value<decltype(p.born)>::value;

					const auto raw_id = stol(val.list->value, NULL, 10);
					std::string name("None");

					extra_ch_vals ev;
					bindCharPvals(p, ev, father, mother, spouse, spousedate, name, val.list, raw_id, 0);

					if (p.born <= global::currentday) {

						const auto this_id = get_rm_char_id(raw_id);

						p.name = sref(name);

						p.mother = get_rm_char_id(mother);
						p.father = get_rm_char_id(father);
						p.spouse = get_rm_char_id(spouse);
						p.primetitle = detail::people[this_id].primetitle;

						detail::people[this_id] = p;
						detail::people[p.spouse.value].spouse = this_id;

						if (p.died == 0) {
							untitled_data* newdata = nullptr;
							create_untitled(char_id_t(this_id), newdata, w_lock());

							newdata->culture = ev.c;
							newdata->religion = ev.r;

							auto statvals = global_store.get_int();

							newdata->stats.analytic = statvals & 0x7;
							statvals = statvals >> 3i64;
							newdata->stats.martial = statvals & 0x7;
							statvals = statvals >> 3i64;
							newdata->stats.social = statvals & 0x7;
							statvals = statvals >> 3i64;
							newdata->stats.intrigue = statvals & 0x7;
							statvals = statvals >> 3i64;

							newdata->attrib.load(statvals);

						}
					}
				}
			}

		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}
}


sf::Color combineclrsB(const std::unique_ptr<prse_list> &vec) {
	sf::Color result;
	result.r = static_cast<unsigned char>(stol(vec->list[0].value->value, NULL, 10));
	result.g = static_cast<unsigned char>(stol(vec->list[1].value->value, NULL, 10));
	result.b = static_cast<unsigned char>(stol(vec->list[2].value->value, NULL, 10));
	return result;
}

sqlite3_int64 dotinsert(std::string name, int type, const std::unique_ptr<prse_list> &contents, INOUT(flat_map<title_id, other_t_vals>) other_v) {
	if (tnumremappings.count(name) == 0)
		tnumremappings.emplace(name, tnumremappings.size() + 1);

	const auto t_val = tnumremappings[name];
	if (detail::titles.size() <= t_val) {
		detail::titles.resize(t_val+1);
	}

	INOUT(auto) t = detail::titles[t_val];

	t.type = static_cast<unsigned char>(type);
	t.rname = sref(convertName(name));
	

	//auto tinsert_v = std::string("INSERT INTO titles (id, type, color1, color2, rname, admin) VALUES(?1, ?2, ?3, ?4, ?5, ") + std::to_string(administration::NONE) + ")";

	for (const auto& val : contents->list) {
		if (val.assoc) {
			//pair<string, string> v = boost::any_cast<pair<string, string>>(val);
			if (val.assoc->left == "culture") {
				other_v[t_val].c_id = str_to_cid(val.assoc->right);
			} else if (val.assoc->left == "religion") {
				other_v[t_val].r_id = str_to_rid(val.assoc->right);
			} else if (val.assoc->left == "capital") {
				other_v[t_val].capital = static_cast<prov_id>(stol(val.assoc->right.c_str()));
			}
		} else if (val.list) {
			if (val.list->value == "color") {
				t.color1 = combineclrsB(val.list);
			} else if (val.list->value == "color2") {
				sf::Color lt = combineclrsB(val.list);
				//sqlite3_bind_int64(tinsert, 4, t.toInteger() >> 8);
			}
		}
	}

	if (other_v.count(t_val) == 0)
		other_v[t_val].t_leige = title::NONE;

	return t_val;
}

void insertrel(title_id sup, title_id inf, IN(w_lock) l) {
	if (sup != inf) {
		date d = date_offset + days(global::currentday);
		const unsigned short y = d.year() - date_offset.year();

		global::add_dj(djrecord{prov_id_t(inf), title_id_t(sup), y}, l);
	}
}


void add_titles(std::vector<sqlite3_int64> &ident, INOUT(flat_map<title_id, other_t_vals>) other_v, const std::unique_ptr<prse_list> &contents, IN(w_lock) l) {
	for (const auto &val : contents->list) {
		if (val.list) {
			if (val.list->value.size() > 2 && val.list->value[0] == 'e' && val.list->value[1] == '_') {
				sqlite3_int64 row = dotinsert( val.list->value, EMPIRE_TYPE, val.list, other_v);
				ident.push_back(row);
				add_titles(ident, other_v, val.list, l);
				ident.pop_back();
			} else if (val.list->value.size() > 2 && val.list->value[0] == 'k' && val.list->value[1] == '_') {
				sqlite3_int64 row = dotinsert( val.list->value, KINGDOM_TYPE, val.list, other_v);
				ident.push_back(row);
				add_titles(ident, other_v, val.list, l);
				ident.pop_back();
			} else if (val.list->value.size() > 2 && val.list->value[0] == 'd' && val.list->value[1] == '_') {
				sqlite3_int64 row = dotinsert( val.list->value, DUTCHY_TYPE, val.list, other_v);
				ident.push_back(row);
				add_titles(ident, other_v, val.list, l);
				ident.pop_back();
			} else if (val.list->value.size() > 2 && val.list->value[0] == 'c' && val.list->value[1] == '_') {
				sqlite3_int64 row = dotinsert( val.list->value, COUNTY_TYPE, val.list, other_v);
				for (const auto ival : ident) {
					insertrel(static_cast<title_id>(ival), static_cast<title_id>(row), l);
				}
				//insertrel(row, row);
			}
		}
	}
}

void BuildTitlesDB(INOUT(flat_map<title_id, other_t_vals>) other_v, IN(w_lock) l) {
	std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
	ParseCKFile(L"common\\landed_titles\\landed_titles.txt", results, titleexclude);
	detail::titles.resize(1);

	std::vector<sqlite3_int64> idents;
	add_titles(idents, other_v, results, l);
}

void UpdateTextData() {

	HANDLE hFind;
	WIN32_FIND_DATA data;

	std::wregex parsefname(L"([^.]*)\\.txt$");
	std::wcmatch m;



	hFind = FindFirstFile(L"localisation\\*.csv", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			OutputDebugString(data.cFileName); OutputDebugStringA("\r\n");
			//open: data.cFileName
			
			HANDLE hfile = CreateFile((std::wstring(L"localisation\\") + data.cFileName).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
			if (hfile != INVALID_HANDLE_VALUE) {
				DWORD bytes;

				LARGE_INTEGER sz;
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

				if (read) {
					std::string istr(fdata, sz.LowPart);
					
					size_t strt = 0;
					size_t end = 0;
					while (strt != std::string::npos) {
						end = istr.find_first_of(";", strt);
						std::string val = istr.substr(strt, end - strt);
						if (val.length() >= 3 && val[1] == '_') {
							if (val[0] == 'c' || val[0] == 'd' || val[0] == 'k' || val[0] == 'e') {
								if (val[val.length() - 1] == 'j' && val[val.length() - 2] == 'd' && val[val.length() - 3] == 'a' && val[val.length() - 4] == '_') {
									strt = end + 1;
									end = istr.find_first_of(";", strt);
									
									detail::titles[tnumremappings[val.substr(0, val.length() - 4)]].adj = sref(istr.substr(strt, end - strt));

								} else {
									strt = end + 1;
									end = istr.find_first_of(";", strt);
									detail::titles[tnumremappings[val]].rname = sref(istr.substr(strt, end - strt));
								}
							}
						}
						if (val.length() >= 5) {
							if (val[0] == 'P' && val[1] == 'R' && val[2] == 'O' && val[3] == 'V') {
								int p = strtol(val.substr(4, val.length() - 4).c_str(), NULL, 10);
								if (p != 0) {
									strt = end + 1;
									end = istr.find_first_of(";", strt);
									detail::provinces[pnumremappings[p]].name = sref(istr.substr(strt, end - strt));
								}
							}
						}

						if (end == std::string::npos) {
							end = istr.size();
						}
						end = istr.find_first_of("\r\n", strt);
						strt = istr.find_first_not_of("\r\n", end);
					}

					OutputDebugStringA("END READ\r\n");
				}
				delete[] fdata;
			}

		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}

}

void BuildHoldersTable(INOUT(flat_map<title_id, other_t_vals>) other_v, IN(w_lock) l) {
	
	OutputDebugStringA("MAKING HOLDERS VALUES\r\n");


	HANDLE hFind;
	WIN32_FIND_DATA data;

	std::wregex parsefname(L"([^.]*)\\.txt$");
	std::wcmatch m;


	hFind = FindFirstFile(L"history\\titles\\*.txt", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			//OutputDebugString(data.cFileName); OutputDebugStringA("\r\n");
			if (std::regex_match(data.cFileName, m, parsefname)) {
				std::string name = wstr_to_str(m[1].str());

				if (name[0] != 'b') {

					const auto t_val = tnumremappings[name];
						

					std::unique_ptr<prse_list> results = std::make_unique<prse_list>();
					ParseCKFile((TCHAR*)(std::wstring(L"history\\titles\\") + data.cFileName).c_str(), results, nullptr);

					bindHolderPvals(t_val, results, other_v, l);

				}
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}

	for (INOUT(auto) pr : other_v) {
		if (pr.second.t_leige != title::NONE) {
			if (!valid_ids(detail::titles[pr.second.t_leige].holder)) {
				pr.second.t_leige = title::NONE;
			}
		}
	}


	OutputDebugStringA("MAKING PRIME TITLES\r\n");

	for (title_id t = 1; t != detail::titles.size(); ++t) {
		const auto c = detail::titles[t].holder;
		if (valid_ids(c)) {
			if (!valid_ids(detail::people[c.value].primetitle) || detail::titles[detail::people[c.value].primetitle.value].type > detail::titles[t].type) {
				detail::people[c.value].primetitle = t;
				//std::string message = std::to_string(t) + " -> " + std::to_string(c) + " pt: " + std::to_string(global::people[c].primetitle) + "\r\n";
				//OutputDebugStringA(message.c_str());
			}
		}
	}

}

sqlite3_int64 combineclrs(const std::unique_ptr<prse_list > &vec) {
	sqlite3_int64 result = 0;
	for (unsigned int i = 0; i < vec->list.size(); i++) {
		if (vec->list[i].value) {
			int value = stoi(vec->list[i].value->value);
			result = (result << 8) | (value & 0xFF);
		}
	}
	return result;
}


void generate_data(const char* szPath) {
	days dtemp = date(1400 + 867, Jan, 1) - date_offset;
	global::currentday = dtemp.days();

	count_provinces();
	number_titles();

	BuildCultureDB();
	BuildReligionDB();

	flat_map<title_id, other_t_vals> other_title_info;

	detail::people.resize(1);
	detail::people[0].name = sref("None");

	BuildDynasties();

	{
		w_lock l;
		BuildTitlesDB(other_title_info, l);
		BuildHoldersTable(other_title_info, l);
	}

	BuildCharDB();

	OutputDebugStringA("bad character IDs:\r\n");
	for (IN(auto) p : char_id_remapping) {
		if (detail::people[p.second].name == sref("")) {
			std::string num = std::to_string(p.first) + "\r\n";
			OutputDebugStringA(num.c_str());
		}
	}
	OutputDebugStringA("END bad IDs\r\n");

	number_provinces();

	{
		w_lock l;

		const std::function<void(title_id)> add_admin_for_t = [&l, &other_title_info, &add_admin_for_t](title_id t_id) {
			if (other_title_info.count(t_id) != 0) {
				if (valid_ids(detail::titles[t_id].associated_admin))
					return;

				admin_id_t new_id = new_admin_id(l);
				detail::titles[t_id].associated_admin = new_id;

				if (other_title_info[t_id].t_leige != title::NONE && !valid_ids(detail::titles[other_title_info[t_id].t_leige].associated_admin)) {
					add_admin_for_t(other_title_info[t_id].t_leige);
				}

				get_object(new_id, l).random_administration(title_id_t(t_id), prov_id_t(other_title_info[t_id].capital),
					cul_id_t(other_title_info[t_id].c_id), rel_id_t(other_title_info[t_id].r_id),
					other_title_info[t_id].t_leige != title::NONE ? detail::titles[other_title_info[t_id].t_leige].associated_admin : admin_id_t(), l);
				
				//TODO: make regents
				get_object(new_id, l).executive = detail::titles[t_id].holder.value;
			} else {
				auto ev = std::string("Error: title information not found for: ") + std::to_string(t_id) + "\r\n";
				OutputDebugStringA(ev.c_str());
				ev = std::string("tile name: ") + detail::titles[t_id].rname.get() + "\r\n";
				OutputDebugStringA(ev.c_str());
			}
		};



		global::holdertotitle.for_all(l, [&add_admin_for_t](char_id_t c, title_id_t t) {
			if (detail::people[c.value].primetitle == t && detail::people[c.value].died == 0) {
				add_admin_for_t(t.value);
			}
		});

		///
		///create province control records
		for (prov_id id = 1; id < detail::provinces.size(); ++id) {
			if ((detail::provinces[id].pflags & province::FLAG_HASTITLE) != 0) {
				auto admin = detail::titles[id].associated_admin;
				title_id ctitile = id;
				while (!valid_ids(admin)) {
					if (other_title_info[ctitile].t_leige == title::NONE || detail::titles[id].holder != detail::titles[other_title_info[ctitile].t_leige].holder)
						break;
					ctitile = other_title_info[ctitile].t_leige;
					admin = detail::titles[ctitile].associated_admin;
				}
				if (!valid_ids(admin)) {
					admin = detail::titles[detail::people[detail::titles[id].holder.value].primetitle.value].associated_admin;
				}
				if (valid_ids(admin)) {
					global::update_prov_control(prov_id_t(id), admin, l);
				}
			}
		}

	}

	LoadWater();
	LoadClimates();

	flat_map<__int32, prov_id> colortoprov;
	LoadProv(colortoprov);


	flat_map<__int64, std::vector<pair<pair<int, int>, pair<int, int>>>> segments;
	flat_map<edge, std::vector<std::vector<sf::Vector2f>>> fullborders;
	flat_multimap<edge, std::vector<sf::Vector2<short>>> oborders;

	sf::Vector2u size;
	MakeBorders(colortoprov, size, oborders, segments, fullborders);

	OutputDebugStringA("BOUNDS&INTERSECTS\r\n");
	for (size_t it = 1; it < detail::provinces.size(); it++) {
		std::vector<std::vector<std::vector<sf::Vector2f>>> borders;
		for (const auto& bdr : fullborders) {
			if (bdr.first.first == it || bdr.first.second == it) {
				borders.push_back(bdr.second);
			}
		}

		if (borders.size() > 0) {
			detail::provinces[it].bounds = bounds(borders, 0, borders.size());
			MakeIntersects(detail::provinces[it].intersect, detail::provinces[it].bounds, borders);
		}
	}

	LoadAjac(oborders);


	BuildProvDB();


	// generate initial force estimates
	flat_set<admin_id_t> all_admin;
	admin_pool.for_each(fake_lock(), [&all_admin](INOUT(administration) adm) {
		admin_id_t id = get_id(adm, fake_lock());
		all_admin.insert(id);
	});
	{
		w_lock wlk;
		raise_all_troops(all_admin, wlk);
	
		for (char_id i = 1; i != detail::people.size(); ++i) {
			if (valid_ids(detail::people[i].primetitle)) {
				with_udata(char_id_t(i), wlk, [i, &wlk](INOUT(udata) d) {
					d.mu = static_cast<float>(count_troops_raised(char_id_t(i), wlk));
					d.sigma_sq = known_variance;
				});
				
			}
		}
		
		for (auto a : all_admin) {
			stand_down_troop(a, wlk);

			const auto hos = head_of_state(a, wlk);

			std::vector<char_id_t> lst;
			global::get_living_family(hos, lst);

			for (auto chid : lst) {
				if (!valid_ids(get_object(chid).primetitle)) {
					with_udata(chid, wlk, [a, chid, &wlk, hos](INOUT(udata) d) {
						if (!valid_ids(d.a_court)) {
							d.a_court = a;
							insert_to_court(chid, a, wlk);
						} else if (get_object(chid).mother == hos || get_object(chid).father == hos) {
							d.a_court = a;
							insert_to_court(chid, a, wlk);
						}
					});
				}
			}
		}
	}
	

	UpdateTextData();

	// set global values: player id
	global::playerid = 1366;
	//set capitals
	{
		w_lock l;
		admin_pool.for_each(l, [&l](INOUT(administration) adm) {
			if (!valid_ids(adm.capital) || !is_controlled_by_a(adm.capital, admin_id_t(admin_pool.get_index(adm, l)), l)) {
				global::update_capital(admin_id_t(admin_pool.get_index(adm, l)), l);
			}
		});
	}

	sqlite3* db;
	int rc = sqlite3_open(":memory:", &db);
	if (rc) {
		OutputDebugStringA("FAILED TO OPEN DB\r\n");
		sqlite3_close(db);
		return;
	}
	file_generate(db);
	file_save_scenario(db, r_lock(), oborders);

	if (loadOrSaveDb(db, szPath, true) != SQLITE_OK) {
		OutputDebugStringA("SAVE ERROR\r\n");
	}

	sqlite3_close(db);
}

void load_save(const char *zFilename) {
	sqlite3* db;
	int rc = sqlite3_open(":memory:", &db);
	if (rc) {
		OutputDebugStringA("FAILED TO OPEN DB\r\n");
		sqlite3_close(db);
		return;
	}

	if (loadOrSaveDb(db, zFilename, false) != SQLITE_OK) {
		OutputDebugStringA("LOAD ERROR\r\n");
	}

	file_load(db, w_lock());

	sqlite3_close(db);
};

void load_save_no_ui(const char *zFilename) {
	sqlite3* db;
	int rc = sqlite3_open(":memory:", &db);
	if (rc) {
		OutputDebugStringA("FAILED TO OPEN DB\r\n");
		sqlite3_close(db);
		return;
	}

	if (loadOrSaveDb(db, zFilename, false) != SQLITE_OK) {
		OutputDebugStringA("LOAD ERROR\r\n");
	}

	file_load(db, w_lock(), true);

	sqlite3_close(db);
}
