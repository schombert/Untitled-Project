#pragma once
#include "globalhelpers.h"
#include <Windows.h>

class prse_assoc {
public:
	std::string left;
	std::string right;
	prse_assoc(std::string &val, std::string &valb) : left(val), right(valb) {}
};

class prse_value {
public:
	std::string value;
	prse_value(std::string &val) : value(val) {}
};

class prse_list;

class prse_generic {
public:
	std::unique_ptr<prse_assoc> assoc;
	std::unique_ptr<prse_value> value;
	std::unique_ptr<prse_list> list;

	prse_generic(prse_generic& in) : assoc(std::move(in.assoc)), value(std::move(in.value)), list(std::move(in.list)) {};

	template<typename T, typename U, typename V>
	prse_generic(T&& a, U&& v, V&& l) : assoc(std::forward<T>(a)), value(std::forward<U>(v)), list(std::forward<V>(l)) {}
};

class prse_list {
public:
	std::string value;
	std::vector<prse_generic> list;

	prse_list() {};
};

void ParseCKFile(TCHAR* filename, std::unique_ptr<prse_list> &results, bool(*exclude)(std::string&) = NULL);