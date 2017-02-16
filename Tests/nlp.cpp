#include "globalhelpers.h"
#include "nlp.h"

template<typename VT>
void _vector_times_ut_inverse(IN(VT) vector, IN(matrix_type) coeff, INOUT(rvector_type) result) { // result must be zeroed prior to call
	const auto sz = coeff.rows();
	for (Eigen::Index i = 0; i < sz; ++i) {
		result(i) = (vector(i) - (result * coeff.col(i))(0, 0)) / (coeff(i, i));
	}
}

void vector_times_ut_inverse(IN(rvector_type) vector, IN(matrix_type) coeff, INOUT(rvector_type) result) { // result must be zeroed prior to call
	_vector_times_ut_inverse(vector, coeff, result);
}

void ut_inverse_times_vector(IN(matrix_type) coeff, IN(vector_type) vector, INOUT(vector_type) result) { // result must be zeroed prior to call
	const auto sz = coeff.rows();
	for (Eigen::Index i = sz - 1; i >= 0; --i) {
		result(i) = (vector(i) - (coeff.row(i).head(sz) * result)(0, 0)) / (coeff(i, i));
	}
}

template<typename VT>
void _vector_times_LU_inverse(IN(VT) vector, IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, INOUT(rvector_type) result) {
	//result.noalias() = (lu_decomposition.transpose().solve(vector.transpose())).transpose();
	result.transpose() = lu_decomposition.transpose().solve(vector.transpose());
}

void vector_times_LU_inverse(IN(rvector_type) vector, IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, INOUT(rvector_type) result) {
	_vector_times_LU_inverse(vector, lu_decomposition, result);
}

void LU_inverse_times_vector(IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, IN(vector_type) vector, INOUT(vector_type) result) {
	result.noalias() = lu_decomposition.solve(vector);
}

void caclulate_reduced_n_gradient_ut(Eigen::Index rows, Eigen::Index remainder, IN(matrix_type) coeff, IN(rvector_type) gradient, INOUT(rvector_type) n_result) {
	rvector_type intermediate((value_type*)_alloca(rows * sizeof(value_type)), rows);
	intermediate.noalias() = rvector_type::Zero(rows);

	_vector_times_ut_inverse(gradient.head(rows), coeff, intermediate);
	n_result.noalias() = gradient.tail(remainder) - (intermediate * coeff.rightCols(remainder));
}

void calcuate_b_direction_ut(Eigen::Index rows, Eigen::Index remainder, IN(matrix_type) coeff, INOUT(vector_type) direction) {
	vector_type n_times_d((value_type*)_alloca(rows * sizeof(value_type)), rows);
	n_times_d.noalias() = coeff.rightCols(remainder) * direction.tail(remainder);

	vector_type intermediate((value_type*)_alloca(rows * sizeof(value_type)), rows);
	intermediate.noalias() = vector_type::Zero(rows);

	ut_inverse_times_vector(coeff, n_times_d, intermediate);
	direction.head(rows).noalias() = -intermediate;
}

void caclulate_reduced_n_gradient_LU(Eigen::Index rows, Eigen::Index remainder, IN(matrix_type) coeff, IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, IN(rvector_type) gradient, INOUT(rvector_type) n_result) {
	n_result.transpose().noalias() = gradient.tail(remainder).transpose() - 
		(coeff.rightCols(coeff.cols() - rows).transpose() * ((lu_decomposition.transpose()).solve(gradient.head(rows).transpose())));
}

void calcuate_b_direction_LU(Eigen::Index rows, Eigen::Index remainder, IN(matrix_type) coeff, IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, INOUT(vector_type) direction) {
	direction.head(rows).noalias() = -lu_decomposition.solve(coeff.rightCols(remainder) * direction.tail(remainder));
}

void calcuate_n_direction_steepest(const Eigen::Index b_size, const Eigen::Index remainder, IN(std::vector<var_mapping>) variable_mapping, IN(rvector_type) reduced_n_gradient, INOUT(vector_type) direction) {
	direction.tail(remainder).noalias() = -(reduced_n_gradient.transpose());

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		if (direction(mi) < value_type(0.0) && variable_mapping[i].current_value == value_type(0.0)) {
			direction(mi) = value_type(0.0);
		}
	}
}

void calcuate_n_direction_mokhtar(const Eigen::Index b_size, const Eigen::Index remainder, IN(std::vector<var_mapping>) variable_mapping, IN(rvector_type) reduced_n_gradient, INOUT(vector_type) direction) {
	direction.tail(remainder).noalias() = -(reduced_n_gradient.transpose());

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		const auto mi = variable_mapping[i].mapped_index;
		if (direction(mi) < value_type(0.0)) {
			direction(mi) *= variable_mapping[i].current_value;
		}
	}
}

using cg_function_type = value_type(*)(IN(rvector_type), IN(rvector_type), IN(rvector_type));

value_type cg_hestenes_stiefel(IN(rvector_type) current_gradient, IN(rvector_type) prev_gradient, IN(rvector_type) prev_cg) {
	return (current_gradient * (current_gradient - prev_gradient).transpose())(0, 0) / (-prev_cg * (current_gradient - prev_gradient).transpose())(0, 0);
}

value_type cg_hager_zhang(IN(rvector_type) current_gradient, IN(rvector_type) prev_gradient, IN(rvector_type) prev_cg) {
	constexpr value_type η = value_type(0.01); // range (0, inf) used in the lower bound for β

	const auto dty = value_type(1.0) / ((-prev_cg * (current_gradient - prev_gradient).transpose())(0, 0));
	const auto β = dty * ((current_gradient - prev_gradient - value_type(2.0) * (current_gradient - prev_gradient).squaredNorm() * dty * -prev_cg) * current_gradient.transpose())(0, 0);
	return std::max(β, value_type (-1.0) / (prev_cg.norm() * std::min(η, prev_gradient.norm())));
}

