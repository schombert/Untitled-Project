#include "globalhelpers.h"
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

template<typename MT, typename RVT, typename VT>
void calcuate_n_direction_steepest(IN(MT) coeff, IN(std::vector<var_mapping>) variable_mapping, IN(RVT) reduced_n_gradient, INOUT(VT) direction) {
	const auto b_size = coeff.rows();
	const auto remainder = coeff.cols() - b_size;

	direction.tail(remainder).noalias() = -(reduced_n_gradient.transpose());

	
	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		if (n_direction(mi) < 0 && variable_mapping[i].current_value == 0.0f) {
			direction(mi) = 0.0f;
		}
	}
}

template<typename MT, typename RVT, typename VT>
void calcuate_n_direction_mokhtar(IN(MT) coeff, IN(std::vector<var_mapping>) variable_mapping, IN(RVT) reduced_n_gradient, INOUT(VT) direction) {
	const auto b_size = coeff.rows();
	const auto remainder = coeff.cols() - b_size;

	direction.tail(remainder).noalias() = -(reduced_n_gradient.transpose());

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		if (direction(mi) < 0) {
			direction(mi) *= variable_mapping[i].current_value;
		}
	}
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

class cg_HS {
public:
	template<typename RVT>
	static value_type alpha(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
		return (current_gradient * (current_gradient - prev_gradient).transpose())(0, 0) / (-prev_cg * (current_gradient - prev_gradient).transpose())(0, 0);
	}
};

class cg_HZ {
public:
	static constexpr value_type η = 0.01; // range (0, inf) used in the lower bound for β

	template<typename RVT>
	static value_type alpha(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
		const auto dty = 1.0 / (-prev_cg * (current_gradient - prev_gradient).transpose())(0, 0));
		const auto β = dty * ((current_gradient - prev_gradient - 2.0 * (current_gradient - prev_gradient).squaredNorm() * dty * -prev_cg) * current_gradient.transpose())(0, 0);
		return std::max(β, -1.0 / (prev_cg.norm() * std::min(η, prev_gradient.norm())));
	}
};

class cg_PR {
public:
	template<typename RVT>
	static value_type alpha(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
		return (current_gradient * (current_gradient - prev_gradient).transpose())(0, 0) / (prev_gradient.squaredNorm());
	}
};

class cg_FR {
public:
	template<typename RVT>
	static value_type alpha(IN(RVT) current_gradient, IN(RVT) prev_gradient, IN(RVT) prev_cg) {
		return (current_gradient.squaredNorm()) / (prev_gradient.squaredNorm());
	}
};

template<typename function_class, typename cg_varient, typename RVT>
void calculate_cg_gradient(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(RVT) previous_gvals, INOUT(RVT) prev_cg, INOUT(RVT) gradient) {
	const auto sz = variable_mapping.size();
	IN_P(float) gvals = (value_type*)_malloca(sizeof(value_type) * sz);

	function.get_gradient_at(variable_mapping, gvals);
	Eigen::Map<e_row_vector, Eigen::Aligned> c_gvals(gvals, sz);

	Eigen::Map<e_row_vector, Eigen::Aligned> new_cg((value_type*)_alloca(sizeof(value_type) * sz), sz);
	new_cg.noalias() = c_gvals + cg_varient::alpha(c_gvals, previous_gvals, prev_cg) * prev_cg;

	//gradient = e_row_vector::Zero(sz);

	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		gradient(mi) = new_cg(i);
	}

	prev_cg.noalias() = new_cg;
	previous_gvals.noalias() = c_gvals;

	_freea(gvals);
}

