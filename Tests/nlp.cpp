﻿#include "globalhelpers.h"
#include "nlp.h"

#ifdef SAFETY_OFF
#define EIGEN_NO_DEBUG
#else
#define EIGEN_NO_MALLOC
#endif

#define EIGEN_MPL2_ONLY

#include "Eigen"


using value_type = float;
using e_vector = Eigen::Matrix<value_type, Eigen::Dynamic, 1>;
using e_row_vector = Eigen::Matrix<value_type, 1, Eigen::Dynamic>;
using e_matrix = Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic>;

template<typename VT, typename MT>
void vector_times_ut_inverse(IN(VT) vector, IN(MT) coeff, INOUT(VT) result) {
#ifndef SAFETY_OFF
	if (!result.isZero()) {
		abort(); // result must start zeroed out
	}
#endif
	const auto sz = coeff.rows();
	for (Eigen::Index i = 0; i < sz; ++i) {
		result(i) = (vector(i) - (result * coeff.col(i))(0, 0)) / (coeff(i, i));
	}
}

template<typename VT, typename MT>
void ut_inverse_times_vector(IN(MT) coeff, IN(VT) vector, INOUT(VT) result) {
#ifndef SAFETY_OFF
	if (!result.isZero()) {
		abort(); // result must start zeroed out
	}
#endif
	const auto sz = coeff.rows();
	for (Eigen::Index i = sz - 1; i >= 0; --i) {
		result(i) = (vector(i) - (coeff.row(i).head(sz) * result)(0, 0)) / (coeff(i, i));
	}
}

template<typename RVT, typename MT>
void caclulate_reduced_n_gradient(IN(MT) coeff, IN(RVT) gradient, INOUT(RVT) n_result) {
	const auto rows = coeff.rows();
	const auto remainder = coeff.cols() - rows;
	
	Eigen::Map<e_vector, Eigen::Aligned> intermediate((value_type*)_alloca(rows * sizeof(value_type)), rows);
	intermediate.noalias() = Eigen::VectorXf::Zero(rows);

	vector_times_ut_inverse(gradient.head(rows), coeff, intermediate);
	n_result.noalias() = gradient.tail(remainder) - (intermediate * coeff.rightCols(coeff.cols() - rows));
}

template<typename VT, typename MT>
void calcuate_b_direction(IN(MT) coeff, INOUT(VT) direction) {
	const auto rows = coeff.rows();
	const auto remainder = coeff.cols() - rows;

	Eigen::Map<e_vector, Eigen::Aligned> n_times_d((value_type*)_alloca(rows * sizeof(value_type)), rows);
	n_times_d.noalias() = coeff.rightCols(remainder) * direction.tail(remainder);

	Eigen::Map<e_vector, Eigen::Aligned> intermediate((value_type*)_alloca(rows * sizeof(value_type)), rows);
	intermediate.noalias() = Eigen::VectorXf::Zero(rows);

	ut_inverse_times_vector(coeff, n_times_d, intermediate);
	direction.head(rows).noalias() = -intermediate;
}

struct var_mapping {
	value_type current_value;
	unsigned short mapped_index;
	unsigned short rank;
};

template<typename RVT, typename VT>
void calcuate_n_direction_steepest(const Eigen::Index b_size, const Eigen::Index remainder, IN(std::vector<var_mapping>) variable_mapping, IN(RVT) reduced_n_gradient, INOUT(VT) direction) {
	direction.tail(remainder).noalias() = -(reduced_n_gradient.transpose());

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		if (n_direction(mi) < value_type(0.0) && variable_mapping[i].current_value == value_type(0.0)) {
			direction(mi) = value_type(0.0);
		}
	}
}

template<typename RVT, typename VT>
void calcuate_n_direction_mokhtar(const Eigen::Index b_size, const Eigen::Index remainder, IN(std::vector<var_mapping>) variable_mapping, IN(RVT) reduced_n_gradient, INOUT(VT) direction) {
	direction.tail(remainder).noalias() = -(reduced_n_gradient.transpose());

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		if (direction(mi) < value_type(0.0)) {
			direction(mi) *= variable_mapping[i].current_value;
		}
	}
}

template<typename function_class, typename RVT>
void calculate_raw_gradient(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(RVT) gradient) {
	function.get_raw_gradient_at(variable_mapping, gradient);
}

