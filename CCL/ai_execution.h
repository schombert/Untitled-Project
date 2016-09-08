#pragma once
#include "globalhelpers.h"
#include "datamanagement.hpp"
#include "wardata.h"
#include "ai_interest.h"
#include "envoys.h"


#define P_FLAG_PERPARING_EVENT 0x0001

class ai_blobs : public std::enable_shared_from_this<ai_blobs> {
public:
	double activity = 0.0;

	double strength_estimate = -DBL_MAX;
	double income_estimate = -DBL_MAX;
	double defensive_estimate = -DBL_MAX;
	double wealth = 0.0;
	double p_honorable;
	double p_peaceful;

	//ai_collection ai;
	
	interest_manager interests;
	warcollection wars;
	std::vector<std::unique_ptr<envoy_mission>> envoy_missions;

	unsigned short flags = 0;

	ai_blobs(char_id p) : wars(p), p_honorable(0.5), p_peaceful(0.5) {};
};

void execute_ai(IN_P(ai_blobs) blb, IN(g_lock) l);