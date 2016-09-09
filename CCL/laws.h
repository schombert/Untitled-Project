#pragma once
#include "globalhelpers.h"
#include "structs.hpp"

using sub_title_type = unsigned char;


struct sub_title {
	static constexpr sub_title_type EXECUTIVE = 0; // should not actually be used
	static constexpr sub_title_type CHANCELLOR =		0b00000001;
	static constexpr sub_title_type MARSHALL =			0b00000010;
	static constexpr sub_title_type MAGISTRATE =		0b00000011;
	static constexpr sub_title_type SPY_MASTER =		0b00000100;
	static constexpr sub_title_type REGENT =			0b00000101;
	static constexpr sub_title_type ADVISOR =			0b00000110;
	static constexpr sub_title_type DESIGNATED_HEIR =	0b00000111;

	static constexpr sub_title_type VOTING_TYPE =		0b10000000;

	static constexpr sub_title_type LEGISLATIVE_COUNCIL = 0b10000001;
	static constexpr sub_title_type MILITARY_COUNCIL =	0b10000010;
	static constexpr sub_title_type JUDICIAL_COUNCIL =	0b10000011;
	static constexpr sub_title_type RELIGIOUS_COUNCIL =	0b10000100;

	static constexpr size_t NUM_SUB_TITLE_TYPES = 11;

	char_id_t holder;
	sub_title_type type;
	unsigned char title_flags;
};

struct r_sub_title {
	title_id_t parent;
	sub_title_type type;
};

extern multiindex<title_id_t, sub_title> sub_titles;
extern multiindex<char_id_t, r_sub_title> r_sub_titles;

namespace inheritance {
	static constexpr unsigned char INHERITANCE_TYPE_MASK = 0b00000111;
	static constexpr unsigned char PARTIBLE = 0b00000000;
	static constexpr unsigned char PRIMOGENITURE = 0b00000001;
	static constexpr unsigned char ELECTIVE = 0b00000010;
	static constexpr unsigned char ELECTIVE_NON_DYNASTIC = 0b00000011;
	static constexpr unsigned char ELECTIVE_DYNASTIC_ONLY = 0b00000100;
	static constexpr unsigned char LIFETIME_APPOINTMENT = 0b00000101; // for sub titles
	static constexpr unsigned char REGENT = 0b00000110; // for title flags in admin
	static constexpr unsigned char GENDER_LAW_MASK = 0b00111000;
	static constexpr unsigned char MALE = 0b00001000;
	static constexpr unsigned char FEMALE = 0b00000000;
	static constexpr unsigned char INHERIT_THROUGH_OTHER = 0b00010000;
	static constexpr unsigned char PREFERENCE_ONLY = 0b00100000; // should never co-occur with inherit through other, preference only implies the above
	static constexpr unsigned char GENDER_EQUALITY = 0b00111000;
};

class administration {
public:
	static constexpr admin_id NONE = max_value<admin_id>::value;

	title_stats stats;															// 4bytes
	char_id_t executive;														// 8bytes


	//law protections
		static constexpr unsigned short RELIGION_CP = 0x0001;					// CP = constitutionally protected, HP = protected by honor
		static constexpr unsigned short RELIGION_HP = 0x0002;
		static constexpr unsigned short LEGISLATIVE_CP = 0x0004;
		static constexpr unsigned short LEGISLATIVE_HP = 0x0008;
		static constexpr unsigned short WAR_CP = 0x0010;
		static constexpr unsigned short WAR_HP = 0x0020;
		static constexpr unsigned short JUDGEMENT_CP = 0x0040;
		static constexpr unsigned short JUDGEMENT_HP = 0x0080;
		static constexpr unsigned short STANDARD_VASSAL_CP = 0x0100;
		static constexpr unsigned short STANDARD_VASSAL_HP = 0x0200;
		static constexpr unsigned short INTERNAL_WARS_CP = 0x0400;
		static constexpr unsigned short INTERNAL_WARS_HP = 0x0800;
		static constexpr unsigned short INHERITANCE_CP = 0x1000;
		static constexpr unsigned short INHERITANCE_HP = 0x2000;
		static constexpr unsigned short EXECUTIVE_VETO_CP = 0x4000;
		static constexpr unsigned short EXECUTIVE_VETO_HP = 0x8000;