template<typename VT>
std::pair<value_type, short> max_lambda(INOUT(std::vector<var_mapping>) variable_mapping, INOUT(VT) direction) {
	short index = -1;
	value_type least = max_value<value_type>::value;

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++sz) {
		const auto indx = variable_mapping[i].mapped_index;
		if (direction(indx) < 0) {
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
	float largest_value = 0.0f;
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
			largest_value = 0.0f;
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
std::pair<limiting_value, limiting_value> hager_zhang_interval_update(IN(function_class) function, IN(std::vector<var_mapping>) variable_mapping, IN(RVT) direction, IN(limiting_value) a, IN(limiting_value) b, IN(limiting_value) c, const value_type φ_zero, const value_type ϵ_k, const value_type θ) {
	if (c.at < a.at || c.at > b.at)
		return std::make_pair(a, b);
	if(c.φ_prime >= 0.0)
		return std::make_pair(a, c);
	if(c.φ <= φ_zero + ϵ_k)
		return std::make_pair(c, b);

	auto a_temp = a;
	auto b_temp = c;

	while (true) {
		const auto d_at = (1.0 - θ) * (a_temp.at + θ * b_temp.at);
		const limiting_value d(d, function.evaluate_at_with_derivative(variable_mapping, d, direction));

		if (d.φ_prime >= 0) 
			return std::make_pair(a_temp, d);
		if (d.φ <= φ_zero + ϵ_k)
			a_temp = d;
		else
			b_temp = d;
	}
}

value_type secant(IN(limiting_value) a, IN(limiting_value) b) {
	return (a.at * b.φ_prime - b.at * a.φ_prime) / (b.φ_prime - a.φ_prime);
}

template<typename function_class, typename RVT>
std::pair<limiting_value, limiting_value> hager_zhang_secant_step(IN(function_class) function, IN(std::vector<var_mapping>) variable_mapping, IN(RVT) direction, IN(limiting_value) a, IN(limiting_value) b, const value_type φ_zero, const value_type ϵ_k, const value_type θ) {
	const auto secant_at = secant(a, b);
	const limiting_value c(secant_at, function.evaluate_at_with_derivative(variable_mapping, secant_at, direction));
	const std::pair<limiting_value, limiting_value> intermediate_interval = hager_zhang_interval_update(function, variable_mapping, direction, a, b, c, φ_zero, ϵ_k, θ);

	if (c.at != intermediate_interval.first.at && c.at != intermediate_interval.second.at) {
		return intermediate_interval;
	} else {
		const auto c_new_at = (c.at == intermediate_interval.first.at) ? secant(a, intermediate_interval.first) : secant(b, intermediate_interval.second);
		const limiting_value c_new(c_new_at, function.evaluate_at_with_derivative(variable_mapping, c_new_at, direction));
		return hager_zhang_interval_update(function, variable_mapping, direction, a, b, c_new, φ_zero, ϵ_k, θ);
	}
}

bool satisfies_hager_zhang_conditions(IN(limiting_value) v, const value_type φ_zero, const value_type φ_prime_zero, const value_type δ, const value_type σ, const value_type ϵ_k) {
	return (v.φ - φ_zero <= δ * v.at * v.φ_prime && v.φ_prime >= φ_prime_zero * σ)   // wolfe condition 2.2: f(x_k + α_k * d_k ) − f(x _k) ≤ δ * α_k * g_k * d_k &&  wolfe condition 2.3: g_k+1*d_k >= σ*g_k*d_k
		|| ((2.0 * δ - 1.0) * φ_prime_zero >= v.φ_prime && v.φ_prime >= σ * φ_prime_zero && v.φ <= φ_zero * ϵ_k); // approximate wolf condition 4.1: (2δ − 1) * φ_prime(0) ≥ φ_prime(α_k) ≥ σ * φ_prime(0)
}

template<typename function_class, typename RVT>
void update_with_hager_zhang_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type deriv, IN(RVT) direction) {
	constexpr value_type δ = 0.1; // range (0, 0.5)  used in the Wolfe conditions
	constexpr value_type σ = 0.9; // range [δ, 1)  used in the Wolfe conditions
	constexpr value_type ϵ = 0.000001; // range [0, inf) used in the approximate Wolfe conditions
	constexpr value_type θ = 0.5; // range (0, 1) used in the update rules when the potential intervals [a,c] or [c, b] violate the opposite slope condition
	constexpr value_type γ = 0.66; // range (0, 1) determines when a bisection step is performed
	constexpr value_type η = 0.01; // range (0, inf) used in the lower bound for β

	limiting_value a_k(0.0, function.evaluate_at_with_derivative(variable_mapping, 0.0, direction));
	const auto φ_zero = a_k.φ;
	const auto φ_prime_zero = a_k.φ_prime;
	limiting_value b_k(max_lambda, function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction));


	do { 
		ϵ_k = ϵ * abs(b_k.φ);

		const auto interval = hager_zhang_secant_step(function, variable_mapping, direction, a_k, b_k, φ_zero, ϵ_k, θ);

		if (interval.second.at - interval.first.at > γ * (b_k.at - a_k.at)) {
			const auto c_val = (interval.first.at + interval.second.at) / 2.0;
			const limiting_value c(c_val, function.evaluate_at_with_derivative(variable_mapping, c_val, direction));
			const auto new_interval = hager_zhang_interval_update(function, variable_mapping, direction, interval.first, interval.second, c, φ_zero, ϵ_k, θ);

			a_k = new_interval.first;
			b_k = new_interval.second;
		} else {
			a_k = interval.first;
			b_k = interval.second;
		}

	} while(!satisfies_hager_zhang_conditions(b_k, φ_zero, φ_prime_zero, δ, σ, ϵ_k)); // conditions not satisfied 

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * b_k.at;
	}
	if (b_k.at == max_lambda)
		variable_mapping[max_index].mapped_index = 0.0;
	return;
}