value_type cg_polak_ribiere(IN(rvector_type) current_gradient, IN(rvector_type) prev_gradient, IN(rvector_type) prev_cg) {
	return (current_gradient * (current_gradient - prev_gradient).transpose())(0, 0) / (prev_gradient.squaredNorm());
}

value_type cg_polak_ribiere_plus(IN(rvector_type) current_gradient, IN(rvector_type) prev_gradient, IN(rvector_type) prev_cg) {
	return std::max((current_gradient * (current_gradient - prev_gradient).transpose())(0, 0) / (prev_gradient.squaredNorm()), value_type(0.0));
}

value_type cg_fletcher_reeves(IN(rvector_type) current_gradient, IN(rvector_type) prev_gradient, IN(rvector_type) prev_cg) {
	return (current_gradient.squaredNorm()) / (prev_gradient.squaredNorm());
}

std::pair<value_type, short> max_lambda(INOUT(std::vector<var_mapping>) variable_mapping, INOUT(vector_type) direction) {
	short index = -1;
	value_type least = max_value<value_type>::value;

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
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

template<typename replacement_selection>
void swap_base(INOUT(matrix_type) coeff, INOUT(std::vector<var_mapping>) variable_mapping, IN(std::vector<unsigned short>) rank_starts, const Eigen::Index to_replace) {
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

void maximize_basis(INOUT(matrix_type) coeff, INOUT(std::vector<var_mapping>) variable_mapping, IN(std::vector<unsigned short>) rank_starts) {
	const auto last_rank = rank_starts.size() - 1;
	unsigned short current_rank = 0;
	unsigned short rank_end = rank_starts[1];
	unsigned short largest = 0;
	value_type largest_value = value_type(0.0);
	unsigned short basis = 0;

	for (unsigned short current_position = 0; true; ++current_position) {
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

bool zero_element_in_basis(const Eigen::Index basis_size, IN(std::vector<var_mapping>) variable_mapping) {
	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		if ((variable_mapping[i].current_value == value_type(0.0)) & (variable_mapping[i].mapped_index < basis_size)) {
			return true;
		}
	}
	return false;
}

bool conditional_maximize_basis(const Eigen::Index basis_size, INOUT(matrix_type) coeff, INOUT(std::vector<var_mapping>) variable_mapping, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	const auto sz = variable_mapping.size();

	if (!zero_element_in_basis(basis_size, variable_mapping)) {
		return false;
	}

	for (unsigned short b = 0; b < static_cast<unsigned short>(basis_size); ++b) {
		unsigned short largest = max_value<unsigned short>::value;
		unsigned short current_basis = max_value<unsigned short>::value;
		value_type largest_value = value_type(0.0);

		for (auto pr = rank_starts.equal_range(b); pr.first != pr.second; ++pr.first) {
			const auto indx = pr.first->second;
			if (((variable_mapping[indx].mapped_index == b) | (variable_mapping[indx].mapped_index >= basis_size)) & (variable_mapping[indx].current_value > largest_value)) {
				largest = indx;
				largest_value = variable_mapping[indx].current_value;
			}
			if (variable_mapping[indx].mapped_index == b) {
				current_basis = indx;
			}
		}

		const auto largest_index = variable_mapping[largest].mapped_index;
		coeff.col(b).swap(coeff.col(largest_index));
		variable_mapping[largest].mapped_index = b;
		variable_mapping[current_basis].mapped_index = largest_index;
	}

	return true;
}

struct limiting_value {
	value_type at;
	value_type φ;
	value_type φ_prime;
	limiting_value() : at(0.0), φ(0.0), φ_prime(0.0) {};
	limiting_value(value_type a, value_type b, value_type c) : at(a), φ(b), φ_prime(c) {};
	limiting_value(value_type a, IN(std::pair<value_type, value_type>) b) : at(a), φ(b.first), φ_prime(b.second) {};
};

template<typename function_class>
std::pair<limiting_value, limiting_value> hager_zhang_interval_update(IN(function_class) function, IN(std::vector<var_mapping>) variable_mapping, IN(vector_type) direction, IN(limiting_value) a, IN(limiting_value) b, IN(limiting_value) c, const value_type φ_zero, const value_type ϵ_k) {
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

template<typename function_class>
std::pair<limiting_value, limiting_value> hager_zhang_secant_step(IN(function_class) function, IN(std::vector<var_mapping>) variable_mapping, IN(vector_type) direction, IN(limiting_value) a, IN(limiting_value) b, const value_type φ_zero, const value_type ϵ_k) {
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

	static_assert(σ_times_1000 > δ_times_1000, "sigma must be > than delta");
	static_assert(σ_times_1000 < 1000, "sigma must be < than 1");
	static_assert(δ_times_1000 < 500, "delta must be < than 0.5");
	static_assert(δ_times_1000 > 0, "delta must be > than 0");


	return 0 != (((v.φ <= φ_zero + δ * v.at * φ_prime_zero) & (v.φ_prime >= σ * φ_prime_zero))   // wolfe condition 2.2: f(x_k + α_k * d_k ) − f(x _k) ≤ δ * α_k * g_k * d_k &&  wolfe condition 2.3: g_k+1*d_k >= σ*g_k*d_k
		| (((value_type(2.0) * δ - value_type(1.0)) * φ_prime_zero >= v.φ_prime) & (v.φ_prime >= σ * φ_prime_zero) & (v.φ <= φ_zero + ϵ_k))); // approximate wolf condition 4.1: (2δ − 1) * φ_prime(0) ≥ φ_prime(α_k) ≥ σ * φ_prime(0)
}

template<int δ_times_1000, int σ_times_1000>
bool satisfies_hager_zhang_conditions_zero_epsilon(IN(limiting_value) v, const value_type φ_zero, const value_type φ_prime_zero) {
	constexpr value_type δ = static_cast<value_type>(δ_times_1000) / value_type(1000.0); // range (0, 0.5)  used in the Wolfe conditions
	constexpr value_type σ = static_cast<value_type>(σ_times_1000) / value_type(1000.0); // range [δ, 1)  used in the Wolfe conditions
	
	static_assert(σ_times_1000 > δ_times_1000, "sigma must be > than delta");
	static_assert(σ_times_1000 < 1000, "sigma must be < than 1");
	static_assert(δ_times_1000 < 500, "delta must be < than 0.5");
	static_assert(δ_times_1000 > 0, "delta must be > than 0");


	return 0 != (((v.φ <= φ_zero + δ * v.at * φ_prime_zero) & (v.φ_prime >= σ * φ_prime_zero))   // wolfe condition 2.2: f(x_k + α_k * d_k ) − f(x _k) ≤ δ * α_k * g_k * d_k &&  wolfe condition 2.3: g_k+1*d_k >= σ*g_k*d_k
		| (((value_type(2.0) * δ - value_type(1.0)) * φ_prime_zero >= v.φ_prime) & (v.φ_prime >= σ * φ_prime_zero) & (v.φ <= φ_zero))); // approximate wolf condition 4.1: (2δ − 1) * φ_prime(0) ≥ φ_prime(α_k) ≥ σ * φ_prime(0)
}

template<int δ_times_1000>
bool satisfies_hager_zhang_delta_conditions(IN(limiting_value) v, const value_type φ_zero, const value_type φ_prime_zero) {
	constexpr value_type δ = static_cast<value_type>(δ_times_1000) / value_type(1000.0); // range (0, 0.5)  used in the Wolfe conditions

	static_assert(δ_times_1000 < 500, "delta must be < than 0.5");
	static_assert(δ_times_1000 > 0, "delta must be > than 0");


	return 0 != (((v.φ <= φ_zero + δ * v.at * φ_prime_zero))   // wolfe condition 2.2: f(x_k + α_k * d_k ) − f(x _k) ≤ δ * α_k * g_k * d_k &&  wolfe condition 2.3: g_k+1*d_k >= σ*g_k*d_k
		| (((value_type(2.0) * δ - value_type(1.0)) * φ_prime_zero >= v.φ_prime) & (v.φ <= φ_zero))); // approximate wolf condition 4.1: (2δ − 1) * φ_prime(0) ≥ φ_prime(α_k) ≥ σ * φ_prime(0)
}

template<typename function_class>
void update_with_hager_zhang_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type deriv, IN(vector_type) direction) {
	constexpr value_type ϵ = value_type(0.000001); // range [0, inf) used in the approximate Wolfe conditions
	constexpr value_type γ = value_type(0.66); // range (0, 1) determines when a bisection step is performed
	
	// constexpr value_type η = 0.01; // range (0, inf) used in the lower bound for β

	limiting_value a_k(value_type(0.0), function.evaluate_at_with_derivative(variable_mapping, value_type(0.0), direction));
	const auto φ_zero = a_k.φ;
	const auto φ_prime_zero = a_k.φ_prime;
	limiting_value b_k(max_lambda, function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction));
	value_type ϵ_k = value_type(0.0);

	do { 
		ϵ_k = ϵ * abs(b_k.φ);

		const auto interval = hager_zhang_secant_step(function, variable_mapping, direction, a_k, b_k, φ_zero, ϵ_k);

		if (interval.second.at > γ * (b_k.at - a_k.at) + interval.first.at) {
			const auto c_val = (interval.first.at + interval.second.at) / value_type(2.0);
			const limiting_value c(c_val, function.evaluate_at_with_derivative(variable_mapping, c_val, direction));
			std::tie(a_k, b_k) = hager_zhang_interval_update(function, variable_mapping, direction, interval.first, interval.second, c, φ_zero, ϵ_k);
		} else {
			std::tie(a_k, b_k) = interval;
		}

	} while(!satisfies_hager_zhang_conditions<100,900>(b_k, φ_zero, φ_prime_zero, ϵ_k) & (std::nextafter(a_k.at, max_value<value_type>::value) != b_k.at)); // conditions not satisfied 

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * b_k.at;
	}
	if (b_k.at == max_lambda)
		variable_mapping[max_index].current_value = value_type(0.0);
	return;
}

std::string fp_output_helper(value_type f) {
	char* buffer = (char*)_alloca(100);
	snprintf(buffer, 100, "%.9g", f);
	return std::string(buffer);
}


template<typename function_class>
void update_with_derivitive_minimization_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, const value_type max_lambda, const short max_index, const value_type φ_prime_a0, IN(vector_type) direction) {
	constexpr int δ_times_1000 = 100;
	constexpr int σ_times_1000 = 900;
	constexpr value_type ϵ = value_type(0.000001); // range [0, inf) used in the approximate Wolfe conditions

	const value_type φ_a0 = function.evaluate_at(variable_mapping, value_type(0.0), direction);

	auto a_k = limiting_value(value_type(0.0), φ_a0, φ_prime_a0);
	auto b_k = limiting_value(max_lambda, function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction));

	while (b_k.φ_prime < 0 && std::nextafter(a_k.at, max_value<value_type>::value) != b_k.at) {
		const auto c_k_at = value_type(0.5) * (a_k.at + b_k.at);
		auto c_k = limiting_value(c_k_at, function.evaluate_at_with_derivative(variable_mapping, c_k_at, direction));
		if (c_k.φ_prime >= 0) {
			b_k = c_k;
		} else if (c_k.φ <= a_k.φ) {
			a_k = c_k;
		} else {
			b_k = c_k;
		}
	}

	auto ϵ_k = ϵ * abs(b_k.φ);
	while (!satisfies_hager_zhang_conditions<δ_times_1000, σ_times_1000>(b_k, φ_a0, φ_prime_a0, ϵ_k)  && std::nextafter(a_k.at, max_value<value_type>::value) != b_k.at) {
		const auto c_k_at = secant(a_k, b_k);
		const auto c_k = ((c_k_at == a_k.at) | (c_k_at == b_k.at)) ?
			limiting_value((a_k.at + b_k.at) * value_type(0.5), function.evaluate_at_with_derivative(variable_mapping, (a_k.at + b_k.at) * value_type(0.5), direction))
			: limiting_value(c_k_at, function.evaluate_at_with_derivative(variable_mapping, c_k_at, direction));
		if (c_k.φ_prime >= 0) {
			b_k = c_k;
		} else {
			a_k = c_k;
		}
		ϵ_k = ϵ * abs(b_k.φ);
	}

	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * b_k.at;
	}
	if (b_k.at == max_lambda)
		variable_mapping[max_index].current_value = value_type(0.0);
}

