#pragma once
#include "globalhelpers.h"
#include "ai_mem.h"
#include <unordered_set>
#include "globalhelpers.h"
#include <vector>
#include <unordered_map>
#include <climits>
#include <memory>
#include <functional>
#include <type_traits>

using ai_priority_type = short;

#define MIN_P_PRIORITY SHRT_MIN;
#define MAX_P_PRIORITY SHRT_MAX;
#define NO_GOAL USHRT_MAX
#define NO_PLAN USHRT_MAX
#define PLAN_FINISHED USHRT_MAX
#define PLAN_FAILED USHRT_MAX-1


struct ai_prefs {
	float self;
	float nation;
	float family;
	float relations;
};

struct ai_plan_prefs {
	ai_priority_type self;
	ai_priority_type nation;
	std::function<void(std::vector<std::pair<int, short>> &lst, BYTE* params)> enumaffected;
};

class ai_blobs;
class plandata;

using plansList = std::vector < plandata >;

template<typename T>
class genericPlanMember {
public:
	unsigned short next;
	virtual unsigned short Exec(T* const data, ai_mem_pool &mem, plansList &pl, unsigned short thisplan, ai_blobs* const db, const ai_prefs& prfs) = 0;
	virtual void resume(T* const data, bool success, ai_mem_pool &mem, plansList &pl, unsigned short thisplan) {};
	virtual void results(T* const data, const void* const results, ai_mem_pool &mem, plansList &pl, unsigned short thisplan) {};
	genericPlanMember(unsigned short n) : next(n) {};
	virtual ~genericPlanMember() {
	};
};

inline bool operator==(const NullType & a, const NullType & b) {
	return true;
}

class plandata {
public:
	blockposition datamembers;
	unsigned short returnplan;
	unsigned short planid;
	unsigned short goal;
	unsigned short currentstep;
	ai_priority_type planpriority;
	plandata() {};


	template<typename DATA>
	plandata(unsigned short id, unsigned short gl, ai_priority_type p, unsigned short ret, ai_mem_pool &mem, DATA&& args) :
		goal(gl), planid(id), planpriority(p), returnplan(ret) {
		using basetype = std::remove_reference<DATA>::type;
		currentstep = 0;
		datamembers = ai_alloc<basetype>(mem, std::forward<DATA>(args));
	};
};

class PlanBase {
public:
	unsigned short ID;
	std::unordered_map<unsigned short, float> goalsatisfaction; //how good the plan is for a particular goal.
	ai_plan_prefs planeval;


	static std::vector<std::shared_ptr<PlanBase>> idsmap;
	static std::vector<std::vector<unsigned short>> plansforgoal; // vectors of plans indexed by the goal they serve

	virtual BYTE* conflictData(unsigned short goal, size_t& size, const plandata& pdat, ai_mem_pool &mem) = 0;
	virtual unsigned short execStep_inner(ai_mem_pool &mem, plansList &pl, unsigned short planindex, ai_blobs* const db, const ai_prefs& prfs) = 0;
	virtual size_t numSteps() = 0;
	virtual void freeData(blockposition &pos, ai_mem_pool &mem) = 0;
	virtual void resume(bool succes, ai_mem_pool &mem, plansList &pl, unsigned short planindex) = 0;
	virtual void results(const void* const results, ai_mem_pool &mem, plansList &pl, unsigned short thisplan, unsigned short step) = 0;
	virtual const char* description() const = 0;
	//template<typename U>
	//virtual void sendValue(U* val, ai_mem_pool &mem, plansList &pl, unsigned short planindex) = 0;

	
	virtual ~PlanBase() {};
	PlanBase(unsigned short i) : ID(i) {};
};


class ExposeConflictData {
public:
	inline BYTE* conflictData(unsigned short goal, size_t& size) const {
		return nullptr;
	}
};

class ExposeProbability {
public:
	float probability(ai_blobs* const db, const ai_prefs& prf) const {
		return 1.0f;
	}
};

class ExposeDescription {
public:
	static const char* description() { return "Undefined"; };
};

class ExposeEval {
public:
	ai_priority_type evalPlan(ai_blobs* const db, const plansList &pl, const ai_prefs & prefs) const  {
		return 0;
	}
};

template <typename T>
inline float getSProb(const T* const dat, ai_blobs* const db, const ai_prefs& prf, std::true_type) {
	return dat->probability(db, prf);
}

template <typename T>
inline float getSProb(const T* const dat, ai_blobs* const db, const ai_prefs& prf, std::false_type) {
	return 1.0f;
}

