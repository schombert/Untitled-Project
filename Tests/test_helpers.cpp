#include "globalhelpers.h"
#include "test_helpers.h"
#include "generation.hpp"
#include "laws.h"

bool load_test_data() {
	static bool data_loaded = false;
	if (!data_loaded) {
		data_loaded = true;
		if (file_exists("sample_data.db")) {
			load_save_no_ui("sample_data.db");
			return true;
		} else {
			OutputDebugStringA("Could not find sample data file\r\n");
			return false;
		}
	}
	return true;
}

title_id_t title_from_name(IN(std::string) name) {
	sref comp(name, sref::no_add);
	for (title_id i = 0; i != detail::titles.size(); ++i) {
		if (detail::titles[i].rname == comp)
			return title_id_t(i);
	}
	return title_id_t();
}

admin_id_t admin_from_name(IN(std::string) name) {
	sref comp(name, sref::no_add);
	for (title_id i = 1; i != detail::titles.size(); ++i) {
		if (detail::titles[i].rname == comp) {
			if (valid_ids(get_object(title_id_t(i)).associated_admin))
				return get_object(title_id_t(i)).associated_admin;
		}
	}
	return admin_id_t();
}

prov_id_t province_from_name(IN(std::string) name) {
	sref comp(name, sref::no_add);
	for (prov_id i = 0; i != detail::provinces.size(); ++i) {
		if (detail::provinces[i].name == comp)
			return prov_id_t(i);
	}
	return prov_id_t();
}