template<typename function_class, typename RVT>
void update_with_derivative_interpolation_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type deriv, IN(RVT) direction) {
	value_type phi_a0 = function.evaluate_at(variable_mapping, 0.0, direction);
	value_type phi_derivitive_a0 = deriv;

	constexpr value_type slope_factor = 0.001;

	auto phi_a = function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction);

	if (phi_a.first <= phi_a0 + (slope_factor * max_lambda * phi_derivitive_a0)) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * max_lambda;
		}
		variable_mapping[max_index].current_value = 0.0;
		return;
	}

	value_type a_last = max_lambda;
	auto phi_a_last = phi_a;

	//Quadratic interpolation
	value_type a = std::min(-0.5 * phi_derivitive_a0 * max_lambda * max_lambda / (a_phi.first - phi_a0 - phi_derivitive_a0 * max_lambda), max_lambda);
	phi_a = function.evaluate_at_with_derivative(variable_mapping, a, direction);

	if (phi_a.first <= phi_a0 + (slope_factor * a * phi_derivitive_a0)) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a;
		}
		return;
	}

	while (true) {
		const value_type d_1 = phi_a_last.second + phi_a.second + 3 * phi_a_last.first - phi_a.first / (a_last - a);
		const value_type d_2 = sqrt(d_1*d_1 - phi_a_last.second*phi_a.second);

		value_type a_new = a - (a - a_last) * ((phi_a.second + d_2 - d_1) / (phi_a.second - phi_a_last.second + 2 * d_2));
		auto phi_a_new = function.evaluate_at_with_derivative(variable_mapping, a_new, direction);

		if (phi_a_new.first <= phi_a0 + (slope_factor * a * phi_derivitive_a0)) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_new;
			}
			return;
		}

		if (a - a_new > 0.5 * a || 1.0 - a_new / a < 0.9) {
			a_new = 0.5 * a;
			phi_a_new = function.evaluate_at_with_derivative(variable_mapping, a_new, direction);
		}

		a_last = a;
		phi_a_last = phi_a;
		a = a_new;
		phi_a = phi_a_new;
	}
}