template<typename function_class, typename RVT>
void calculate_simple_gradient(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(RVT) gradient) {
	const auto sz = variable_mapping.size();
	IN_P(float) gvals = (value_type*)_malloca(sizeof(value_type) * sz);

	function.get_gradient_at(variable_mapping, gvals);

	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		gradient(mi) = gvals[i];
	}

	_freea(gvals);
}

template<typename RVT>
using cg_function_type = value_type(*)(IN(RVT), IN(RVT), IN(RVT));

template<typename RVT>
value_type cg_HS(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
	return (current_gradient * (current_gradient - prev_gradient).transpose())(0, 0) / (-prev_cg * (current_gradient - prev_gradient).transpose())(0, 0);
}

template<typename RVT>
value_type cg_HZ(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
	constexpr value_type η = 0.01; // range (0, inf) used in the lower bound for β

	const auto dty = value_type(1.0) / (-prev_cg * (current_gradient - prev_gradient).transpose())(0, 0));
	const auto β = dty * ((current_gradient - prev_gradient - value_type(2.0) * (current_gradient - prev_gradient).squaredNorm() * dty * -prev_cg) * current_gradient.transpose())(0, 0);
	return std::max(β, value_type (-1.0) / (prev_cg.norm() * std::min(η, prev_gradient.norm())));
}

template<typename RVT>
value_type cg_PR(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
	return (current_gradient * (current_gradient - prev_gradient).transpose())(0, 0) / (prev_gradient.squaredNorm());
}

template<typename RVT>
value_type cg_PR_plus(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
	return std::max((current_gradient * (current_gradient - prev_gradient).transpose())(0, 0) / (prev_gradient.squaredNorm()), value_type(0.0));
}

template<typename RVT>
value_type cg_PR(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
	return (current_gradient.squaredNorm()) / (prev_gradient.squaredNorm());
}

template<typename RVT>
using restart_conditions_type = bool(*)(size_t, size_t, IN(RVT), IN(RVT), IN(RVT));

template<typename function_class, typename RVT, cg_function_type<RVT> cg_varient, restart_conditions_type<RVT> restart_test>
bool calculate_cg_gradient(IN(function_class) function, IN(std::vector<var_mapping>) variable_mapping, size_t restart_count, INOUT(RVT) previous_gvals, INOUT(RVT) prev_cg, INOUT(RVT) gradient) {
	const auto sz = variable_mapping.size();
	IN_P(value_type) gvals = (value_type*)_malloca(sizeof(value_type) * sz);

	function.get_gradient_at(variable_mapping, gvals);
	Eigen::Map<e_row_vector, Eigen::Aligned> c_gvals(gvals, sz);

	Eigen::Map<e_row_vector, Eigen::Aligned> new_cg((value_type*)_alloca(sizeof(value_type) * sz), sz);
	new_cg.noalias() = c_gvals + cg_varient(c_gvals, previous_gvals, prev_cg) * prev_cg;

	//gradient = e_row_vector::Zero(sz);

	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		gradient(mi) = new_cg(i);
	}

	const bool restart = restart_test(restart_count, sz, prev_cg, previous_gvals, new_cg);

	prev_cg.noalias() = new_cg;
	previous_gvals.noalias() = c_gvals;

	_freea(gvals);

	return restart;
}

template<typename VT>
std::pair<value_type, short> max_lambda(INOUT(std::vector<var_mapping>) variable_mapping, INOUT(VT) direction) {
	short index = -1;
	value_type least = max_value<value_type>::value;

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++sz) {
		const auto indx = variable_mapping[i].mapped_index;
		if (direction(indx) < value_type(0.0)) {
			const auto lm = -variable_mapping[i].current_value / direction(indx);
			if (lm < least) {
				least = lm;
				index = static_cast<short>(i);
			}
		}
	}

	return std::make_pair(least, index);
}