	unsigned short protections;													// 10bytes

	title_id_t associated_title;												// 12bytes -- never set to title_id_t::NONE
	prov_id_t capital;															// 14bytes
	cul_id_t court_culture;														// 16bytes
	rel_id_t official_religion;													// 18bytes
	admin_id_t leige;															// 20bytes

	// estimation of military power
	// float mu;																	// 24bytes
	// float sigma_sq;																// 28bytes

	//powers
		static constexpr sub_title_type BY_VOTE = max_value<sub_title_type>::value;	// NOTE: all vassals vote
		static constexpr sub_title_type EXECUTIVE = 0;

	sub_title_type legislative;														// 29bytes
	sub_title_type war;																// 30bytes
	sub_title_type judgement;														// 31bytes
	sub_title_type religious_head;													// 32bytes

	//contract for this title to leige
	unsigned char vassal_req;													// 33bytes
	unsigned char vassal_flags;													// 34bytes

	//standard vassal contract
		static constexpr unsigned char TAX_MASK = 0x0F;
		static constexpr unsigned char TROOP_MASK = 0xF0;
		static constexpr unsigned char TAX_MINIMUM = 0x00;
		static constexpr unsigned char TAX_LOW = 0x01;
		static constexpr unsigned char TAX_MEDIUM = 0x02;
		static constexpr unsigned char TAX_HIGH = 0x03;
		static constexpr unsigned char TAX_MAXIMUM = 0x04;
		static constexpr unsigned char TROOP_MINIMUM = 0x00;
		static constexpr unsigned char TROOP_LOW = 0x10;
		static constexpr unsigned char TROOP_MEDIUM = 0x20;
		static constexpr unsigned char TROOP_HIGH = 0x30;
		static constexpr unsigned char TROOP_MAXIMUM = 0x40;

		static constexpr unsigned char VFLAG_NO_VOTE = 0x01;
		static constexpr unsigned char VFLAG_NO_EXT_WARS = 0x02;
		static constexpr unsigned char VFLAG_NO_ARMIES = 0x04;

	unsigned char standard_vassal_req;											// 35bytes
	unsigned char standard_vassal_flags;										// 36bytes

	double tax_amount() const noexcept {
		return static_cast<double>(vassal_req & TAX_MASK) / 5.0;
	}

	//global title flags
		//includes inheritance of executive
		static constexpr unsigned char TFLAG_NO_EXECUTIVE_VETO =	0b10000000;
		static constexpr unsigned char TFLAG_NO_INTERNAL_WARS =		0b01000000;

	unsigned char title_flags;													// 37bytes - 8 = 29 bytes


	administration() noexcept : associated_title(title::NONE) {};
	void random_administration(title_id_t at, prov_id_t cap, cul_id_t culture, rel_id_t religion, admin_id_t leige_id, IN(w_lock) l) noexcept;

	void construct() noexcept {
		// mu = 0.0f;
		// sigma_sq = 0.0f;
	}
	void set_clear() noexcept {
		associated_title = title_id_t::NONE;
	}
	bool is_clear() const noexcept {
		return !valid_ids(associated_title);
	}
};


class envoy_mission;

extern multiindex<admin_id_t, admin_id_t> leigetoadmin;
extern v_pool_t<administration, admin_id> admin_pool;
extern multiindex<admin_id_t, concurrent_uniq<envoy_mission>> envoy_missions;
extern multiindex<admin_id_t, SM_TAG_TYPE> spy_missions;

inline const administration& get_object(admin_id_t id, IN(g_lock) l) noexcept {
	return admin_pool.get(id.value, l);
}
inline administration& get_object(admin_id_t id, IN(w_lock) l) noexcept {
	return admin_pool.get(id.value, l);
}
inline admin_id_t get_id(IN(administration) obj, IN(g_lock) l) noexcept {
	return admin_id_t(admin_pool.get_index(obj, l));
}

