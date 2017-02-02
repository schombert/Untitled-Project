#include "globalhelpers.h"
#include "ai_plan.h"
#include <unordered_map>


std::vector<std::shared_ptr<PlanBase>> PlanBase::idsmap;
std::vector<std::vector<unsigned short>> PlanBase::plansforgoal; // vectors of plans indexed by the goal they serve

unsigned short execStep(ai_mem_pool &mem, plansList &pl, unsigned short planindex, ai_blobs* const db, const ai_prefs& prfs) {
	if (planindex >= pl.size()) {
		return PLAN_FAILED;
	}

	auto pln = PlanBase::idsmap[pl[planindex].planid];
	unsigned short step;

	if (pl[planindex].currentstep == PLAN_FINISHED || pl[planindex].currentstep == PLAN_FAILED || pl[planindex].currentstep >= pln->numSteps()) {
		step = pl[planindex].currentstep;
	}
	else {
		//removing temp variable WILL cause problems as the memory the pl array points to may change during the step (if a new goal is added)
		//and thus the province it will be stored into will no longer be valid; the value will then be lost, and live memory could be corrupted
		step = pln->execStep_inner(mem, pl, planindex, db, prfs);
		pl[planindex].currentstep = step;

		if (step == PLAN_FINISHED || step == PLAN_FAILED) {
			bool result = step == PLAN_FINISHED;
			unsigned short returndest = pl[planindex].returnplan;
			pl[planindex].returnplan = NO_PLAN;
			pln->freeData(pl.back().datamembers, mem);

			if (planindex == pl.size() - 1)
				pl.pop_back();

			if (returndest != NO_PLAN && returndest < pl.size()) {
				PlanBase::idsmap[pl[returndest].planid]->resume(result, mem, pl, returndest);
			}
		}
	}


	while (pl.size() > 0 && (pl.back().currentstep == PLAN_FINISHED || pl.back().currentstep == PLAN_FAILED)) {
		bool result = pl.back().currentstep == PLAN_FINISHED;
		unsigned short returndest = pl.back().returnplan;
		pl.back().returnplan = NO_PLAN;

		PlanBase::idsmap[pl.back().planid]->freeData(pl.back().datamembers, mem);
		pl.pop_back();

		if (returndest != NO_PLAN && returndest < pl.size()) {
			PlanBase::idsmap[pl[returndest].planid]->resume(result, mem, pl, returndest);
		}

	}

	return step;

};

template<>
void enumeratePlansForGoal<NO_GOAL>(std::vector<idevalpair> &planIDs) {
	return;
}