template<typename replacement_selection, typename MT>
void swap_base(INOUT(MT) coeff, INOUT(std::vector<var_mapping>) variable_mapping, IN(std::vector<unsigned short>) rank_starts, const Eigen::Index to_replace) {
	const auto first_of_rank = rank_starts[to_replace];
	const auto last_of_rank = rank_starts[to_replace + 1];

	size_t selected_replacement = replacement_selection::pick_best(variable_mapping, first_of_rank, last_of_rank);
	const auto replacement_index = variable_mapping[selected_replacement].mapped_index;
	coeff.col(to_replace).swap(coeff.col(replacement_index));

	for (size_t i = rank_range.first; i != rank_range.second; ++i) {
		if (variable_mapping[i].mapped_index == to_replace) {
			variable_mapping[i].mapped_index = replacement_index;
			break;
		}
	}
	variable_mapping[selected_replacement].mapped_index = to_replace;
}

template<typename MT>
void maximize_basis(INOUT(MT) coeff, INOUT(std::vector<var_mapping>) variable_mapping, IN(std::vector<unsigned short>) rank_starts) {
	const auto last_rank = rank_starts.size() - 1;
	unsigned short current_rank = 0;
	unsigned short rank_end = rank_starts[1];
	unsigned short largest = 0;
	value_type largest_value = value_type(0.0);
	unsigned short basis = 0;

	for (size_t current_position = 0; true; ++current_position) {
		if (current_position == rank_end) {
			if (largest != basis) {
				const auto largest_index = variable_mapping[largest].mapped_index;
				coeff.col(current_rank).swap(coeff.col(largest_index));
				variable_mapping[largest].mapped_index = current_rank;
				variable_mapping[basis].mapped_index = largest_index;
			}

			++current_rank;

			if (current_rank >= last_rank)
				break;

			rank_end = rank_starts[current_rank+1];
			largest = current_position;
			largest_value = value_type(0.0);
		}
		if (variable_mapping[current_position].mapped_index == current_rank) {
			basis = current_position;
		}
		if (variable_mapping[current_position].current_value > largest_value) {
			largest_value = variable_mapping[current_position].current_value;
			largest = current_position;
		}
	}
}

