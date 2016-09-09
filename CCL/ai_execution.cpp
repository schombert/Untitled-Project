#include "globalhelpers.h"
#include "ai_execution.h"
#include "structs.hpp"
#include "wardata.h"
#include "random_activity.h"

#define Q_WEIGHT 1

const double u_value = UPDATE_FREQ * Q_WEIGHT;

void execute_ai(IN_P(ai_blobs) blb, IN(g_lock) l) {
	const auto id = blb->wars.primary;

	blb->strength_estimate = -DBL_MAX;
	blb->income_estimate = -DBL_MAX;
	blb->defensive_estimate = -DBL_MAX;

	blb->activity -= u_value;
	
	blb->interests.interest_tick(id, l);
	if (blb->activity <= 0.0) {
		//increase activity
		generate_activity(id);
	}

	// 
	// blb->ai.update(id, l);
}