#pragma once
#include "globalhelpers.h"

template <int k_value, typename T>
inline T soft_max(const T x, const T y) {
	const T maximum = T(k_value) * std::max(x, y);
	const T minimum = T(k_value) * std::min(x, y);
	return maximum + log1p(exp(minimum - maximum));
}

template <int k_value, typename T>
inline T soft_min(const T x, const T y) {
	const T minimum = T(k_value) * std::min(x, y);
	const T maximum = T(k_value) * std::max(x, y);
	return minimum - log1p(exp(minimum - maximum));
}

inline double nan_guard_min_pos(const double inv) {
	return std::isfinite(inv) ? inv : std::numeric_limits<double>::min();
}

inline double nan_guard_min_neg(const double inv) {
	return std::isfinite(inv) ? inv : -std::numeric_limits<double>::min();
}

inline auto military_contest(double v_success, double v_failure, double force_cost, double e_force_quanity) {
	constexpr double k_value = 4.0;
	return [=](const IN_P(double) var) {// var[0] = force_quantity, var[1] = money
		const auto mv = soft_min<4>(var[0], var[1] / force_cost) / k_value;
		return (-v_success * mv + -v_failure * e_force_quanity) / (mv + e_force_quanity);
	};
}

inline auto military_contest_deriv(double v_success, double v_failure, double force_cost, double e_force_quanity) {
	constexpr double k_value = 4.0;
	return [=](const IN_P(double) var, const IN_P(double) direction) { // var[0] = force_quantity, var[1] = money
		const auto mv = soft_min<4>(var[0], var[1] / force_cost) / k_value;
		const auto common_term = -e_force_quanity * (v_success - v_failure) / pow(mv - e_force_quanity, 2.0);
		const auto common_difference = k_value * (var[1] / force_cost - var[0]);
		const auto deriv_1 = nan_guard_min_neg(common_term / (1 + exp(-common_difference)));
		const auto deriv_2 = nan_guard_min_neg(common_term / (force_cost * (1 + exp(common_difference))));
		return direction[0] * deriv_1 + direction[1] * deriv_2;
	};
}

inline auto military_contest_combined(double v_success, double v_failure, double force_cost, double e_force_quanity) {
	constexpr double k_value = 4.0;
	return [=](const IN_P(double) var, const IN_P(double) direction) { // var[0] = force_quantity, var[1] = money
		const auto mv = soft_min<4>(var[0], var[1] / force_cost) / k_value;
		const auto common_term = -e_force_quanity * (v_success - v_failure) / pow(mv - e_force_quanity, 2.0);
		const auto common_difference = k_value * (var[1] / force_cost - var[0]);
		const auto deriv_1 = nan_guard_min_neg(common_term / (1 + exp(-common_difference)));
		const auto deriv_2 = nan_guard_min_neg(common_term / (force_cost * (1 + exp(common_difference))));
		return std::make_pair((-v_success * mv + -v_failure * e_force_quanity) / (mv + e_force_quanity),
			direction[0] * deriv_1 + direction[1] * deriv_2);
	};
}

inline auto military_contest_set(double v_success, double v_failure, double force_cost, double e_force_quanity) {
	return std::make_tuple(military_contest(v_success, v_failure, force_cost, e_force_quanity),
		military_contest_deriv(v_success, v_failure, force_cost, e_force_quanity),
		military_contest_combined(v_success, v_failure, force_cost, e_force_quanity));
}

inline auto military_simple_contest_set(double v_success, double v_failure, double force_cost, double e_force_quanity) {
	return std::make_tuple([=](const IN_P(double) var) {
			const auto fq = var[0] / force_cost;
			const auto tfq = e_force_quanity + fq;
			return (-v_success * fq + -v_failure * e_force_quanity) / tfq;
		}, [=](const IN_P(double) var, const IN_P(double) direction) {
			const auto fq = var[0] / force_cost;
			const auto tfq = e_force_quanity + fq;
			return (v_failure - v_success) * e_force_quanity * direction[0] / (tfq*tfq);
		}, [=](const IN_P(double) var, const IN_P(double) direction) {
			const auto fq = var[0] / force_cost;
			const auto tfq = e_force_quanity + fq;
			return std::make_pair((-v_success * fq + -v_failure * e_force_quanity) / tfq, (v_failure - v_success) * e_force_quanity * direction[0] / (tfq*tfq));
		});
}

inline auto voting_contest(double v_success, double v_failure, double num_total, double num_negative, double num_positive, double num_player) {
	constexpr double rt2 = 1.4142135623730950488016887242097;

	const auto remainder = num_total - num_negative - num_positive - num_player;
	const auto inner1 = sqrt(num_negative * num_positive * remainder);
	const auto erf1 = erf(inner1 / (num_negative * rt2));
	const auto erf2 = erf(inner1 / (num_positive * rt2));
	const auto denom = erf1 + erf2;

	return [=](const IN_P(double) var) {
		const auto num = 2.0 * num_player * num_positive - 2.0 * var[0] * (num_negative + num_positive) + num_negative * num_total - num_positive *num_total;
		return (-v_success * erf1 +  -v_failure * erf2 +  (v_success - v_failure) * erf(num / (2.0 * rt2 * inner1))) / denom;
	};
}