template<typename function_class>
void update_with_derivative_interpolation_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type φ_prime_a0, IN(vector_type) direction) {
	const value_type φ_a0 = function.evaluate_at(variable_mapping, value_type(0.0), direction);

	constexpr value_type δ = value_type(0.1);

	limiting_value a_k(max_lambda, function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction));

	if (a_k.φ <= (δ * max_lambda * φ_prime_a0) + φ_a0) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * max_lambda;
		}
		variable_mapping[max_index].current_value = value_type(0.0);
		return;
	}

	while (true) {
		const value_type d_1 = φ_prime_a0 + a_k.φ_prime - value_type(3) * (φ_a0 - a_k.φ) / (-a_k.at);
		const value_type d_2 = sqrt(d_1*d_1 - φ_prime_a0 * a_k.φ_prime);

		const auto denom = a_k.φ_prime - φ_prime_a0 + value_type(2) * d_2;
		const value_type a_new_at = a_k.at - (a_k.at) * ((a_k.φ_prime + d_2 - d_1) / denom);
		//const auto a_new = limiting_value(a_new_at, function.evaluate_at_with_derivative(variable_mapping, a_new_at, direction));

#ifndef SAFETY_OFF
		if (isnan(a_new_at)) {
			std::string rstr = std::string("d_1: ") + fp_output_helper(d_1) + ", d_2: " + fp_output_helper(d_2) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("a_k_at: ") + fp_output_helper(a_k.at) + ", a_k_phi: " + fp_output_helper(a_k.φ) + ", phi_prime_a_k:" + fp_output_helper(a_k.φ_prime) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("phi_prime_a0: ") + fp_output_helper(φ_prime_a0) + ", phi_a0: " + fp_output_helper(φ_a0) + ", denom:" + fp_output_helper(denom) + "\n";
			OutputDebugStringA(rstr.c_str());
			abort();
		}
#endif // !SAFETY_OFF

		if (a_k.at - a_new_at > value_type(0.9) * a_k.at || a_k.at - a_new_at < value_type(0.1) * a_k.at) {
			const value_type a_new_at_b = value_type(0.5) * a_k.at;
			a_k = limiting_value(a_new_at_b, function.evaluate_at_with_derivative(variable_mapping, a_new_at_b, direction));
		} else {
			a_k = limiting_value(a_new_at, function.evaluate_at_with_derivative(variable_mapping, a_new_at, direction));
		}

		if ((a_k.φ <= (δ * a_k.at * φ_prime_a0) + φ_a0) & (a_k.φ_prime <= 0)) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_k.at;
			}
			return;
		}
	}
}

