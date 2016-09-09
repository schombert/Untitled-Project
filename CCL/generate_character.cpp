#include "globalhelpers.h"
#include "generate_character.h"
#include "structs.hpp"
#include "living_data.h"
#include "court.h"
#include "laws.h"

char_id_t generate_courtier_a(admin_id_t a, IN(w_lock) l) {
	IN(auto) adm = get_object(a, l);

	char_id newid = static_cast<char_id>(detail::people.size());
	detail::people.emplace_back();

	INOUT(auto) person = detail::people[newid];
	IN(auto) holder = get_object(adm.executive);
	
	person.gender = holder.gender;

	IN(auto) cap = get_object(adm.capital);
	IN(auto) cul = get_object(valid_ids(adm.court_culture) ? adm.court_culture : cap.culture);

	if (person.gender == 0) {
		person.name = cul.mnames[global_store.get_fast_int() % cul.mnames.size()];
	} else {
		person.name = cul.fnames[global_store.get_fast_int() % cul.fnames.size()];
	}

	auto statvals = global_store.get_int();
	
	double age_range = 5.0 - 10.0  * global_store.get_double();
	person.born = static_cast<unsigned int>(static_cast<int>(global::currentday - 365 * 26) - static_cast<int>(365.0 * age_range));

	untitled_data* newdata = nullptr;
	create_untitled(char_id_t(newid), newdata, l);

	newdata->culture = valid_ids(adm.court_culture) ? adm.court_culture : cap.culture;
	newdata->religion = valid_ids(adm.official_religion) ? adm.official_religion : cap.religion;

	newdata->stats.analytic = statvals & 0x7;
	statvals = statvals >> 3i64;
	newdata->stats.martial = statvals & 0x7;
	statvals = statvals >> 3i64;
	newdata->stats.social = statvals & 0x7;
	statvals = statvals >> 3i64;
	newdata->stats.intrigue = statvals & 0x7;
	statvals = statvals >> 3i64;

	newdata->attrib.load(statvals);


	add_to_court_a(char_id_t(newid), a, l);

	return char_id_t(newid);
}