class general_function {
public:
	template<typename RVT>
	value_type evaluate_at(INOUT(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(RVT) direction) const;
	template<typename RVT>
	std::pair<value_type, value_type> evaluate_at_with_derivative(INOUT(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(RVT) direction) const;
	void get_gradient_at(INOUT(std::vector<var_mapping>) variable_mapping, IN_P(value_type) gradient_out) const;
};

struct limiting_value {
	value_type at;
	value_type φ;
	value_type φ_prime;
	limiting_value() : at(0.0), φ(0.0), φ_prime(0.0) {};
	limiting_value(value_type a, value_type b, value_type c) : at(a), φ(b), φ_prime(c) {};
	limiting_value(value_type a, IN(std::pair<value_type, value_type>) b) : at(a), φ(b.first), φ_prime(b.second) {};
};

template<typename function_class, typename RVT>
std::pair<limiting_value, limiting_value> hager_zhang_interval_update(IN(function_class) function, IN(std::vector<var_mapping>) variable_mapping, IN(RVT) direction, IN(limiting_value) a, IN(limiting_value) b, IN(limiting_value) c, const value_type φ_zero, const value_type ϵ_k) {
	constexpr value_type θ = value_type(0.5); // range (0, 1) used in the update rules when the potential intervals [a,c] or [c, b] violate the opposite slope condition
	
	if (c.at < a.at || c.at > b.at)
		return std::make_pair(a, b);
	if(c.φ_prime >= value_type(0.0))
		return std::make_pair(a, c);
	if(c.φ <= φ_zero + ϵ_k)
		return std::make_pair(c, b);

	auto a_temp = a;
	auto b_temp = c;

	while (true) {
		const auto d_at = (value_type(1.0) - θ) * (a_temp.at + θ * b_temp.at);
		const limiting_value d(d_at, function.evaluate_at_with_derivative(variable_mapping, d_at, direction));

		if (d.φ_prime >= value_type(0.0))
			return std::make_pair(a_temp, d);
		if (d.φ <= φ_zero + ϵ_k)
			a_temp = d;
		else
			b_temp = d;
	}
}

value_type secant(IN(limiting_value) a, IN(limiting_value) b) {
	return b.φ_prime != a.φ_prime ? (a.at * b.φ_prime - b.at * a.φ_prime) / (b.φ_prime - a.φ_prime) : max_value<value_type>::value;
}

template<typename function_class, typename RVT>
std::pair<limiting_value, limiting_value> hager_zhang_secant_step(IN(function_class) function, IN(std::vector<var_mapping>) variable_mapping, IN(RVT) direction, IN(limiting_value) a, IN(limiting_value) b, const value_type φ_zero, const value_type ϵ_k) {
	const auto secant_at = secant(a, b);
	const limiting_value c(secant_at, function.evaluate_at_with_derivative(variable_mapping, secant_at, direction));
	const auto intermediate_interval = hager_zhang_interval_update(function, variable_mapping, direction, a, b, c, φ_zero, ϵ_k);

	if (c.at != intermediate_interval.first.at && c.at != intermediate_interval.second.at) {
		return intermediate_interval;
	} else {
		const auto c_new_at = (c.at == intermediate_interval.first.at) ? secant(a, intermediate_interval.first) : secant(b, intermediate_interval.second);
		const limiting_value c_new(c_new_at, function.evaluate_at_with_derivative(variable_mapping, c_new_at, direction));
		return hager_zhang_interval_update(function, variable_mapping, direction, a, b, c_new, φ_zero, ϵ_k);
	}
}

template<int δ_times_1000, int σ_times_1000>
bool satisfies_hager_zhang_conditions(IN(limiting_value) v, const value_type φ_zero, const value_type φ_prime_zero, const value_type ϵ_k) {
	constexpr value_type δ = static_cast<value_type>(δ_times_1000) / value_type(1000.0); // range (0, 0.5)  used in the Wolfe conditions
	constexpr value_type σ = static_cast<value_type>(σ_times_1000) / value_type(1000.0); // range [δ, 1)  used in the Wolfe conditions

	static_assert(σ_times_1000 > δ_times_1000, "σ must be > than δ");

	return (v.φ - φ_zero <= δ * v.at * v.φ_prime && v.φ_prime >= σ * φ_prime_zero)   // wolfe condition 2.2: f(x_k + α_k * d_k ) − f(x _k) ≤ δ * α_k * g_k * d_k &&  wolfe condition 2.3: g_k+1*d_k >= σ*g_k*d_k
		|| ((value_type(2.0) * δ - value_type(1.0)) * φ_prime_zero >= v.φ_prime && v.φ_prime >= σ * φ_prime_zero && v.φ <= φ_zero + ϵ_k); // approximate wolf condition 4.1: (2δ − 1) * φ_prime(0) ≥ φ_prime(α_k) ≥ σ * φ_prime(0)
}

template<typename function_class, typename RVT>
void update_with_hager_zhang_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type deriv, IN(RVT) direction) {
	
	constexpr value_type ϵ = value_type(0.000001); // range [0, inf) used in the approximate Wolfe conditions
	constexpr value_type γ = value_type(0.66); // range (0, 1) determines when a bisection step is performed
	
	// constexpr value_type η = 0.01; // range (0, inf) used in the lower bound for β

	limiting_value a_k(value_type(0.0), function.evaluate_at_with_derivative(variable_mapping, value_type(0.0), direction));
	const auto φ_zero = a_k.φ;
	const auto φ_prime_zero = a_k.φ_prime;
	limiting_value b_k(max_lambda, function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction));

	do { 
		ϵ_k = ϵ * abs(b_k.φ);

		const auto interval = hager_zhang_secant_step(function, variable_mapping, direction, a_k, b_k, φ_zero, ϵ_k);

		if (interval.second.at - interval.first.at > γ * (b_k.at - a_k.at)) {
			const auto c_val = (interval.first.at + interval.second.at) / value_type(2.0);
			const limiting_value c(c_val, function.evaluate_at_with_derivative(variable_mapping, c_val, direction));
			std::tie(a_k, b_k) = hager_zhang_interval_update(function, variable_mapping, direction, interval.first, interval.second, c, φ_zero, ϵ_k);
		} else {
			std::tie(a_k, b_k) = interval;
		}

	} while(!satisfies_hager_zhang_conditions<100,900>(b_k, φ_zero, φ_prime_zero, ϵ_k)); // conditions not satisfied 

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * b_k.at;
	}
	if (b_k.at == max_lambda)
		variable_mapping[max_index].mapped_index = value_type(0.0);
	return;
}