template<typename function_class>
void update_with_interpolation_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, value_type max_lambda, const short max_index, const value_type φ_prime_a0, IN(vector_type) direction) {
	const value_type φ_a0 = function.evaluate_at(variable_mapping, value_type(0.0), direction);

	constexpr value_type δ = value_type(0.1);
	
	value_type a_k_at = max_lambda;
	limiting_value a_k = limiting_value(a_k_at, function.evaluate_at_with_derivative(variable_mapping, a_k_at, direction));

	if (a_k.φ <= (δ * max_lambda * φ_prime_a0) + φ_a0) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * max_lambda;
		}
		variable_mapping[max_index].current_value = value_type(0.0);
		return;
	}

	auto a_last = a_k;


	a_k_at = value_type(-0.5) * φ_prime_a0 * max_lambda * max_lambda / a_k.φ - φ_a0 - φ_prime_a0 * max_lambda;
	if ( max_lambda - a_k_at >= value_type(0.9) * max_lambda || max_lambda - a_k_at <= value_type(0.1) * max_lambda) {
		a_k = limiting_value(value_type(0.5) * max_lambda, function.evaluate_at_with_derivative(variable_mapping, value_type(0.5) * max_lambda, direction));
	} else {
		a_k = limiting_value(a_k_at, function.evaluate_at_with_derivative(variable_mapping, a_k_at, direction));
	}

	if((a_k.φ <= (δ * a_k.at * φ_prime_a0) + φ_a0) & (a_k.φ_prime <= 0)) {
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_k.at;
		}
		return;
	}

	while (true) {
		const value_type multiplier = value_type(1.0) / (a_last.at * a_last.at * a_k.at * a_k.at * (a_k.at - a_last.at));
		const value_type m_1 = a_k.φ - φ_a0 - φ_prime_a0 * a_k.at;
		const value_type m_2 = a_last.φ - φ_a0 - φ_prime_a0 * a_last.at;

		const value_type a = multiplier * (a_last.at * a_last.at * m_1 - a_k.at *a_k.at * m_2);
		const value_type b = multiplier * (a_k.at * a_k.at * a_k.at * m_2 - a_last.at * a_last.at * a_last.at * m_1);

		value_type a_new_at = -b + sqrt(b * b - value_type(3.0) * a * φ_prime_a0) / (value_type(3.0) * a);

#ifndef SAFETY_OFF
		if (isnan(a_new_at)) {
			std::string rstr = std::string("multiplier: ") + fp_output_helper(multiplier) + ", a_k_at: " + fp_output_helper(a_k.at) + ", a_k_last_at:" + fp_output_helper(a_last.at) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("a: ") + fp_output_helper(a) + ", b: " + fp_output_helper(b) + ", phi_prime_a0:" + fp_output_helper(φ_prime_a0) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("b * b: ") + fp_output_helper(b * b) + ", rest: " + fp_output_helper(value_type(3.0) * a * φ_prime_a0) + ", sqrt of:" + fp_output_helper(b * b - value_type(3.0) * a * φ_prime_a0) + "\n";
			OutputDebugStringA(rstr.c_str());
			abort();
		}
#endif // !SAFETY_OFF

		a_last = a_k;

		if (a_k.at >= value_type(0.9) * a_k.at + a_new_at || a_k.at <= value_type(0.1) * a_k.at + a_new_at) {
			a_k = limiting_value(value_type(0.5) * a_k.at, function.evaluate_at_with_derivative(variable_mapping, value_type(0.5) * a_k.at, direction));
		} else {
			a_k = limiting_value(a_new_at, function.evaluate_at_with_derivative(variable_mapping, a_new_at, direction));
		}

		if ((a_k.φ <= (δ * a_k.at * φ_prime_a0) + φ_a0) & (a_k.φ_prime <= 0)) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_k.at;
			}
			return;
		}
	}
}

