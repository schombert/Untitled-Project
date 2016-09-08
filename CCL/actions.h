#pragma once
#include "globalhelpers.h"
#include "peace.h"
#include "envoys.h"
#include "pacts.h"

class actionbase {
public:
	const char_id_t actor;
	actionbase(char_id_t c) noexcept : actor(c) {};
	virtual ~actionbase() {};
	virtual void execute() noexcept { abort(); };
	virtual bool possible(IN(g_lock) l) const noexcept { abort(); };
};

/*
class action_list_class {
private:
	__declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) SLIST_HEADER stack_head;

	template<typename T>
	struct __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) generic_node
	{
		SLIST_ENTRY slist_entry;
		T obj;
	};

public:
	action_list_class() {
		InitializeSListHead(&stack_head);
	}
	~action_list_class() {
		clear();
	}

	template<typename derived, typename ... P>
	void add_new(P&& ... params) noexcept {
		generic_node<derived>* __restrict pNode = (generic_node<derived>*)concurrent_aligned_malloc<MEMORY_ALLOCATION_ALIGNMENT>(sizeof(generic_node<derived>));
		new (static_cast<void*>(&pNode->obj)) derived(std::forward<P>(params)...);
		InterlockedPushEntrySList(&stack_head, &pNode->slist_entry);
	}

	void clear() noexcept {
		generic_node<size_t>* __restrict pNode = (generic_node<size_t>*)InterlockedPopEntrySList(&stack_head);
		while (pNode) {
			IN_P(actionbase) aptr = (actionbase*)(&pNode->obj);
			aptr->~actionbase();
			concurrent_aligned_free(pNode);
			pNode = (generic_node<size_t>*)InterlockedPopEntrySList(&stack_head);
		}
	}

	void execute_entries() noexcept {
		generic_node<size_t>* __restrict pNode = (generic_node<size_t>*)InterlockedPopEntrySList(&stack_head);
		fake_lock l;
		while (pNode) {
			IN_P(actionbase) aptr = (actionbase*)(&pNode->obj);
			if (aptr->possible(l)) {
				aptr->execute();
			}
			aptr->~actionbase();
			concurrent_aligned_free(pNode);
			pNode = (generic_node<size_t>*)InterlockedPopEntrySList(&stack_head);
		}
	}
};
*/


struct apply_actionbase {
	static void apply(INOUT(actionbase) act) {
		if (act.possible(fake_lock())) {
			act.execute();
		}
	}
};

namespace global {
	extern actionable_list_class<actionbase, apply_actionbase> actionlist;
};

class war_declaration : public actionbase {
public:
	const char_id_t target;
	const wargoal goal;
	war_declaration(char_id_t a, char_id_t t, wargoal g) noexcept : actionbase(a), target(t), goal(g) {}
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept;
};

class nocbwar : public war_declaration {
public:
	nocbwar(char_id_t d, char_id_t t) noexcept : war_declaration(d, t, make_wg_conquest()) {};
};

class dejurewar : public war_declaration {
public:
	dejurewar(char_id_t d, char_id_t t, title_id_t tf) noexcept : war_declaration(d, t, make_wg_dj(tf)) {};
};

class planned_event : public actionbase {
public:
	const unsigned int id;
	const char_id_t target;
	planned_event(char_id_t s, char_id_t t, unsigned int i) noexcept : id(i), target(t), actionbase(s) {};
	planned_event(char_id_t s, unsigned int i) noexcept : id(i), actionbase(s), target(0) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept;
};

class cancel_envoy_mission : public actionbase {
public:
	const admin_id_t associated_a;
	const unsigned char mission_number;
	cancel_envoy_mission(char_id_t s, admin_id_t a, unsigned char n) noexcept : actionbase(s), mission_number(n), associated_a(a) {};
	virtual void execute() noexcept;
	virtual bool possible( IN(g_lock) l) const noexcept { return true; };
};

class setup_envoy_mission : public actionbase {
public:
	const admin_id_t associated_a;
	concurrent_uniq<envoy_mission> newmission;
	setup_envoy_mission(char_id_t s, admin_id_t a, concurrent_uniq<envoy_mission> && m) noexcept :
		actionbase(s), associated_a(a), newmission(std::move(m)) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept;
};


class setup_spy_mission : public actionbase {
public:
	const char_id_t target;
	const char_id_t spy;
	const admin_id_t admsource;
	const unsigned char type;
	setup_spy_mission(char_id_t s, admin_id_t a, char_id_t t, char_id_t sp, unsigned char y) noexcept : actionbase(s), admsource(a), target(t), spy(sp), type(y) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept;
};

class cancel_spy_mission : public actionbase {
public:
	const admin_id_t admsource;
	const SM_TAG_TYPE mission_number;
	cancel_spy_mission(char_id_t s, admin_id_t a, SM_TAG_TYPE n) noexcept : actionbase(s), admsource(a), mission_number(n) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept { return true; };
};

class accept_envoy_offer : public actionbase {
public:
	const unsigned int offer_number;
	const admin_id_t associated_admin;
	const unsigned char mission_number;
	accept_envoy_offer(char_id_t s, admin_id_t a, unsigned char n, unsigned int o) noexcept : actionbase(s), associated_admin(a), mission_number(n), offer_number(o) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept { return true; };
};

class implement_peace_offer : public actionbase {
public:
	peace_deal deal;
	implement_peace_offer(char_id_t a, peace_deal &&d) noexcept : actionbase(a), deal(std::move(d)) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept { return true; };
};
void implement_peace_offer_fac(char_id_t, peace_deal&&) noexcept;

class relation_change : public actionbase {
public:
	const char_id_t secondary;
	const __int8 value;
	const bool symmetric;
	relation_change(char_id_t p, char_id_t s, __int8 v, bool sym) noexcept : actionbase(p), secondary(s), value(v), symmetric(sym) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept;
};

class adjust_front_allocation : public actionbase {
public:
	const war_id_t war_for;
	const unsigned short front;
	const float target;
	const bool attacker;
	adjust_front_allocation(char_id_t a, bool at, war_id_t w, unsigned short f, float t) noexcept : actionbase(a), front(f), attacker(at), war_for(w), target(t) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept { return true; };
};

class break_pact_a : public actionbase {
public:
	const pact_id_t pactid;
	break_pact_a(char_id_t p, pact_id_t pct) noexcept : actionbase(p), pactid(pct) {};
	virtual void execute() noexcept;
	virtual bool possible(IN(g_lock) l) const noexcept { return true; };
};

void transfer_prov_a(prov_id_t prov, admin_id_t to, INOUT(flat_set<char_id_t>) losers, IN(w_lock) l) noexcept;
void execute_actions() noexcept;