template<typename function_class, typename RVT>
void update_with_derivative_interpolation_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type deriv, IN(RVT) direction) {
	const value_type φ_a0 = function.evaluate_at(variable_mapping, value_type(0.0), direction);
	const value_type φ_prime_a0 = deriv;

	constexpr value_type δ = value_type(0.001);

	limiting_value a_k(max_lambda, function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction));

	if (a_k.φ - φ_a0 <= (δ * max_lambda * φ_prime_a0)) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * max_lambda;
		}
		variable_mapping[max_index].current_value = value_type(0.0);
		return;
	}

	auto a_last = a_k;

	//Quadratic interpolation
	{
		const value_type a_new_at = std::min(value_type(-0.5) * φ_prime_a0 * max_lambda * max_lambda / (a_k.φ - φ_a0 - φ_prime_a0 * max_lambda), max_lambda);
		a_k = limiting_value(a_new_at, function.evaluate_at_with_derivative(variable_mapping, a_new_at, direction));
	}

	//v.φ - φ_zero <= δ * v.at * v.φ_prime
	if (a_k.φ - φ_a0 <= (δ * a_new * φ_prime_a0)) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_k.at;
		}
		return;
	}

	while (true) {
		const value_type d_1 = a_last.φ_prime + a_k.φ_prime - value_type(3) * (a_last.φ - a_k.φ) / (a_last.at - a_k.at);
		const value_type d_2 = (a_k.at > a_last.at) ? sqrt(d_1*d_1 - a_last.φ_prime * a_k.φ_prime) : -sqrt(d_1*d_1 - a_last.φ_prime * a_k.φ_prime);

		const value_type a_new_at = a_k.at - (a_k.at - a_last.at) * ((a_k.φ_prime + d_2 - d_1) / (a_k.φ_prime - a_last.φ_prime + value_type(2) * d_2));
		const auto a_new = limiting_value(a_new_at, function.evaluate_at_with_derivative(variable_mapping, a_new_at, direction));

		if (a_new.φ - φ_a0 <= (δ * a_k.at * φ_prime_a0)) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_new_at;
			}
			return;
		}

		a_last = a_k;
		if (a_k.at - a_new_at > value_type(0.6) * a_k.at || a_k.at - a_new_at < value_type(0.1) * a_k.at) {
			const value_type a_new_at_b = value_type(0.5) * a_k.at;
			a_k = limiting_value(a_new_at_b, function.evaluate_at_with_derivative(variable_mapping, a_new_at_b, direction));
		} else {
			a_k = a_new;
		}

	}
}

template<typename function_class, typename RVT>
void update_with_interpolation_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type deriv, IN(RVT) direction) {
	const value_type φ_a0 = function.evaluate_at(variable_mapping, value_type(0.0), direction);
	const value_type φ_prime_a0 = deriv;

	constexpr value_type δ = value_type(0.001);
	
	value_type a_k = max_lambda;
	value_type φ_a_k = function.evaluate_at(variable_mapping, max_lambda, direction);

	if (a_phi - φ_a0 <=  δ * a_k * φ_prime_a0) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * max_lambda;
		}
		variable_mapping[max_index].current_value = value_type(0.0);
		return;
	}

	value_type a_last = a_k;
	value_type φ_a_last = φ_a_k;

	//Quadratic interpolation
	a_k = value_type(-0.5) * φ_prime_a0 * a_k * a_k / (φ_a_k - φ_a0 - φ_prime_a0 * a_k);
	φ_a_k = function.evaluate_at(variable_mapping, a_k, direction);

	if (φ_a_k - φ_a0 <= + δ * a_k * φ_prime_a0) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_k;
		}
		return;
	}
	while (true) {
		const value_type multiplier = value_type(1.0) / (a_last * a_last * a_k * a_k * (a_k - a_last));
		const value_type m_1 = φ_a_k - φ_a0 - φ_prime_a0 * a_k;
		const value_type m_2 = φ_a_last - φ_a0 - φ_prime_a0 * a_last;

		const value_type a = multiplier * (a_last * a_last * m_1 - a_k * a_k * m_2);
		const value_type b = multiplier * (a_k * a_k * a_k * m_2 - a_last * a_last * a_last * m_1);

		const value_type a_new = -b + sqrt(b * b - value_type(3.0) * a * φ_prime_a0)) / (value_type(3.0) * a);
		const value_type φ_a_new = function.evaluate_at(variable_mapping, a_new, direction);

		if (φ_a_new - φ_a0 <= δ * a_k * φ_prime_a0) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_new;
			}
			return;
		}
		
		a_last = a_k;
		φ_a_last = φ_a_k;

		if (a_k - a_new_a > value_type(0.6) * a_k || a_k - a_new < value_type(0.1) * a_k) {
			a_k = 0.5 * a_k;
			φ_a_k = function.evaluate_at(variable_mapping, a_k, direction);
		} else {
			a_k = a_new_a;
			φ_a_k = a_new_phi;
		}
	}
}