template<typename function_class>
void update_with_backtrack_ls(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, const value_type lambda, const short max_index, const value_type φ_prime_a0, IN(vector_type) direction) {
	constexpr value_type δ = value_type(0.100);

	const value_type φ_a0 = function.evaluate_at(variable_mapping, value_type(0.0), direction);
	auto a_k = limiting_value(lambda, function.evaluate_at_with_derivative(variable_mapping, lambda, direction));

	if (a_k.φ <= φ_a0 + δ * a_k.at * φ_prime_a0) {
		// successful at max
		const auto sz = variable_mapping.size();
		for (size_t i = 0; i < sz; ++i) {
			variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * lambda;
		}
		variable_mapping[max_index].current_value = value_type(0.0);
		return;
	}

	a_k = limiting_value(a_k.at * value_type(0.5), function.evaluate_at_with_derivative(variable_mapping, a_k.at * value_type(0.5), direction));

	do {
		if((a_k.φ <= φ_a0 + δ * a_k.at * φ_prime_a0) & (a_k.φ_prime <= 0)) {
			const auto sz = variable_mapping.size();
			for (size_t i = 0; i < sz; ++i) {
				variable_mapping[i].current_value += direction(variable_mapping[i].mapped_index) * a_k.at;
				//minimum_adjust(variable_mapping[i].current_value, direction(variable_mapping[i].mapped_index), a_k.at);
			}
			return;
		}

		a_k = limiting_value(a_k.at * value_type(0.5), function.evaluate_at_with_derivative(variable_mapping, a_k.at * value_type(0.5), direction));
	} while (a_k.at != 0);

	OutputDebugStringA("zero lambda");
}

template<typename function_class>
using ls_function_type = void(*)(IN(function_class), INOUT(std::vector<var_mapping>), value_type, const short, const value_type, IN(vector_type));

template<ls_function_type<linear_test_function> LINE_SEARCH>
std::pair<value_type, size_t> linear_steepest_descent(IN(linear_test_function) func, value_type max_v) {
	std::vector<var_mapping> variable = {var_mapping{value_type(0.0),0}};

	value_type d_value = value_type(0.0);
	vector_type direction(&d_value, 1);

	value_type g_value = value_type(0.0);
	rvector_type gradient(&g_value, 1);

	size_t count = 0;

	do {
		func.gradient_at(variable, gradient);
		direction.noalias() = -gradient;

		// std::string r_str = std::string("step ") + std::to_string(count) + ": f(" + fp_output_helper(variable[0].current_value) + ") = " + fp_output_helper(func.evaluate_at(variable)) + ", dx: " + fp_output_helper(gradient(0)) + "\n";
		// OutputDebugStringA(r_str.c_str());
		// r_str = fp_output_helper(std::nextafter(variable[0].current_value, value_type(0.0))) + " < " + fp_output_helper(variable[0].current_value) + " < " + fp_output_helper(std::nextafter(variable[0].current_value, max_value<value_type>::value)) + "\n";
		// OutputDebugStringA(r_str.c_str());

		if ((direction(0) < 0 && variable[0].current_value == value_type(0.0)) ||
			(direction(0) > 0 && variable[0].current_value == max_v) ||
			(abs(direction(0)) < 0.0000001)) {
			break;
		}

		if (direction(0) < 0.0) {
			value_type lm_max = -variable[0].current_value / direction(0);
			LINE_SEARCH(func, variable, lm_max, 0, gradient(0)*direction(0), direction);
		} else {
			value_type lm_max = (max_v - variable[0].current_value) / direction(0);
			LINE_SEARCH(func, variable, lm_max, 0, gradient(0)*direction(0), direction);
			if (variable[0].current_value == value_type(0.0))
				variable[0].current_value = max_v;
		}
		++count;

		

#ifndef SAFETY_OFF
		if (count > 20) {
			OutputDebugStringA("hit safety limit\n");
			break;
		}
#endif

	} while (true);

	return std::make_pair(variable[0].current_value, count);
}

std::pair<value_type, size_t> backtrack_linear_steepest_descent(IN(linear_test_function) func, value_type max_value) {
	return linear_steepest_descent<update_with_backtrack_ls>(func, max_value);
}

std::pair<value_type, size_t> interpolation_linear_steepest_descent(IN(linear_test_function) func, value_type max_value) {
	return linear_steepest_descent<update_with_interpolation_ls>(func, max_value);
}

std::pair<value_type, size_t> derivative_interpolation_linear_steepest_descent(IN(linear_test_function) func, value_type max_value) {
	return linear_steepest_descent<update_with_derivative_interpolation_ls>(func, max_value);
}

