#pragma once
#include "ai_plan.h"
#include <iostream>

#define TEST_GOAL_0 0
#define TEST_GOAL_FIND_NUM 1
#define TEST_GOAL_DISP_TEST 2

#define TEST_PLAN_0 0
#define TEST_PLAN_1_TESTER 1
#define TEST_PLAN_2_TEST 2
#define TEST_PLAN_3_GOALDISP 3
#define TEST_PLAN_4_TST1 4
#define TEST_PLAN_5_TST2 5
#define TEST_PLAN_6_TST3 6
#define TEST_PLAN_7_TST4 7
#define TEST_PLAN_8_PBDISP 8
#define TEST_PLAN_9_PBTSTS 9

class intholder : public ExposeConflictData {
public:
	int data;
	intholder() : data(1) {};
	intholder(const int &in) : data(in) {};
	inline BYTE* conflictData(unsigned short goal, size_t& size) const {
		if (goal == TEST_GOAL_0) {
			size = sizeof(data);
			return (BYTE*)&data;
		}
		return nullptr;
	}
};

class simpleIntStep : public genericPlanMember<intholder> {
public:
	virtual unsigned short Exec(intholder* const args, ai_mem_pool &mem, plansList &pl, unsigned short thisplan, ai_blobs* const db, const ai_prefs& prfs) {
		std::cout << args->data << std::endl;
		args->data++;
		return next;
	}
	simpleIntStep(unsigned int n) : genericPlanMember<intholder>(n) {};
};

class basicCounter {
public:
	blockposition ptr;
	unsigned short counter;
	int* intref;
	basicCounter(int *in) : intref(in) {};
};

class gltestprm {
public:
	int val;
	int* ptr;
	gltestprm(int v, int* p) : val(v), ptr(p) {};
};

class tptestprm : public ExposeProbability {
public:
	int val;
	int* ptr;
	tptestprm(int v, int* p) : val(v), ptr(p) {};
	float probability(ai_blobs* const db, const ai_prefs& prf) const {
		return val == 6 ? 1.0f : 0.0f;
	}
};

class pbcounter {
public:
	blockposition ptr;
	unsigned short counter;
	int* o;
	pbcounter(int* out) : o(out) {};
};


class glcounter {
public:
	blockposition ptr;
	unsigned short counter;
	gltestprm goalparams;
	glcounter(int* out) : goalparams(3, out) {};
};

class intandrefcounter {
public:
	int* intref;
	int cnter;
	//intandrefcounter() {};
	intandrefcounter(const basicCounter *b, int val) : intref(b->intref), cnter(val) {};
	inline intandrefcounter& operator=(const intandrefcounter &in) { intref = in.intref; cnter = in.cnter; return *this; };
};

class trypbValues : public tryPossible<pbcounter, tptestprm, TEST_PLAN_9_PBTSTS> {
public:
	using parenttype = tryPossible < pbcounter, tptestprm, TEST_PLAN_9_PBTSTS >;
	trypbValues() : tryPossible(PLAN_FINISHED, PLAN_FAILED, [](std::vector<parenttype::enumstruct>& vals, pbcounter* params, ai_blobs* db){
		for (int i = 1; i < 10; i++) {
			vals.emplace_back(i, params->o);
		}
	}) {};
};

class tryValues : public tryPossible<basicCounter, intandrefcounter, TEST_PLAN_2_TEST> {
public:
	using parenttype = tryPossible < basicCounter, intandrefcounter, TEST_PLAN_2_TEST >;
	tryValues() : tryPossible(1, PLAN_FAILED, [](std::vector<parenttype::enumstruct>& vals, basicCounter* params, ai_blobs* db) {
		for (int i = 1; i < 10; i++) {
			vals.emplace_back(params, i);
			//vals.back().param = intandrefcounter(params, i);
		}
	}) {};
};

class goaldispatch : public goalLoop<glcounter, gltestprm, TEST_GOAL_DISP_TEST> {
public:
	goaldispatch() : goalLoop(PLAN_FINISHED, PLAN_FAILED) {};
};

template<int N>
class glTestN : public testCondition < gltestprm > {
public:
	glTestN() : testCondition(PLAN_FINISHED, PLAN_FAILED, [](gltestprm* i, ai_blobs* db, const ai_prefs& prf){*i->ptr |= (0x01 << N); std::cout << N << std::endl; return i->val == N; }) {};
};

//tptestprm
template<int N>
class pbTestN : public testCondition < tptestprm > {
public:
	pbTestN() : testCondition(PLAN_FINISHED, PLAN_FAILED, [](tptestprm* i, ai_blobs* db, const ai_prefs& prf) {*i->ptr |= (0x01 << N); std::cout << N << std::endl; return i->val == N; }) {};
};

