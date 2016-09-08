#pragma once
#include "globalhelpers.h"
#include "traits.h"
#include "structs.hpp"
#include "uielements.hpp"
#include "pacts.h"
#include "living_data.h"

constexpr double OPINION_THRESHOLD = 0.2;

void adjust_for_action_taken(char_id_t actor, IN(std::vector<std::pair<char_id_t, double>>) source, IN(w_lock) l) noexcept;
void build_concil_vec(INOUT(std::vector<std::pair<char_id_t,double>>) v, char_id_t actor, IN(g_lock) l) noexcept;
void build_concil_vec_a(INOUT(std::vector<std::pair<char_id_t, double>>) v, admin_id_t a, IN(g_lock) l) noexcept;
bool confirm_action(IN(std::vector<std::pair<char_id_t, double>>) v, IN(std::function<int(int, int, int, std::shared_ptr<uiElement>)>) description, INOUT(w_lock) l) noexcept;

bool attack_reaction(char_id_t attacker, char_id_t target, wargoal goal, INOUT(w_lock) l) noexcept;
double attack_opinion(char_id_t from, char_id_t attacker, char_id_t target, wargoal goal, IN(g_lock) l) noexcept;
double defensive_envoy_opinion(char_id_t from, char_id_t hos, IN(g_lock) l) noexcept;
double defensive_against_envoy_opinion(char_id_t from, char_id_t hos, char_id_t against, IN(g_lock) l) noexcept;
double non_aggression_envoy_opinion(char_id_t from, char_id_t hos, char_id_t with, IN(g_lock) l) noexcept;
double break_pact_opinion(char_id_t from, char_id_t hos, pact_id_t pid, IN(g_lock) l) noexcept;
double deception_plot_opinion(char_id_t from, char_id_t source, char_id_t target, IN(g_lock) l) noexcept;
double host_mass_event_opinion(char_id_t from, char_id_t host, unsigned int type, IN(g_lock) l) noexcept;

class pact_data;
bool pact_reaction_a(admin_id_t a, char_id_t actor, IN(pact_data) pact, INOUT(w_lock) l) noexcept;
bool dishonor_def_pact_reaction(char_id_t actor, char_id_t other, char_id_t attacker, pact_id_t pid, INOUT(w_lock) l) noexcept;