std::pair<value_type, size_t> hager_zhang_linear_steepest_descent(IN(linear_test_function) func, value_type max_value) {
	return linear_steepest_descent<update_with_hager_zhang_ls>(func, max_value);
}

std::pair<value_type, size_t> derivitive_minimization_steepest_descent(IN(linear_test_function) func, value_type max_value) {
	return linear_steepest_descent<update_with_derivitive_minimization_ls>(func, max_value);
}

using reduced_gradient_to_direction_map_type = void(*)(const Eigen::Index, const Eigen::Index, IN(std::vector<var_mapping>), IN(rvector_type), INOUT(vector_type));

template<typename function_class, ls_function_type<function_class> LINE_SEARCH, reduced_gradient_to_direction_map_type RG_MAP>
void steepest_descent(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	const auto b_size = coeff.rows();
	const auto full = coeff.cols();
	const auto remainder = full - b_size;

	constexpr value_type ϵ = value_type(0.000001); // range [0, inf)

	rvector_type gradient((value_type*)_alloca(full * sizeof(value_type)), full);
	vector_type direction((value_type*)_alloca(full * sizeof(value_type)), full);
	rvector_type reduced_n_gradient((value_type*)_alloca(remainder * sizeof(value_type)), remainder);

	matrix_type LU_store((value_type*)_alloca(sizeof(value_type) * b_size * b_size), b_size, b_size);
	LU_store = coeff.leftCols(b_size);
	Eigen::PartialPivLU<Eigen::Ref<matrix_type>> LU_decomp(LU_store);
	bool is_upper_triangular = coeff.leftCols(b_size).isUpperTriangular();

	//if (!is_upper_triangular) {
	//	LU_store.noalias() = coeff.leftCols(b_size);
	//	new (&LU_decomp) Eigen::PartialPivLU<Eigen::Ref<matrix_type>>(LU_store);
	//}
	
#ifndef SAFETY_OFF
	size_t iteration_count = 0;
#endif

	//inside iteration
	do {
		if (conditional_maximize_basis(b_size, coeff, variable_mapping, rank_starts)) {
			is_upper_triangular = coeff.leftCols(b_size).isUpperTriangular();
			if (!is_upper_triangular) {
				LU_store.noalias() = coeff.leftCols(b_size);
				new (&LU_decomp) Eigen::PartialPivLU<Eigen::Ref<matrix_type>>(LU_store);
			}
		}

		gradient.noalias() = e_row_vector::Zero(full);
		function.gradient_at(variable_mapping, gradient);

		if (is_upper_triangular) {
			reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
			caclulate_reduced_n_gradient_ut(b_size, remainder, coeff, gradient, reduced_n_gradient);

			direction.noalias() = e_vector::Zero(full);
			RG_MAP(b_size, remainder, variable_mapping, reduced_n_gradient, direction);

			calcuate_b_direction_ut(b_size, remainder, coeff, direction);
		} else {
			reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
			caclulate_reduced_n_gradient_LU(b_size, remainder, coeff, LU_decomp, gradient, reduced_n_gradient);

			direction.noalias() = e_vector::Zero(full);
			RG_MAP(b_size, remainder, variable_mapping, reduced_n_gradient, direction);

			calcuate_b_direction_LU(b_size, remainder, coeff, LU_decomp, direction);
		}

		// test for satisfaction
		if (direction.squaredNorm() <= ϵ*ϵ)
			break;

		// perform line search, update solution
		const auto lm_max = max_lambda(variable_mapping, direction);

#ifndef SAFETY_OFF
		if (lm_max.second == -1)
			abort();
#endif
		const value_type m = (gradient * direction)(0,0);

#ifndef SAFETY_OFF
		if (m > value_type(0.0)) {
			OutputDebugStringA("not a descent direction\n");

			std::stringstream strm;
			strm << "gradient: "  << gradient << "\n";
			strm << "direction: " << direction.transpose() << "\n";
			strm << "derivative: " << std::to_string(m) << "\n";

			if (is_upper_triangular) {
				strm << "upper triangular\n";
				strm << "coeff: " << coeff << "\n";

				reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
				caclulate_reduced_n_gradient_ut(b_size, remainder, coeff, gradient, reduced_n_gradient);

				strm << "reduced_n_gradient: " << reduced_n_gradient << "\n";

				direction.noalias() = e_vector::Zero(full);
				RG_MAP(b_size, remainder, variable_mapping, reduced_n_gradient, direction);

				strm << "direction_N: " << direction.transpose() << "\n";

				calcuate_b_direction_ut(b_size, remainder, coeff, direction);

				strm  << "direction_B: " << direction.transpose() << "\n";
				OutputDebugStringA(strm.str().c_str());
			} else {
				OutputDebugStringA("not upper triangular\n");

				reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
				caclulate_reduced_n_gradient_LU(b_size, remainder, coeff, LU_decomp, gradient, reduced_n_gradient);

				direction.noalias() = e_vector::Zero(full);
				RG_MAP(b_size, remainder, variable_mapping, reduced_n_gradient, direction);

				calcuate_b_direction_LU(b_size, remainder, coeff, LU_decomp, direction);
			}


			abort();
		}
#endif // !SAFETY_OFF

		LINE_SEARCH(function, variable_mapping, lm_max.first, lm_max.second, m, direction);

#ifndef SAFETY_OFF
		if (++iteration_count > 50) {
			OutputDebugStringA("too many iterations\n");
			break;
			//abort();
		}
#endif
	} while (true);
}

using restart_conditions_type = bool(*)(size_t, size_t, IN(rvector_type), IN(rvector_type), IN(rvector_type));

bool basic_restart_condition(size_t restart_count, size_t variable_count, IN(rvector_type) previous_cg, IN(rvector_type) previous_gradient, IN(rvector_type) current_gradient) {
	return (restart_count >= variable_count);
}

