#pragma once
#include "globalhelpers.h"


extern std::string index_values;
using parse_ident = string_ref_t<std::string, &index_values>;

template<typename KEY, typename OBJ>
class uvector : public std::vector<std::pair<KEY, OBJ>> {
public:
	OBJ& operator[](IN(KEY) k) noexcept {
		for (size_t i = 0; i != size(); ++i) {
			if (std::vector<std::pair<KEY, OBJ>>::operator[](i).first == k)
				return std::vector<std::pair<KEY, OBJ>>::operator[](i).second;
		}
		emplace_back();
		return back().second;
	}
};

class parse_contents {
public:
	std::string contents;
	parse_contents(IN(std::string) s) : contents(s) {};
};

class parse_option {
public:
	//std::unique_ptr<parse_contents> content;
	//std::unique_ptr<std::pair<parse_ident, std::vector<parse_option>>> node;
	std::pair<parse_ident, std::vector<parse_option>> node;

	//parse_option(IN(std::string) s) : content(std::make_unique<parse_contents>(s)) {};
	parse_option(parse_ident id) : node(id, std::vector<parse_option>()) {};
	parse_option() {};
};


bool parse_v2(TCHAR* filename, INOUT(std::vector<parse_option>) results);