administration& new_admin(INOUT(admin_id_t) id, IN(w_lock) l) noexcept;
admin_id_t new_admin_id(IN(w_lock) l) noexcept;
void create_sub_title(title_id_t parent, sub_title_type type, char_id_t holder, unsigned char title_flags, IN(w_lock) l) noexcept;
void transfer_sub_title(title_id_t parent, sub_title_type type, char_id_t oldholder, char_id_t newholder, IN(w_lock) l) noexcept;
void destroy_sub_title(title_id_t parent, sub_title_type type, char_id_t holder, IN(w_lock) l) noexcept;
void list_direct_controlled_admin(char_id_t id, INOUT(flat_set<admin_id_t>) s, IN(w_lock) l) noexcept;
void list_direct_controlled_admin(char_id_t id, INOUT(std::vector<admin_id_t>) s, IN(w_lock) l) noexcept;
void list_direct_controlled_admin(char_id_t id, INOUT(cflat_set<admin_id_t>) s, IN(g_lock) l) noexcept;
void list_direct_controlled_admin(char_id_t id, INOUT(cvector<admin_id_t>) s, IN(g_lock) l) noexcept;
char_id_t get_prim_heir(IN(administration) adm, char_id_t current_holder, IN(r_lock) l) noexcept;
char_id_t head_of_state(IN(administration) adm) noexcept;
char_id_t head_of_state(admin_id_t a, IN(g_lock) l) noexcept;
bool can_raise_troops(char_id_t c, IN(administration) adm) noexcept;
char_id_t get_sub_title_holder(title_id_t parent, sub_title_type st, IN(r_lock) l) noexcept;
char_id_t get_admin_holder(admin_id_t id, IN(g_lock) l) noexcept;
admin_id_t get_prime_leige(char_id_t id, IN(g_lock) l) noexcept;
admin_id_t get_prime_admin(char_id_t id, IN(g_lock) l) noexcept;
admin_id_t get_associated_admin(char_id_t id, IN(g_lock) l) noexcept;
char_id_t get_diplo_decider(IN(administration) adm, IN(g_lock) l) noexcept;
char_id_t get_local_decider(IN(administration) adm, IN(g_lock) l) noexcept;
char_id_t get_spy_decider(IN(administration) adm, IN(g_lock) l) noexcept;
char_id_t get_leg_decider(IN(administration) adm, IN(g_lock) l) noexcept;
char_id_t get_war_decider(IN(administration) adm, IN(g_lock) l) noexcept;
char_id_t get_judge_decider(IN(administration) adm, IN(g_lock) l) noexcept;
double power_in_administration(admin_id_t adm, char_id_t ch, IN(g_lock) l) noexcept; // ranging from 1.0 = all powers to 0.0 = no powers

constexpr float known_variance = 100.0f;

void get_force_estimate(char_id_t p, INOUT(float) mu, INOUT(float) sigmasq, IN(g_lock) l) noexcept;
void get_defensive_force_estimate(char_id_t p, INOUT(float) mu, INOUT(float) sigmasq, IN(g_lock) l) noexcept;
void get_defensive_against_force_estimate(char_id_t p, char_id_t against, INOUT(float) mu, INOUT(float) sigmasq, IN(g_lock) l) noexcept;
void update_force_est(char_id_t, float pts_raised, IN(w_lock) l) noexcept;

template<typename T>
char_id_t enum_sub_title_holders(title_id_t parent, sub_title_type st, IN(r_lock) l, IN(T) f) noexcept {
	sub_titles.for_each(parent, l, [st, &f](IN(sub_title) t) {
		if (t.type == st) {
			f(t.holder);
		}
	});
}

void admin_f_generate(IN_P(sqlite3) db) noexcept;
void admin_f_load(IN_P(sqlite3) db, IN(w_lock) l) noexcept;
void admin_f_save(IN_P(sqlite3) db, IN(r_lock) l) noexcept;