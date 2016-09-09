#pragma once
#include "globalhelpers.h"

#define S_TYPE_ERASED 0
#define S_TYPE_BATTLE 1
#define S_TYPE_SEIGE 2
#define S_TYPE_EVENT 3
#define S_TYPE_MISSION_EXEC 4
#define S_TYPE_MISSION_EXPIRE 5
#define S_TYPE_SM_STAGE 6

class s_actionbase {
public:
	unsigned short type;
	s_actionbase(unsigned short t) noexcept : type(t) {};
	virtual ~s_actionbase() {};
};

class s_battle : public s_actionbase {
public:
	const war_id_t war_for;
	const double loss, lossb;
	const bool attackers_won;
	s_battle(war_id_t w, double la, double lb, bool aw) noexcept : s_actionbase(S_TYPE_BATTLE), war_for(w), loss(la), lossb(lb), attackers_won(aw) {};
};

class s_seige : public s_actionbase {
public:
	const unsigned int duration;
	const prov_id_t sprov;
	const war_id_t war_for;
	s_seige(war_id_t w, prov_id_t si, unsigned int d) noexcept : s_actionbase(S_TYPE_SEIGE), war_for(w), sprov(si), duration(d) {};
};

class s_event : public s_actionbase {
public:
	const char_id_t host;
	const char_id_t target;
	const unsigned short event_type;
	s_event(char_id_t h, char_id_t t, unsigned short e) noexcept : s_actionbase(S_TYPE_EVENT), host(h), target(t), event_type(e) {};
};

class s_pact_mission_exec : public s_actionbase {
public:
	
	const char_id_t source;
	const char_id_t target;
	const admin_id_t adm;
	const unsigned char index;
	s_pact_mission_exec(char_id_t actor, char_id_t t, admin_id_t a, unsigned char num) noexcept : s_actionbase(S_TYPE_MISSION_EXEC), source(actor), target(t), adm(a), index(num) {};
};

class s_mission_expire : public s_actionbase {
public:
	const char_id_t source;
	const admin_id_t adm;
	const unsigned char index;
	s_mission_expire(char_id_t s, admin_id_t a, unsigned char num) noexcept : s_actionbase(S_TYPE_MISSION_EXPIRE), source(s), adm(a), index(num) {};
};

class s_sm_stage : public s_actionbase {
public:
	const admin_id_t source;
	const SM_TAG_TYPE index;
	s_sm_stage(admin_id_t s, SM_TAG_TYPE num) noexcept : s_actionbase(S_TYPE_SM_STAGE), source(s), index(num) {};
};

void execute_schedule() noexcept;