template<typename T>
inline BYTE* conflict_disp(const T* const ptr, unsigned short gl, size_t &size, std::true_type) {
	return ptr->conflictData(gl,size);
}
template<typename T>
inline BYTE* conflict_disp(const T* const ptr, unsigned short gl, size_t &size, std::false_type) {
	return nullptr;
}

template<typename T>
float conflict(unsigned short planid, const T* const ndat, const plandata &adat, ai_mem_pool &mem) {
	auto nPln = PlanBase::idsmap[planid];
	if (nPln->goalsatisfaction.find(adat.goal) != nPln->goalsatisfaction.cend()) {
		size_t datasize;
		BYTE* cdata;
		if (cdata = PlanBase::idsmap[adat.planid]->conflictData(adat.goal, datasize, adat, mem)) {
			size_t dataszb;
			BYTE* datb;
			if (datb = conflict_disp(ndat, adat.goal, dataszb, std::is_base_of<ExposeConflictData, T>())) {
				if (memcmp(cdata, datb, std::min(datasize, dataszb)) == 0)
					return nPln->goalsatisfaction[adat.planid];
			}
		}
	}
	return 0.0;
}

template<typename T, typename U, unsigned short goal>
bool confComparator(const T &a, const U &b);

template<typename T>
inline ai_priority_type customEval(const T* const params, ai_blobs* const db, const plansList &plans, const ai_prefs& prefs, std::true_type) {
	return params->evalPlan(db, plans, prefs);
}

template<typename T>
inline ai_priority_type customEval(const T* const params, ai_blobs* const db, const plansList &plans, const ai_prefs& prefs, std::false_type) {
	return 0;
}

template<typename T>
ai_priority_type evaluatePlan(ai_priority_type eval, unsigned short id, const T* const params, plansList &plans, ai_mem_pool &mem, ai_blobs* const db, const ai_prefs& prfs) {
	auto pln = PlanBase::idsmap[id];
	eval += static_cast<ai_priority_type>(pln->planeval.self * prfs.self);
	eval += static_cast<ai_priority_type>(pln->planeval.nation * prfs.nation);

	// check affects on friends & enemies

	for (const auto &pd : plans) {
		eval += static_cast<ai_priority_type>(conflict(id, params, pd, mem) * pd.planpriority);
	}

	eval += customEval(params, db, plans, prfs, std::is_base_of<ExposeEval, T>());
	eval = static_cast<ai_priority_type>(static_cast<float>(eval) * getSProb(params, db, prfs, std::is_base_of<ExposeProbability, T>()));
	return eval;
}

template<typename T>
inline const char* descriptionDispatch(std::true_type) {
	return T::description();
}

template<typename T>
inline const char* descriptionDispatch(std::false_type) {
	return "Undefined";
}

template<typename T>
class Plan : public PlanBase {
public:
	std::vector<std::unique_ptr<genericPlanMember<T>>> steps;

	inline BYTE* conflictData_disp(unsigned short goal, size_t& size, const plandata& pdat, ai_mem_pool &mem, std::true_type) {
		return posToPtr<T>(pdat.datamembers, mem)->conflictData(goal, size);
	}
	inline BYTE* conflictData_disp(unsigned short goal, size_t& size, const plandata& pdat, ai_mem_pool &mem, std::false_type) {
		return nullptr;
	}
	
	virtual BYTE* conflictData(unsigned short goal, size_t& size, const plandata& pdat, ai_mem_pool &mem) {
		return conflictData_disp(goal, size, pdat, mem, std::is_base_of<ExposeConflictData, T>());
	};
	virtual size_t numSteps()  {
		return steps.size();
	};
	virtual unsigned short execStep_inner(ai_mem_pool &mem, plansList &pl, unsigned short planindex, ai_blobs* const db, const ai_prefs& prfs) {
		return steps[pl[planindex].currentstep]->Exec(posToPtr<T>(pl[planindex].datamembers, mem), mem, pl, planindex, db, prfs);
	};
	virtual void freeData(blockposition &pos, ai_mem_pool &mem) {
		ai_free<T>(pos, mem);
	}
	virtual void resume(bool succes, ai_mem_pool &mem, plansList &pl, unsigned short planindex) {
		steps[pl[planindex].currentstep]->resume(posToPtr<T>(pl[planindex].datamembers, mem), succes, mem, pl, planindex);
	}
	virtual void results(const void* const results, ai_mem_pool &mem, plansList &pl, unsigned short thisplan, unsigned short step) {
		steps[step]->results(posToPtr<T>(pl[thisplan].datamembers, mem), results, mem, pl, thisplan);
	}
	virtual const char* description() const {
		return descriptionDispatch<T>(std::is_base_of<ExposeDescription, T>());
	};
	Plan(unsigned short i) : PlanBase(i) {};
};

