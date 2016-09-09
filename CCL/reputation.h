#pragma once
#include "globalhelpers.h"

namespace reputation {
	constexpr double p_attack_agressive = 1.0;
	constexpr double p_attack_nonagressive = 0.1;

	constexpr double p_noattack_agressive = 0.5;
	constexpr double p_noattack_nonagressive = 0.9;

	constexpr double p_honor_reliable = 0.9;
	constexpr double p_honor_unreliable = 0.5;

	constexpr double p_dishonor_reliable = 0.1;
	constexpr double p_dishonor_unreliable = 0.5;

	constexpr double p_plotting_reliable = 0.1;
	constexpr double p_plotting_unreliable = 0.9;

	constexpr double p_dishonor_favor_reliable = 0.1;
	constexpr double p_dishonor_favor_unreliable = 0.6;
};

double update_reputation(double p_accurate, double p_innaccurate, double prior) noexcept;
double drift_reputation(double prior) noexcept;