inline auto voting_contest_deriv(double v_success, double v_failure, double num_total, double num_negative, double num_positive, double num_player) {
	constexpr double rt2 = 1.4142135623730950488016887242097;
	constexpr double rt_2_pi = 0.79788456080286535587989211986876;

	const auto remainder = num_total - num_negative - num_positive - num_player;
	const auto inner2 = num_negative * num_positive * remainder;
	const auto inner1 = sqrt(inner2);
	const auto erf1 = erf(inner1 / (num_negative * rt2));
	const auto erf2 = erf(inner1 / (num_positive * rt2));
	const auto denom = erf1 + erf2;

	return [=](const IN_P(double) var, const IN_P(double) direction) {
		const auto num = 2.0 * num_player * num_positive - 2.0 * var[0] * (num_negative + num_positive) + num_negative * num_total - num_positive *num_total;
		return ((exp(num / (-8.0 * inner2)) * rt_2_pi * (v_failure - v_success) * (num_positive + num_negative)) / (inner1 * denom)) * direction[0];
	};
}

inline auto voting_contest_combined(double v_success, double v_failure, double num_total, double num_negative, double num_positive, double num_player) {
	constexpr double rt2 = 1.4142135623730950488016887242097;
	constexpr double rt_2_pi = 0.79788456080286535587989211986876;

	const auto remainder = num_total - num_negative - num_positive - num_player;
	const auto inner2 = num_negative * num_positive * remainder;
	const auto inner1 = sqrt(inner2);
	const auto erf1 = erf(inner1 / (num_negative * rt2));
	const auto erf2 = erf(inner1 / (num_positive * rt2));
	const auto denom = erf1 + erf2;

	return [=](const IN_P(double) var, const IN_P(double) direction) {
		const auto num = 2.0 * num_player * num_positive - 2.0 * var[0] * (num_negative + num_positive) + num_negative * num_total - num_positive *num_total;
		return std::make_pair((-v_success * erf1 + -v_failure * erf2 + (v_success - v_failure) * erf(num / (2.0 * rt2 * inner1))) / denom,
			((exp(num / (-8.0 * inner2)) * rt_2_pi * (v_failure - v_success) * (num_positive + num_negative)) / (inner1 * denom)) * direction[0]);
	};
}

inline auto voting_contest_set(double v_success, double v_failure, double num_total, double num_negative, double num_positive, double num_player) {
	constexpr double rt2 = 1.4142135623730950488016887242097;
	constexpr double rt_2_pi = 0.79788456080286535587989211986876;

	const auto remainder = num_total - num_negative - num_positive - num_player;
	const auto inner2 = num_negative * num_positive * remainder;
	const auto inner1 = sqrt(inner2);
	const auto erf1 = erf(inner1 / (num_negative * rt2));
	const auto erf2 = erf(inner1 / (num_positive * rt2));
	const auto denom = erf1 + erf2;

	return std::make_tuple([=](const IN_P(double) var) {
		const auto num = 2.0 * num_player * num_positive - 2.0 * var[0] * (num_negative + num_positive) + num_negative * num_total - num_positive *num_total;
		return (-v_success * erf1 + -v_failure * erf2 + (v_success - v_failure) * erf(num / (2.0 * rt2 * inner1))) / denom;
	}, [=](const IN_P(double) var, const IN_P(double) direction) {
		const auto num = 2.0 * num_player * num_positive - 2.0 * var[0] * (num_negative + num_positive) + num_negative * num_total - num_positive *num_total;
		return ((exp(num / (-8.0 * inner2)) * rt_2_pi * (v_failure - v_success) * (num_positive + num_negative)) / (inner1 * denom)) * direction[0];
	}, [=](const IN_P(double) var, const IN_P(double) direction) {
		const auto num = 2.0 * num_player * num_positive - 2.0 * var[0] * (num_negative + num_positive) + num_negative * num_total - num_positive *num_total;
		return std::make_pair((-v_success * erf1 + -v_failure * erf2 + (v_success - v_failure) * erf(num / (2.0 * rt2 * inner1))) / denom,
			((exp(num / (-8.0 * inner2)) * rt_2_pi * (v_failure - v_success) * (num_positive + num_negative)) / (inner1 * denom)) * direction[0]);
	});
}

inline auto saving_valuation_set(double factor) {
	return std::make_tuple([=](const IN_P(double) var) { return -factor * var[0]; },
		[=](const IN_P(double) var, const IN_P(double) direction) { return -factor * direction[0]; },
		[=](const IN_P(double) var, const IN_P(double) direction) { return std::make_pair(-factor * var[0], -factor * direction[0]); });
}