class testfor5 : public testCondition < intandrefcounter > {
public:
	testfor5() : testCondition(PLAN_FINISHED, PLAN_FAILED, [](intandrefcounter* i, ai_blobs* db, const ai_prefs& prf){*(i->intref) = i->cnter; std::cout << i->cnter << std::endl; return i->cnter == 5; }) {};
};

class testfor5B : public testCondition < basicCounter > {
public:
	testfor5B() : testCondition(PLAN_FINISHED, PLAN_FAILED, [](basicCounter* i, ai_blobs* db, const ai_prefs& prf){std::cout << "final test" << std::endl;  return *i->intref == 5; }) {};
};


void initTestPlans() {
	static bool ran = false;
	if (!ran) {
		{
			std::shared_ptr<Plan<intholder>> ptr = std::make_shared <Plan<intholder>>(TEST_PLAN_0);
			ptr->steps.emplace_back(std::make_unique<simpleIntStep>(1));
			ptr->steps.emplace_back(std::make_unique<simpleIntStep>(PLAN_FINISHED));
			ptr->goalsatisfaction[TEST_GOAL_0] = -1.0;
			PlanBase::idsmap.push_back(ptr);
		}
		{
			std::shared_ptr<Plan<basicCounter>> ptr = std::make_shared <Plan<basicCounter>>(TEST_PLAN_1_TESTER);
			ptr->steps.emplace_back(std::make_unique<tryValues>());
			ptr->steps.emplace_back(std::make_unique<testfor5B>());
			ptr->goalsatisfaction[TEST_GOAL_FIND_NUM] = 1.0;
			PlanBase::idsmap.push_back(ptr);
		}
		{
			std::shared_ptr<Plan<intandrefcounter>> ptr = std::make_shared <Plan<intandrefcounter>>(TEST_PLAN_2_TEST);
			ptr->steps.emplace_back(std::make_unique<testfor5>());
			PlanBase::idsmap.push_back(ptr);
		}

		{
			std::shared_ptr<Plan<glcounter>> ptr = std::make_shared <Plan<glcounter>>(TEST_PLAN_3_GOALDISP);
			ptr->steps.emplace_back(std::make_unique<goaldispatch>());
			PlanBase::idsmap.push_back(ptr);
		}

		{
			std::shared_ptr<Plan<gltestprm>> ptr = std::make_shared <Plan<gltestprm>>(TEST_PLAN_4_TST1);
			ptr->steps.emplace_back(std::make_unique<glTestN<2>>());
			ptr->goalsatisfaction[TEST_GOAL_DISP_TEST] = 4.0;
			PlanBase::idsmap.push_back(ptr);
		}

		{
			std::shared_ptr<Plan<gltestprm>> ptr = std::make_shared <Plan<gltestprm>>(TEST_PLAN_5_TST2);
			ptr->steps.emplace_back(std::make_unique<glTestN<3>>());
			ptr->goalsatisfaction[TEST_GOAL_DISP_TEST] = 2.0;
			PlanBase::idsmap.push_back(ptr);
		}

		{
			std::shared_ptr<Plan<gltestprm>> ptr = std::make_shared <Plan<gltestprm>>(TEST_PLAN_6_TST3);
			ptr->steps.emplace_back(std::make_unique<glTestN<4>>());
			ptr->goalsatisfaction[TEST_GOAL_DISP_TEST] = 3.0;
			PlanBase::idsmap.push_back(ptr);
		}

		{
			std::shared_ptr<Plan<gltestprm>> ptr = std::make_shared <Plan<gltestprm>>(TEST_PLAN_7_TST4);
			ptr->steps.emplace_back(std::make_unique<glTestN<5>>());
			ptr->goalsatisfaction[TEST_GOAL_DISP_TEST] = 1.0;
			PlanBase::idsmap.push_back(ptr);
		}

		{
			std::shared_ptr<Plan<pbcounter>> ptr = std::make_shared <Plan<pbcounter>>(TEST_PLAN_8_PBDISP);
			ptr->steps.emplace_back(std::make_unique<trypbValues>());
			PlanBase::idsmap.push_back(ptr);
		}

		{
			std::shared_ptr<Plan<tptestprm>> ptr = std::make_shared <Plan<tptestprm>>(TEST_PLAN_9_PBTSTS);
			ptr->steps.emplace_back(std::make_unique<pbTestN<6>>());
			PlanBase::idsmap.push_back(ptr);
		}

		PlanBase::plansforgoal.resize(TEST_GOAL_DISP_TEST +1);
		PlanBase::plansforgoal[TEST_GOAL_DISP_TEST].push_back(TEST_PLAN_4_TST1);
		PlanBase::plansforgoal[TEST_GOAL_DISP_TEST].push_back(TEST_PLAN_5_TST2);
		PlanBase::plansforgoal[TEST_GOAL_DISP_TEST].push_back(TEST_PLAN_6_TST3);
		PlanBase::plansforgoal[TEST_GOAL_DISP_TEST].push_back(TEST_PLAN_7_TST4);
		ran = true;
	}
}