unsigned short execStep(ai_mem_pool &mem, plansList &pl, unsigned short planindex, ai_blobs* const db, const ai_prefs& prfs);

struct idevalpair {
	union {
		unsigned short planid;
		unsigned short listpos;
	} data;
	ai_priority_type eval;
};

template<unsigned short goal>
void enumeratePlansForGoal(std::vector<idevalpair> &planIDs) {
	planIDs.reserve(PlanBase::plansforgoal[goal].size());
	for (auto pln : PlanBase::plansforgoal[goal]) {
		planIDs.emplace_back();
		planIDs.back().data.planid = pln;
	}
}

template<>
void enumeratePlansForGoal<NO_GOAL>(std::vector<idevalpair> &planIDs);

template<typename T, typename U, unsigned short goal>
class goalLoop : public genericPlanMember<T> {
public:
	const unsigned short fail;
	virtual void resume(T* const args, bool success, ai_mem_pool &mem, plansList &pl, unsigned short thisplan) {
		idevalpair* localdata = posToPtr<idevalpair>(args->ptr, mem);
		if (!localdata)
			return;

		++args->counter;
		if (success) { //sucess
			ai_free(args->ptr, mem);
			pl[thisplan].currentstep = next;
		}
		else if (localdata[args->counter].data.planid == NO_PLAN) {
			ai_free(args->ptr, mem);
			pl[thisplan].currentstep = fail;
		}
		else {//create new plan if ! passed;
			pl.emplace_back(localdata[args->counter].data.planid, goal, localdata[args->counter].eval, thisplan, mem, U(args->goalparams));
			localdata[args->counter].data.listpos = static_cast<unsigned short>(pl.size() - 1);
		}
	}

	virtual unsigned short Exec(T* const args, ai_mem_pool &mem, plansList &pl, unsigned short thisplan, ai_blobs* const db, const ai_prefs& prfs) {
		if (args->ptr.size == 0) {
			//enumerate & order plans for goal, alloc mem for list & alloc space for return values from plans, terminate list with USHRT_MAX
			std::vector<idevalpair> plans;
			enumeratePlansForGoal<goal>(plans);
			U goalparams(args->goalparams);
			for (unsigned int indx = 0; indx < plans.size(); ++indx) {
				ai_priority_type eval = evaluatePlan(static_cast<ai_priority_type>(pl[thisplan].planpriority * PlanBase::idsmap[plans[indx].data.planid]->goalsatisfaction[goal]), plans[indx].data.planid, &goalparams, pl, mem, db, prfs);
				if (eval > 0) {
					plans[indx].eval = eval;
				}
				else { // list only positive
					plans[indx] = plans.back();
					plans.pop_back();
					--indx;
				}
			}

			std::sort(plans.begin(), plans.end(), [&](const idevalpair &a, const idevalpair &b){return a.eval > b.eval; });

			if (plans.size() == 0)
				return fail;

			args->ptr = ai_alloc(sizeof(idevalpair)*(plans.size() + 1), mem);
			idevalpair* localdata = posToPtr<idevalpair>(args->ptr, mem);

			for (unsigned int i = 0; i < plans.size(); ++i) {
				localdata[i] = plans[i];
			}
			localdata[plans.size()].data.planid = NO_PLAN;

			args->counter = 0;
			/**/
			pl.emplace_back(localdata[0].data.planid, goal, localdata[0].eval, thisplan, mem, U(args->goalparams));
			/**/
			localdata[0].data.listpos = static_cast<unsigned short>(pl.size() - 1);
			return pl[thisplan].currentstep;
		}
		else {
			idevalpair* localdata = posToPtr<idevalpair>(args->ptr, mem);
			if (localdata[args->counter].data.listpos < pl.size()) {
				execStep(mem, pl, localdata[args->counter].data.listpos, db, prfs);
			}
			else {
				resume(args, false, mem, pl, thisplan);
			}
			return pl[thisplan].currentstep;
		}

	}
	goalLoop(unsigned short n, unsigned short f) : fail(f), genericPlanMember<T>(n) {};
};

template<typename T, typename U, unsigned short ID>
class tryPossible : public genericPlanMember<T> {
public:
	struct enumstruct {
		U param;
		union {
			ai_priority_type eval;
			unsigned short listpos;
		} data;
		template<typename ... Q>
		enumstruct(const Q ... p) : param(p ...) {}
	};