template<typename function_class, typename RVT>
void update_with_backtrack_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type lambda, const short max_index, const value_type deriv, IN(RVT) direction) {
	constexpr value_type factor = value_type(0.5);

	const value_type base_value = function.evaluate_at(variable_mapping, value_type(0.0), direction);
	if ((function.evaluate_at(variable_mapping, lambda, direction) - base_value) <= lambda * deriv * factor) {
		// successful at max
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * lambda;
		}
		variable_mapping[max_index].current_value = value_type(0.0);
		return;
	} 

	while (true) {
		lambda /= value_type(2.0);

		if ((function.evaluate_at(variable_mapping, lambda, direction) - base_value) <= lambda * deriv * factor) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * lambda;
			}
			return;
		}
	}
}

template<typename function_class, typename RVT>
using ls_function_type = void(*)(IN(function_class), INOUT(std::vector<var_mapping>), value_type, const short, const value_type, IN(RVT));

template<typename RVT, typename VT>
using reduced_gradient_to_direction_map_type = void(*)(const Eigen::Index, const Eigen::Index, IN(std::vector<var_mapping>), IN(RVT), INOUT(VT));

template<typename function_class, typename MT,
	ls_function_type<function_class, Eigen::Map<e_row_vector, Eigen::Aligned>> LINE_SEARCH,
	reduced_gradient_to_direction_map_type<Eigen::Map<e_row_vector, Eigen::Aligned>, Eigen::Map<e_vector, Eigen::Aligned>> RG_MAP>
void steepest_descent(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(MT) coeff, IN(std::vector<unsigned short>) rank_starts) {
	const auto b_size = coeff.rows();
	const auto full = coeff.cols();
	const auto remainder = b_size - full;

	constexpr value_type ϵ = value_type(0.000001); // range [0, inf)

	Eigen::Map<e_row_vector, Eigen::Aligned> gradient((value_type*)_alloca(full * sizeof(value_type)), full);
	Eigen::Map<e_vector, Eigen::Aligned> direction((value_type*)_alloca(full * sizeof(value_type)), full);
	Eigen::Map<e_row_vector, Eigen::Aligned> reduced_n_gradient((value_type*)_alloca(remainder * sizeof(value_type)), remainder);

#ifndef SAFETY_OFF
	size_t iteration_count = 0;
#endif

	//inside iteration
	do {
		maximize_basis(coeff, variable_mapping, rank_starts);

		gradient.noalias() = e_row_vector::Zero(full);
		calculate_simple_gradient(function, variable_mapping, gradient);

		reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
		caclulate_reduced_n_gradient(coeff, gradient, n_result);

		direction.noalias() = e_vector::Zero(full);
		RG_MAP(b_size, remainder, variable_mapping, reduced_n_gradient, direction);
		calcuate_b_direction(coeff, direction);

		// test for satisfaction
		if (direction.squaredNorm() <= ϵ*ϵ)
			break;

		// perform line search, update solution
		const auto lm_max = max_lambda(variable_mapping, direction);

#ifndef SAFETY_OFF
		if (lm_max.second == -1)
			abort();
#endif
		const value_type m = (gradient * direction.transpose())(0,0);
		LINE_SEARCH(function, variable_mapping, lm_max.first, lm_max.second, m, direction);

#ifndef SAFETY_OFF
		if (++iteration_count > 100)
			abort();
#endif
	} while (true);
}