bool no_restart_condition(size_t restart_count, size_t variable_count, IN(rvector_type) previous_cg, IN(rvector_type) previous_gradient, IN(rvector_type) current_gradient) {
	return false;
}

bool powell_restart_condition(size_t restart_count, size_t variable_count, IN(rvector_type) new_cg, IN(rvector_type) previous_gradient, IN(rvector_type) current_gradient) {
	const auto d_g = -((new_cg.transpose() * current_gradient)(0, 0));
	const auto current_sn = current_gradient.squaredNorm();
	return (restart_count >= variable_count)
		|| abs((previous_gradient.transpose() * current_gradient)(0,0)) >= value_type(0.2) * current_sn
		|| (restart_count >= 2 && value_type(-1.2) * current_sn <= d_g && d_g <= value_type(-0.8) * current_sn);
}


template<typename function_class, ls_function_type<function_class> LINE_SEARCH, reduced_gradient_to_direction_map_type RG_MAP, cg_function_type CG_FUNCTION, restart_conditions_type RESTART_FUNCTION>
void conjugate_gradient_method(IN(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {

	const auto b_size = coeff.rows();
	const auto full = coeff.cols();
	const auto remainder = full - b_size;

	const auto sz = variable_mapping.size();

	constexpr value_type ϵ = value_type(0.000001); // range [0, inf)

	rvector_type gradient((value_type*)_alloca(full * sizeof(value_type)), full);
	vector_type direction((value_type*)_alloca(full * sizeof(value_type)), full);

	rvector_type prev_cg((value_type*)_alloca(remainder * sizeof(value_type)), full);
	rvector_type new_cg((value_type*)_alloca(remainder * sizeof(value_type)), full);
	rvector_type prev_reduced_n_gradient((value_type*)_alloca(remainder * sizeof(value_type)), remainder);
	rvector_type reduced_n_gradient((value_type*)_alloca(remainder * sizeof(value_type)), remainder);

	matrix_type LU_store((value_type*)_alloca(sizeof(value_type) * b_size * b_size), b_size, b_size);
	LU_store = coeff.leftCols(b_size);
	Eigen::PartialPivLU<Eigen::Ref<matrix_type>> LU_decomp(LU_store);
	bool is_upper_triangular = coeff.leftCols(b_size).isUpperTriangular();

	//if (!is_upper_triangular) {
	//	LU_store.noalias() = coeff.leftCols(b_size);
	//	new (&LU_decomp) Eigen::PartialPivLU<Eigen::Ref<matrix_type>>(LU_store);
	//}

#ifndef SAFETY_OFF
	size_t iteration_count = 0;
#endif

	bool after_reset = true;
	size_t restart_count = 0;

	//inside iteration
	do {
		if (conditional_maximize_basis(b_size, coeff, variable_mapping, rank_starts)) {
			is_upper_triangular = coeff.leftCols(b_size).isUpperTriangular();

			if (!is_upper_triangular) {
				LU_store.noalias() = coeff.leftCols(b_size);
				new (&LU_decomp) Eigen::PartialPivLU<Eigen::Ref<matrix_type>>(LU_store);
			}

			after_reset = true;
		}

		gradient.noalias() = e_row_vector::Zero(full);

		function.gradient_at(variable_mapping, gradient);
		reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
		if (is_upper_triangular)
			caclulate_reduced_n_gradient_ut(b_size, remainder, coeff, gradient, reduced_n_gradient);
		else
			caclulate_reduced_n_gradient_LU(b_size, remainder, coeff, LU_decomp, gradient, reduced_n_gradient);


		if (after_reset) {
			prev_reduced_n_gradient.noalias() = reduced_n_gradient;
			prev_cg.noalias() = reduced_n_gradient;

			after_reset = false;
			restart_count = 0;
		} else {
			new_cg.noalias() = reduced_n_gradient + CG_FUNCTION(reduced_n_gradient, prev_reduced_n_gradient, prev_cg) * prev_cg;

			++restart_count;
			after_reset = RESTART_FUNCTION(restart_count, remainder, prev_cg, prev_reduced_n_gradient, new_cg);

			prev_reduced_n_gradient.noalias() = reduced_n_gradient;
			prev_cg.noalias() = new_cg;
		}

		
		direction.noalias() = e_vector::Zero(full);
		RG_MAP(b_size, remainder, variable_mapping, reduced_n_gradient, direction);
		if (is_upper_triangular)
			calcuate_b_direction_ut(b_size, remainder, coeff, direction);
		else
			calcuate_b_direction_LU(b_size, remainder, coeff, LU_decomp, direction);

		// test for satisfaction
		if (direction.squaredNorm() <= ϵ*ϵ)
			break;

		// perform line search, update solution
		const auto lm_max = max_lambda(variable_mapping, direction);

#ifndef SAFETY_OFF
		if (lm_max.second == -1)
			abort();
#endif
		const value_type m = (gradient * direction)(0, 0);
		LINE_SEARCH(function, variable_mapping, lm_max.first, lm_max.second, m, direction);

#ifndef SAFETY_OFF
		if (++iteration_count > 100)
			abort();
#endif
	} while (true);
}

void sof_hz_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_hager_zhang_ls, calcuate_n_direction_steepest>(function, variable_mapping, coeff, rank_starts);
}

void sof_m_hz_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_hager_zhang_ls, calcuate_n_direction_mokhtar>(function, variable_mapping, coeff, rank_starts);
}

void sof_dm_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_derivitive_minimization_ls, calcuate_n_direction_steepest>(function, variable_mapping, coeff, rank_starts);
}

void sof_m_dm_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_derivitive_minimization_ls, calcuate_n_direction_mokhtar>(function, variable_mapping, coeff, rank_starts);
}

void sof_bt_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_backtrack_ls, calcuate_n_direction_steepest>(function, variable_mapping, coeff, rank_starts);
}

void sof_m_bt_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_backtrack_ls, calcuate_n_direction_mokhtar>(function, variable_mapping, coeff, rank_starts);
}