	const unsigned short fail;
	const std::function<void(std::vector<enumstruct>&, T*, ai_blobs*)> enumf;

	virtual void resume(T* const args, bool success, ai_mem_pool &mem, plansList &pl, unsigned short thisplan) {
		enumstruct* localdata = posToPtr<enumstruct>(args->ptr, mem);
		if (!localdata)
			return;

		++args->counter;
		if (success) {
			ai_free(args->ptr, mem);
			pl[thisplan].currentstep = next;
		}
		else if (localdata[args->counter].data.eval < 0) {
			ai_free(args->ptr, mem);
			pl[thisplan].currentstep = fail;
		}
		else {//create new plan if ! passed;
			pl.emplace_back(ID, NO_GOAL, localdata[args->counter].data.eval, thisplan, mem, localdata[args->counter].param);
			localdata[args->counter].data.listpos = static_cast<unsigned short>(pl.size() - 1);
		}
	}

	virtual unsigned short Exec(T* const args, ai_mem_pool &mem, plansList &pl, unsigned short thisplan, ai_blobs* const db, const ai_prefs& prfs) {
		if (args->ptr.size == 0) {
			//enumerate & order plans for goal, alloc mem for list & alloc space for return values from plans, terminate list with USHRT_MAX
			std::vector<enumstruct> plans;
			enumf(plans, args, db);

			for (unsigned int indx = 0; indx < plans.size(); ++indx) {
				ai_priority_type eval = evaluatePlan(pl[thisplan].planpriority, ID, &plans[indx].param, pl, mem, db, prfs);
				if (eval > 0) {
					plans[indx].data.eval = eval;
				}
				else { // list only positive
					plans[indx] = plans.back();
					plans.pop_back();
					--indx;
				}
			}
			
			std::sort(plans.begin(), plans.end(), [&](const enumstruct &a, const enumstruct &b){return a.data.eval > b.data.eval; });

			if (plans.size() == 0)
				return fail;

			args->ptr = ai_alloc(sizeof(enumstruct)*(plans.size()+1), mem);
			enumstruct* localdata = posToPtr<enumstruct>(args->ptr, mem);

			for (unsigned int i = 0; i < plans.size(); ++i) {
				localdata[i] = plans[i];
			}
			localdata[plans.size()].data.eval = -1;

			args->counter = 0;
			/**/
			pl.emplace_back(ID, NO_GOAL, localdata[0].data.eval, thisplan, mem, localdata[0].param);
			/**/
			localdata[0].data.listpos = static_cast<unsigned short>(pl.size() - 1);
			return pl[thisplan].currentstep;
		}
		else {
			enumstruct* localdata = posToPtr<enumstruct>(args->ptr, mem);
			if (localdata[args->counter].data.listpos < pl.size()) {
				execStep(mem, pl, localdata[args->counter].data.listpos, db, prfs);
			}
			else {
				resume(args, false, mem, pl, thisplan);
			}
			return pl[thisplan].currentstep;
		}

	}
	tryPossible(unsigned short n, unsigned short fa, std::function<void(std::vector<enumstruct>&, T*, ai_blobs*)> f) : fail(fa), genericPlanMember<T>(n), enumf(f) {};
};


template<typename T>
class testCondition : public genericPlanMember<T> {
public:
	unsigned short onfail;
	std::function<bool (T*, ai_blobs*, const ai_prefs&)> conditionTest;

	virtual unsigned short Exec(T* const args, ai_mem_pool &mem, plansList &pl, unsigned short thisplan, ai_blobs* const db, const ai_prefs& prfs) {
		if (conditionTest(args, db, prfs)) {
			return next;
		}
		return onfail;
	};
	testCondition(unsigned short pass, unsigned short fail, std::function<bool(T*, ai_blobs*, const ai_prefs&)> test) : genericPlanMember<T>(pass), onfail(fail), conditionTest(test) {};
};


template<unsigned short steps>
void ExecuteSteps(ai_mem_pool &mem, plansList &pl, ai_blobs* const db, const ai_prefs& prfs) {
	for (unsigned short cntr = 0; cntr < steps && pl.size() > 0;) {
		for (unsigned int cplan = 0; cplan < pl.size() && cntr < steps; ++cplan) {
			execStep(mem, pl, cplan, db, prfs);
			++cntr;
		}
		while (pl.size() > 0 && (pl.back().currentstep == PLAN_FINISHED || pl.back().currentstep == PLAN_FAILED)) {
			pl.pop_back();
		}
	}
}