template<typename function_class, typename RVT>
void update_with_interpolation_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type deriv, IN(RVT) direction) {
	value_type a0_a = 0.0;
	value_type a0_phi = function.evaluate_at(variable_mapping, 0.0, direction);
	value_type a0_phi_derivitive = deriv;

	constexpr value_type slope_factor = 0.001;
	
	value_type a_a = max_lambda;
	value_type a_phi = function.evaluate_at(variable_mapping, max_lambda, direction);

	if (a_phi <= a0_phi + (slope_factor* a_a * a0_phi_derivitive)) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * max_lambda;
		}
		variable_mapping[max_index].current_value = 0.0;
		return;
	}

	value_type a_last_a = a_a;
	value_type a_last_phi = a_phi;

	//Quadratic interpolation
	a_a = -0.5 * a0_phi_derivitive * a_a * a_a / (a_phi - a0_phi - a0_phi_derivitive * a_a);
	a_phi = function.evaluate_at(variable_mapping, a_a, direction);

	if (a_phi <= a0_phi + (slope_factor* a_a * a0_phi_derivitive)) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_a;
		}
		return;
	}
	while (true) {
		const value_type multiplier = 1.0 / (a_last_a * a_last_a * a_a * a_a * (a_a - a_last_a));
		const value_type m_1 = a_phi - a0_phi - a0_phi_derivitive * a_a;
		const value_type m_2 = a_last_phi - a0_phi - a0_phi_prime * a_last_a;

		const value_type a = multiplier * (a_last_a * a_last_a * m_1 - a_a * a_a * m_2);
		const value_type b = multiplier * (a_a * a_a * a_a * m_2 - a_last_a * a_last_a * a_last_a * m_1);

		value_type a_new_a = -b + sqrt(b * b - 3.0 * a * a0_phi_derivitive)) / (3.0 * a);
		value_type a_new_phi = function.evaluate_at(variable_mapping, a_new_a, direction);

		if (a_new_phi <= a0_phi + (slope_factor* a_a * a0_phi_derivitive)) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_new_a;
			}
			return;
		}

		if (a_a - a_new_a > 0.5 * a_a || 1.0 - a_new_a / a_a < 0.9) {
			a_new_a = 0.5 * a_a;
			a_new_phi = function.evaluate_at(variable_mapping, a_new_a, direction);
		}

		a_last_a = a_a;
		a_last_phi = a_phi;
		a_a = a_new_a;
		a_phi = a_new_phi;
	}
}

template<typename function_class, typename RVT>
void update_with_backtrack_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type lambda, const short max_index, const value_type deriv, IN(RVT) direction) {
	constexpr value_type factor = 0.5;

	const value_type base_value = function.evaluate_at(variable_mapping, 0.0, direction);
	if ((function.evaluate_at(variable_mapping, lambda, direction) - base_value) <= lambda * deriv * factor) {
		// successful at max
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * lambda;
		}
		variable_mapping[max_index].current_value = 0.0;
		return;
	} 

	while (true) {
		lambda /= 2.0;

		if ((function.evaluate_at(variable_mapping, lambda, direction) - base_value) <= lambda * deriv * factor) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * lambda;
			}
			return;
		}
	}
}

template<typename function_class, typename MT>
void m_algo(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(MT) coeff, IN(std::vector<unsigned short>) rank_starts) {
	const auto b_size = coeff.rows();
	const auto full = coeff.cols();
	const auto remainder = b_size - full;

	Eigen::Map<e_row_vector, Eigen::Aligned> gradient((value_type*)_alloca(full * sizeof(value_type)), full);
	Eigen::Map<e_row_vector, Eigen::Aligned> direction((value_type*)_alloca(full * sizeof(value_type)), full);
	Eigen::Map<e_row_vector, Eigen::Aligned> reduced_n_gradient((value_type*)_alloca(remainder * sizeof(value_type)), remainder);

	//inside iteration
	do {
		maximize_basis(coeff, variable_mapping, rank_starts);

		gradient.noalias() = e_row_vector::Zero(full);
		calculate_simple_gradient(function, variable_mapping, gradient);


		reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
		caclulate_reduced_n_gradient(coeff, gradient, n_result);

		direction.noalias() = e_row_vector::Zero(full);
		calcuate_n_direction_mokhtar(coeff, variable_mapping, reduced_n_gradient, direction);
		calcuate_b_direction(coeff, direction);

		// perform line search, update solution
		const auto lm_max = max_lambda(variable_mapping, direction);
#ifndef SAFETY_OFF
		if (lm_max.second == -1)
			abort();
#endif
		const value_type m = (gradient * direction.transpose())(0,0);
		upate_with_backtrack_ls(function, variable_mapping, lm_max.first, lm_max.second, m, direction);

		// test for satisfaction or iterate
	} while (true);
}

void main_func() {
	var_mapping* variable_mapping;
	float* current_solution;

	Eigen::RowVectorXf n_gradient;
	Eigen::RowVectorXf b_gradient;
	calculate_gradient(current_solution, variable_mapping, n_gradient, b_gradient);

	Eigen::Index num_constraints;
	Eigen::Index num_variables;

	Eigen::MatrixXf n_constraints = Eigen::MatrixXf::Zero(num_constraints, num_variables - num_constraints);
	Eigen::MatrixXf b_matrix = Eigen::MatrixXf::Zero(num_constraints, num_constraints);



}