void sof_int_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_interpolation_ls, calcuate_n_direction_steepest>(function, variable_mapping, coeff, rank_starts);
}

void sof_m_int_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_interpolation_ls, calcuate_n_direction_mokhtar>(function, variable_mapping, coeff, rank_starts);
}

void sof_dint_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_derivative_interpolation_ls, calcuate_n_direction_steepest>(function, variable_mapping, coeff, rank_starts);
}

void sof_m_dint_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	steepest_descent<sum_of_functions, update_with_derivative_interpolation_ls, calcuate_n_direction_mokhtar>(function, variable_mapping, coeff, rank_starts);
}

void sum_of_functions::add_function(IN(std::function<value_type(const value_type*const)>) f, IN(std::function<value_type(const value_type*const, const value_type*const)>) f_p, IN(std::vector<unsigned short>) variables) {
	max_function_size = std::max(max_function_size, unsigned int(variables.size()));
	unsigned short fnum = static_cast<unsigned short>(functions.size());
	for (const auto v : variables) {
		variable_map.emplace_back(fnum, v);
	}
	functions.push_back(f);
	function_derivatives.push_back(f_p);
}

value_type sum_of_functions::evaluate_at(INOUT(std::vector<var_mapping>) variable_mapping) const {
	IN_P(value_type) variable_store = (value_type*)_alloca(max_function_size * sizeof(value_type));
	value_type current_sum = value_type(0.0);

	unsigned short current_function = 0;
	unsigned short array_index = 0;

	for (IN(auto) pr : variable_map) {
		if (pr.first != current_function) {
			current_sum += functions[current_function](variable_store);
			current_function = pr.first;
			array_index = 0;
		}
		variable_store[array_index] = variable_mapping[pr.second].current_value;
		++array_index;
	}
	current_sum += functions[current_function](variable_store);
	return current_sum;
}

void sum_of_functions::gradient_at(IN(std::vector<var_mapping>) variable_mapping, INOUT(rvector_type) gradient) const {
	IN_P(value_type) location = (value_type*)_alloca(max_function_size * sizeof(value_type));
	IN_P(value_type) direction = (value_type*)_alloca(max_function_size * sizeof(value_type));

	size_t outer_index = 0;

	for (IN(auto) var : variable_mapping) {
		gradient(var.mapped_index) = value_type(0.0);

		unsigned short current_function = 0;
		unsigned short array_index = 0;
		bool touches_variable = false;

		for (IN(auto) pr : variable_map) {
			if (pr.first != current_function) {
				if (touches_variable) {
					gradient(var.mapped_index) += function_derivatives[current_function](location, direction);
					touches_variable = false;
				}
				current_function = pr.first;
				array_index = 0;
			}
			if (pr.second == outer_index) {
				direction[array_index] = value_type(1.0);
				location[array_index] = variable_mapping[pr.second].current_value;
				touches_variable = true;
			} else {
				location[array_index] = variable_mapping[pr.second].current_value;
				direction[array_index] = value_type(0.0);
			}
			++array_index;
		}

		if (touches_variable) {
			gradient(var.mapped_index) += function_derivatives[current_function](location, direction);
		}

		++outer_index;
	}
}

value_type sum_of_functions::evaluate_at(INOUT(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
	IN_P(value_type) variable_store = (value_type*)_alloca(max_function_size * sizeof(value_type));
	value_type current_sum = value_type(0.0);

	unsigned short current_function = 0;
	unsigned short array_index = 0;

	for (IN(auto) pr : variable_map) {
		if (pr.first != current_function) {
			current_sum += functions[current_function](variable_store);
			current_function = pr.first;
			array_index = 0;
		}
		variable_store[array_index] = variable_mapping[pr.second].current_value + direction(variable_mapping[pr.second].mapped_index) * lambda;
		++array_index;
	}
	current_sum += functions[current_function](variable_store);
	return current_sum;
}

value_type sum_of_functions::gradient_at(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
	IN_P(value_type) location = (value_type*)_alloca(max_function_size * sizeof(value_type));
	IN_P(value_type) direction_a = (value_type*)_alloca(max_function_size * sizeof(value_type));

	size_t outer_index = 0;
	value_type sum = value_type(0.0);

	unsigned short current_function = 0;
	unsigned short array_index = 0;

	for (IN(auto) pr : variable_map) {
		if (pr.first != current_function) {
			sum += function_derivatives[current_function](location, direction_a);

			current_function = pr.first;
			array_index = 0;
		}

		const auto dv = direction(variable_mapping[pr.second].mapped_index);
		location[array_index] = variable_mapping[pr.second].current_value + dv * lambda;
		direction_a[array_index] = dv;

		++array_index;
	}

	sum += function_derivatives[current_function](location, direction_a);

	return sum;
}

std::pair<value_type, value_type> sum_of_functions::evaluate_at_with_derivative(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
	IN_P(value_type) location = (value_type*)_alloca(max_function_size * sizeof(value_type));
	IN_P(value_type) direction_a = (value_type*)_alloca(max_function_size * sizeof(value_type));

	size_t outer_index = 0;
	value_type deriv_sum = value_type(0.0);
	value_type f_sum = value_type(0.0);

	unsigned short current_function = 0;
	unsigned short array_index = 0;

	for (IN(auto) pr : variable_map) {
		if (pr.first != current_function) {
			deriv_sum += function_derivatives[current_function](location, direction_a);
			f_sum += functions[current_function](location);

			current_function = pr.first;
			array_index = 0;
		}

		const auto dv = direction(variable_mapping[pr.second].mapped_index);
		location[array_index] = variable_mapping[pr.second].current_value + dv * lambda;
		direction_a[array_index] = dv;

		++array_index;
	}

	deriv_sum += function_derivatives[current_function](location, direction_a);
	f_sum += functions[current_function](location);

	return std::make_pair(f_sum, deriv_sum);
}