template<typename RVT>
bool basic_restart_condition(size_t restart_count, size_t variable_count, IN(RVT) previous_cg, IN(RVT) previous_gradient, IN(RVT) current_gradient) {
	return (restart_count >= variable_count);
}

template<typename RVT>
bool no_restart_condition(size_t restart_count, size_t variable_count, IN(RVT) previous_cg, IN(RVT) previous_gradient, IN(RVT) current_gradient) {
	return false;
}

template<typename RVT>
bool powell_restart_condition(size_t restart_count, size_t variable_count, IN(RVT) new_cg, IN(RVT) previous_gradient, IN(RVT) current_gradient) {
	const auto d_g = -((new_cg.transpose() * current_gradient)(0, 0));
	const auto current_sn = current_gradient.squaredNorm();
	return (restart_count >= variable_count)
		|| abs((previous_gradient.transpose() * current_gradient)(0,0)) >= value_type(0.2) * current_sn
		|| (restart_count >= 2 && value_type(-1.2) * current_sn <= d_g && d_g <= value_type(-0.8) * current_sn);
}

template<typename function_class, typename MT,
	ls_function_type<function_class, Eigen::Map<e_row_vector, Eigen::Aligned>> LINE_SEARCH,
	reduced_gradient_to_direction_map_type<Eigen::Map<e_row_vector, Eigen::Aligned>, Eigen::Map<e_vector, Eigen::Aligned>> RG_MAP,
	cg_function_type<Eigen::Map<e_row_vector, Eigen::Aligned>> CG_FUNCTION,
	restart_conditions_type<Eigen::Map<e_row_vector, Eigen::Aligned>> RESTART_FUNCTION>
	void conjugate_gradient_method(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(MT) coeff, IN(std::vector<unsigned short>) rank_starts) {

	const auto b_size = coeff.rows();
	const auto full = coeff.cols();
	const auto remainder = b_size - full;

	constexpr value_type ϵ = value_type(0.000001); // range [0, inf)

	Eigen::Map<e_row_vector, Eigen::Aligned> gradient((value_type*)_alloca(full * sizeof(value_type)), full);
	Eigen::Map<e_row_vector, Eigen::Aligned> previous_gvals((value_type*)_alloca(full * sizeof(value_type)), full);
	Eigen::Map<e_row_vector, Eigen::Aligned> prev_cg((value_type*)_alloca(full * sizeof(value_type)), full);

	Eigen::Map<e_vector, Eigen::Aligned> direction((value_type*)_alloca(full * sizeof(value_type)), full);
	Eigen::Map<e_row_vector, Eigen::Aligned> reduced_n_gradient((value_type*)_alloca(remainder * sizeof(value_type)), remainder);

#ifndef SAFETY_OFF
	size_t iteration_count = 0;
#endif

	bool after_reset = true;
	size_t restart_count = 0;

	//inside iteration
	do {
		maximize_basis(coeff, variable_mapping, rank_starts);

		gradient.noalias() = e_row_vector::Zero(full);


		if(after_reset) {
			calculate_simple_gradient(function, variable_mapping, previous_gvals);

			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				gradient(variable_mapping[i].mapped_index) = previous_gvals(i);
			}
			prev_cg.noalias() = previous_gvals;

			after_reset = false;
			restart_count = 0;
		} else {
			++restart_count;
			after_reset = calculate_cg_gradient<CG_FUNCTION, RESTART_FUNCTION>(function, variable_mapping, restart_count, previous_gvals, prev_cg, gradient);
		}
		
		reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
		caclulate_reduced_n_gradient(coeff, gradient, n_result);

		direction.noalias() = e_vector::Zero(full);
		RG_MAP(b_size, remainder, variable_mapping, reduced_n_gradient, direction);
		calcuate_b_direction(coeff, direction);

		// test for satisfaction
		if (direction.squaredNorm() <= ϵ*ϵ)
			break;

		// perform line search, update solution
		const auto lm_max = max_lambda(variable_mapping, direction);

#ifndef SAFETY_OFF
		if (lm_max.second == -1)
			abort();
#endif
		const value_type m = (gradient * direction.transpose())(0, 0);
		LINE_SEARCH(function, variable_mapping, lm_max.first, lm_max.second, m, direction);

#ifndef SAFETY_OFF
		if (++iteration_count > 100)
			abort();
#endif
	} while (true);
}