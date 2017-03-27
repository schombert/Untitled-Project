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

template<typename T>
std::pair<value_type, short> _max_lambda(IN(T) variable_mapping, INOUT(vector_type) direction) {
	short index = -1;
	value_type least = max_value<value_type>::value;

	const auto sz = num_variables(variable_mapping);
	for (decltype(num_variables(variable_mapping)) i = 0; i < sz; ++i) {
		const auto indx = get_variable_index(variable_mapping, i);

		const auto lm = -get_nth_variable(variable_mapping, i) / direction(indx);
		if ((lm < least) & (lm > value_type(0.0)) & (direction(indx) != value_type(0.0))) {
			least = lm;
			index = static_cast<short>(i);
		}
	}

	return std::make_pair(least, index);
}

std::pair<value_type, short> max_lambda(IN(std::vector<var_mapping>) variable_mapping, INOUT(vector_type) direction) {
	return _max_lambda(variable_mapping, direction);
}

std::pair<value_type, short> max_lambda(IN(vector_type) variables, INOUT(vector_type) direction) {
	return _max_lambda(variables, direction);
}

template<typename T>
std::pair<value_type, short> max_lambda_tail(const Eigen::Index start, IN(T) variable_mapping, INOUT(vector_type) direction) {
	short index = -1;
	value_type least = max_value<value_type>::value;

	const auto sz = num_variables(variable_mapping);
	for (size_t i = 0; i < sz; ++i) {
		const auto indx = get_variable_index(variable_mapping, i);
		if (indx > start) {
			const auto lm = -get_nth_variable(variable_mapping, i) / direction(indx);
			if ((lm < least) & (lm > value_type(0.0)) & (direction(indx) != value_type(0.0))) {
				least = lm;
				index = static_cast<short>(i);
			}
		}
	}

	return std::make_pair(least, index);
}


bool zero_element_in_basis(const Eigen::Index basis_size, IN(std::vector<var_mapping>) variable_mapping) {
	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		if ((variable_mapping[i].current_value <= value_type(0.0)) & (variable_mapping[i].mapped_index < basis_size)) {
			return true;
		}
	}
	return false;
}

void setup_rank_map(IN(matrix_type) coeff, IN(std::vector<var_mapping>) variable_mapping, INOUT(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	const auto rows = coeff.rows();
	const auto sz = variable_mapping.size();
	for (size_t i = 0; i < sz; ++i) {
		for (Eigen::Index j = 0; j < rows; ++j) {
			if (coeff(j, variable_mapping[i].mapped_index) != value_type(0.0)) {
				rank_starts.insert(std::make_pair(static_cast<unsigned short>(j), static_cast<unsigned short>(i)));
			}
		}
	}
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

bool singular_column(const Eigen::Index basis_size, const Eigen::Index target_column, const Eigen::Index target_row, INOUT(matrix_type) coeff) {
	for (Eigen::Index i = 0; i < basis_size; ++i) {
		if ((coeff(i, target_column) != value_type(0.0)) == (i != target_row)) {
			return false;
		}
	}
	return true;
}

void optimize_basis(const Eigen::Index basis_size, INOUT(matrix_type) coeff, INOUT(std::vector<var_mapping>) variable_mapping, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	const auto sz = variable_mapping.size();

	for (unsigned short b = 0; b < static_cast<unsigned short>(basis_size); ++b) {
		unsigned short best_fit = max_value<unsigned short>::value;
		unsigned short current_basis = max_value<unsigned short>::value;
		value_type best_value = value_type(0.0);

		for (auto pr = rank_starts.equal_range(b); pr.first != pr.second; ++pr.first) {
			const auto indx = pr.first->second;
			const auto column = variable_mapping[indx].mapped_index;
			if (singular_column(basis_size, column, b, coeff) && abs(value_type(1.0) - coeff(b, column)) <= abs(value_type(1.0) - best_value) && variable_mapping[indx].current_value != value_type(0)) {
				best_fit = indx;
				best_value = coeff(b, column);
			}
			if (variable_mapping[indx].mapped_index == b) {
				current_basis = indx;
			}
		}

		if (best_fit != max_value<unsigned short>::value) {
			const auto best_index = variable_mapping[best_fit].mapped_index;
			coeff.col(b).swap(coeff.col(best_index));
			variable_mapping[best_fit].mapped_index = b;
			variable_mapping[current_basis].mapped_index = best_index;
		}
	}
}

struct limiting_value {
	value_type at;
	value_type φ;
	value_type φ_prime;
	limiting_value() : at(0.0), φ(0.0), φ_prime(0.0) {};
	limiting_value(value_type a, value_type b, value_type c) : at(a), φ(b), φ_prime(c) {};
	limiting_value(value_type a, IN(std::pair<value_type, value_type>) b) : at(a), φ(b.first), φ_prime(b.second) {};
};

template<typename function_class, typename V>
std::pair<limiting_value, limiting_value> hager_zhang_interval_update(IN(function_class) function, IN(V) variable_mapping, IN(vector_type) direction, IN(limiting_value) a, IN(limiting_value) b, IN(limiting_value) c, const value_type φ_zero, const value_type ϵ_k) {
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
		const auto d_at = (value_type(1.0) - θ) * a_temp.at + θ * b_temp.at;
		const limiting_value d(d_at, function.evaluate_at_with_derivative(variable_mapping, d_at, direction));

		if (d.φ_prime >= value_type(0.0))
			return std::make_pair(a_temp, d);
		if (d.φ <= φ_zero + ϵ_k) {
			if (d_at == a_temp.at) // to catch zero interval
				return std::make_pair(a_temp, b_temp);
			a_temp = d;
		} else {
			if (d_at == b_temp.at) // to catch zero interval
				return std::make_pair(a_temp, b_temp);
			b_temp = d;
		}
	}
}

value_type secant(IN(limiting_value) a, IN(limiting_value) b) {
	return b.φ_prime != a.φ_prime ? (a.at * b.φ_prime - b.at * a.φ_prime) / (b.φ_prime - a.φ_prime) : max_value<value_type>::value;
}

template<typename function_class, typename V>
std::pair<limiting_value, limiting_value> hager_zhang_secant_step(IN(function_class) function, IN(V) variable_mapping, IN(vector_type) direction, IN(limiting_value) a, IN(limiting_value) b, const value_type φ_zero, const value_type ϵ_k) {
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

void adjust_variables(INOUT(vector_type) variable_mapping, const value_type λ, IN(vector_type) direction) {
	variable_mapping.noalias() += λ * direction;
}

template<typename V>
void adjust_variables(INOUT(std::vector<var_mapping, V>) variable_mapping, const value_type λ, IN(vector_type) direction) {
	const auto sz = num_variables(variable_mapping);
	for (decltype(num_variables(variable_mapping)) i = 0; i < sz; ++i) {
		inc_nth_variable(variable_mapping, i, direction(get_variable_index(variable_mapping, i)) * λ);
	}
}

void max_adjust_variables(INOUT(vector_type) variable_mapping, IN(vector_type) direction, const value_type λ_max, const short max_index) {
	variable_mapping.noalias() += λ_max * direction;
	if (max_index != -1)
		variable_mapping(max_index) = value_type(0.0);
}

template<typename V>
void max_adjust_variables(INOUT(std::vector<var_mapping, V>) variable_mapping, IN(vector_type) direction, const value_type λ_max, const short max_index) {
	const auto sz = num_variables(variable_mapping);
	for (decltype(num_variables(variable_mapping)) i = 0; i < sz; ++i) {
		inc_nth_variable(variable_mapping, i, direction(get_variable_index(variable_mapping, i)) * λ_max);
	}
	if (max_index != -1)
		set_nth_variable(variable_mapping, max_index, value_type(0.0));
}

void adjust_variables(INOUT(vector_type) variable_mapping, const value_type λ, IN(vector_type) direction, const value_type λ_max, const short max_index) {
	variable_mapping.noalias() += λ * direction;
	if ((λ == λ_max) & (max_index != -1))
		variable_mapping(max_index) = value_type(0.0);
}


template<typename V>
void adjust_variables(INOUT(std::vector<var_mapping, V>) variable_mapping, const value_type λ, IN(vector_type) direction, const value_type λ_max, const short max_index) {
	const auto sz = num_variables(variable_mapping);
	for (decltype(num_variables(variable_mapping)) i = 0; i < sz; ++i) {
		inc_nth_variable(variable_mapping, i, direction(get_variable_index(variable_mapping, i)) * λ);
	}
	if ((λ == λ_max) & (max_index != -1))
		set_nth_variable(variable_mapping, max_index, value_type(0.0));
}

template<typename function_class, typename V>
void update_with_hager_zhang_ls(IN(function_class) function, INOUT(V) variable_mapping, value_type max_lambda, const short max_index, const value_type φ_zero, const value_type φ_prime_zero, IN(vector_type) direction) {
	constexpr value_type ϵ = value_type(0.000001); // range [0, inf) used in the approximate Wolfe conditions
	constexpr value_type γ = value_type(0.66); // range (0, 1) determines when a bisection step is performed
	
	// constexpr value_type η = 0.01; // range (0, inf) used in the lower bound for β

	limiting_value a_k(value_type(0.0), φ_zero, φ_prime_zero);
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

	} while(!satisfies_hager_zhang_conditions<100,900>(b_k, φ_zero, φ_prime_zero, ϵ_k) & (std::nextafter(a_k.at, max_value<value_type>::value) < b_k.at)); // conditions not satisfied 

	adjust_variables(variable_mapping, b_k.at, direction, max_lambda, max_index);
}

std::string fp_output_helper(value_type f) {
	char* buffer = (char*)_alloca(100);
	snprintf(buffer, 100, "%.9g", f);
	return std::string(buffer);
}


template<typename function_class, typename V>
void update_with_derivitive_minimization_ls(IN(function_class) function, INOUT(V) variable_mapping, const value_type max_lambda, const short max_index, const value_type φ_a0, const value_type φ_prime_a0, IN(vector_type) direction) {
	constexpr int δ_times_1000 = 100;
	constexpr int σ_times_1000 = 900;
	constexpr value_type ϵ = value_type(0.000001); // range [0, inf) used in the approximate Wolfe conditions

	limiting_value a_k = limiting_value(value_type(0.0), φ_a0, φ_prime_a0);
	limiting_value b_k = limiting_value(max_lambda, function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction));

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
		const limiting_value c_k = (((c_k_at * value_type(0.9)) <= a_k.at) | (c_k_at >= (b_k.at * value_type(0.9)) )) ?
			limiting_value((a_k.at + b_k.at) * value_type(0.5), function.evaluate_at_with_derivative(variable_mapping, (a_k.at + b_k.at) * value_type(0.5), direction))
			: limiting_value(c_k_at, function.evaluate_at_with_derivative(variable_mapping, c_k_at, direction));
		if (c_k.φ_prime >= 0) {
			b_k = c_k;
		} else {
			a_k = c_k;
		}
		ϵ_k = ϵ * abs(b_k.φ);
	}

	adjust_variables(variable_mapping, b_k.at, direction, max_lambda, max_index);
}

template<typename function_class, typename V>
void update_with_derivative_interpolation_ls(IN(function_class) function, INOUT(V) variable_mapping, value_type max_lambda, const short max_index, const value_type φ_a0, const value_type φ_prime_a0, IN(vector_type) direction) {
	constexpr value_type δ = value_type(0.1);

	limiting_value a_k(max_lambda, function.evaluate_at_with_derivative(variable_mapping, max_lambda, direction));

	if (a_k.φ <= (δ * max_lambda * φ_prime_a0) + φ_a0) {
		max_adjust_variables(variable_mapping, direction, max_lambda, max_index);
		return;
	}

	while (true) {
		const value_type d_1 = φ_prime_a0 + a_k.φ_prime - value_type(3) * (φ_a0 - a_k.φ) / (-a_k.at);
		const value_type d_2 = sqrt(d_1*d_1 - φ_prime_a0 * a_k.φ_prime);

		const auto denom = a_k.φ_prime - φ_prime_a0 + value_type(2) * d_2;
		const value_type a_new_at = a_k.at - (a_k.at) * ((a_k.φ_prime + d_2 - d_1) / denom);
		//const auto a_new = limiting_value(a_new_at, function.evaluate_at_with_derivative(variable_mapping, a_new_at, direction));

#ifndef SAFETY_OFF
		/*if (isnan(a_new_at)) {
			std::string rstr = std::string("d_1: ") + fp_output_helper(d_1) + ", d_2: " + fp_output_helper(d_2) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("a_k_at: ") + fp_output_helper(a_k.at) + ", a_k_phi: " + fp_output_helper(a_k.φ) + ", phi_prime_a_k:" + fp_output_helper(a_k.φ_prime) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("phi_prime_a0: ") + fp_output_helper(φ_prime_a0) + ", phi_a0: " + fp_output_helper(φ_a0) + ", denom:" + fp_output_helper(denom) + "\n";
			OutputDebugStringA(rstr.c_str());
			abort();
		}*/
#endif // !SAFETY_OFF

		if (a_k.at - a_new_at > value_type(0.9) * a_k.at || a_k.at - a_new_at < value_type(0.1) * a_k.at || !std::isfinite(a_new_at)) {
			const value_type a_new_at_b = value_type(0.5) * a_k.at;
			a_k = limiting_value(a_new_at_b, function.evaluate_at_with_derivative(variable_mapping, a_new_at_b, direction));
		} else {
			a_k = limiting_value(a_new_at, function.evaluate_at_with_derivative(variable_mapping, a_new_at, direction));
		}

		if ((a_k.φ <= (δ * a_k.at * φ_prime_a0) + φ_a0) & (a_k.φ_prime <= 0)) {
			adjust_variables(variable_mapping, a_k.at, direction);
			return;
		}
	}
}

template<typename function_class, typename V>
void update_with_interpolation_ls(IN(function_class) function, INOUT(V) variable_mapping, value_type max_lambda, const short max_index, const value_type φ_a0, const value_type φ_prime_a0, IN(vector_type) direction) {
	constexpr value_type δ = value_type(0.1);
	
	value_type a_k_at = max_lambda;
	limiting_value a_k = limiting_value(a_k_at, function.evaluate_at_with_derivative(variable_mapping, a_k_at, direction));

	if (a_k.φ <= (δ * max_lambda * φ_prime_a0) + φ_a0) {
		max_adjust_variables(variable_mapping, direction, max_lambda, max_index);
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
		adjust_variables(variable_mapping, a_k.at, direction);
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
		 /* if (isnan(a_new_at)) {
			std::string rstr = std::string("multiplier: ") + fp_output_helper(multiplier) + ", a_k_at: " + fp_output_helper(a_k.at) + ", a_k_last_at:" + fp_output_helper(a_last.at) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("a: ") + fp_output_helper(a) + ", b: " + fp_output_helper(b) + ", phi_prime_a0:" + fp_output_helper(φ_prime_a0) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("m_1: ") + fp_output_helper(m_1) + ", m_2: " + fp_output_helper(m_2) + ", phi_a0:" + fp_output_helper(φ_a0) + "\n";
			OutputDebugStringA(rstr.c_str());
			rstr = std::string("b * b: ") + fp_output_helper(b * b) + ", rest: " + fp_output_helper(value_type(3.0) * a * φ_prime_a0) + ", sqrt of:" + fp_output_helper(b * b - value_type(3.0) * a * φ_prime_a0) + "\n";
			OutputDebugStringA(rstr.c_str());
			abort();
		} */
#endif // !SAFETY_OFF

		a_last = a_k;

		if (a_k.at >= value_type(0.9) * a_k.at + a_new_at || a_k.at <= value_type(0.1) * a_k.at + a_new_at || !std::isfinite(a_new_at)) {
			a_k = limiting_value(value_type(0.5) * a_k.at, function.evaluate_at_with_derivative(variable_mapping, value_type(0.5) * a_k.at, direction));
		} else {
			a_k = limiting_value(a_new_at, function.evaluate_at_with_derivative(variable_mapping, a_new_at, direction));
		}

		if ((a_k.φ <= (δ * a_k.at * φ_prime_a0) + φ_a0) & (a_k.φ_prime <= 0)) {
			adjust_variables(variable_mapping, a_k.at, direction);
			return;
		}
	}
}

template<typename function_class, typename V>
void update_with_backtrack_ls(IN(function_class) function, INOUT(V) variable_mapping, const value_type lambda, const short max_index, const value_type φ_a0, const value_type φ_prime_a0, IN(vector_type) direction) {
	constexpr value_type δ = value_type(0.100);

	limiting_value a_k = limiting_value(lambda, function.evaluate_at_with_derivative(variable_mapping, lambda, direction));

	if (a_k.φ <= φ_a0 + δ * a_k.at * φ_prime_a0) {
		max_adjust_variables(variable_mapping, direction, lambda, max_index);
		return;
	}

	a_k = limiting_value(a_k.at * value_type(0.5), function.evaluate_at_with_derivative(variable_mapping, a_k.at * value_type(0.5), direction));

	do {
		if((a_k.φ <= φ_a0 + δ * a_k.at * φ_prime_a0) & (a_k.φ_prime <= 0)) {
			adjust_variables(variable_mapping, a_k.at, direction);
			return;
		}

		a_k = limiting_value(a_k.at * value_type(0.5), function.evaluate_at_with_derivative(variable_mapping, a_k.at * value_type(0.5), direction));
	} while (a_k.at != 0);

	OutputDebugStringA("zero lambda");
}

template<typename function_class, typename V>
using ls_function_type = void(*)(IN(function_class), INOUT(V), value_type, const short, const value_type, const value_type, IN(vector_type));

template<ls_function_type<linear_test_function, std::vector<var_mapping>> LINE_SEARCH>
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

		if ((direction(0) < 0 && variable[0].current_value == value_type(0.0)) ||
			(direction(0) > 0 && variable[0].current_value == max_v) ||
			(abs(direction(0)) < 0.0000001)) {
			break;
		}

		if (direction(0) < 0.0) {
			value_type lm_max = -variable[0].current_value / direction(0);
			LINE_SEARCH(func, variable, lm_max, 0, func.evaluate_at(variable), gradient(0)*direction(0), direction);
		} else {
			value_type lm_max = (max_v - variable[0].current_value) / direction(0);
			LINE_SEARCH(func, variable, lm_max, 0, func.evaluate_at(variable), gradient(0)*direction(0), direction);
			if (variable[0].current_value == value_type(0.0))
				variable[0].current_value = max_v;
		}
		++count;

		

#ifndef SAFETY_OFF
		if (count > 30) {
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

template<typename function_class, ls_function_type<function_class, std::vector<var_mapping>> LINE_SEARCH, reduced_gradient_to_direction_map_type RG_MAP>
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

		LINE_SEARCH(function, variable_mapping, lm_max.first, lm_max.second, function.evaluate_at(variable_mapping), m, direction);

#ifndef SAFETY_OFF
		if (++iteration_count > 65) {
			//OutputDebugStringA("too many iterations (sd)\n");
			break;
			//abort();
		}
#endif
	} while (true);
}

template<typename function_class, ls_function_type<function_class, std::vector<var_mapping>> LINE_SEARCH>
void barrier_steepest_descent(INOUT(function_class) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	const auto b_size = coeff.rows();
	const auto full = coeff.cols();
	const auto remainder = full - b_size;

	constexpr value_type ϵ = value_type(0.0001); // range [0, inf)
	value_type τ = value_type(0.5);

	rvector_type gradient((value_type*)_alloca(full * sizeof(value_type)), full);
	vector_type direction((value_type*)_alloca(full * sizeof(value_type)), full);
	rvector_type reduced_n_gradient((value_type*)_alloca(remainder * sizeof(value_type)), remainder);

	optimize_basis(b_size, coeff, variable_mapping, rank_starts);

	matrix_type LU_store((value_type*)_alloca(sizeof(value_type) * b_size * b_size), b_size, b_size);
	LU_store = coeff.leftCols(b_size);
	Eigen::PartialPivLU<Eigen::Ref<matrix_type>> LU_decomp(LU_store);

	const bool is_upper_triangular = coeff.leftCols(b_size).isUpperTriangular();
	function.μ = value_type(0.5);
	function.n = b_size;
	function.λ = (value_type*)_alloca(b_size * sizeof(value_type));
	for (Eigen::Index i = 0; i < b_size; ++i)
		function.λ[i] = value_type(0.0);


#ifndef SAFETY_OFF
	size_t iteration_count = 0;
#endif

	value_type prev_function_value = max_value<double>::value;

	//inside iteration
	do {
		gradient.noalias() = e_row_vector::Zero(full);
		function.gradient_at(variable_mapping, gradient);

		if (is_upper_triangular) {
			reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
			caclulate_reduced_n_gradient_ut(b_size, remainder, coeff, gradient, reduced_n_gradient);

			direction.noalias() = e_vector::Zero(full);
			calcuate_n_direction_steepest(b_size, remainder, variable_mapping, reduced_n_gradient, direction);

			calcuate_b_direction_ut(b_size, remainder, coeff, direction);
		} else {
			reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
			caclulate_reduced_n_gradient_LU(b_size, remainder, coeff, LU_decomp, gradient, reduced_n_gradient);

			direction.noalias() = e_vector::Zero(full);
			calcuate_n_direction_steepest(b_size, remainder, variable_mapping, reduced_n_gradient, direction);

			calcuate_b_direction_LU(b_size, remainder, coeff, LU_decomp, direction);
		}

		// test for satisfaction
		const auto sn = direction.squaredNorm();
		if (sn <= τ*τ) {
			// do final convergence test
			bool satisfactory_values = true;
			for (IN(auto) v : variable_mapping) {
				if (v.current_value <= -ϵ)
					satisfactory_values = false;
			}
			if (satisfactory_values & (sn <= ϵ*ϵ))
					break;

			for (IN(auto) v : variable_mapping) {
				if (v.mapped_index < b_size) {
					function.λ[v.mapped_index] = std::max(function.λ[v.mapped_index] - v.current_value / function.μ, value_type(0));
				}
			}
			function.μ *= value_type(0.75);
			τ = std::max(τ * value_type(0.05), ϵ);
			continue;
		}

		// perform line search, update solution
		const auto lm_max = max_lambda_tail(b_size, variable_mapping, direction);
		const value_type m = (gradient * direction)(0, 0);

		//const auto new_function_value = function.evaluate_at(variable_mapping);
		//if (new_function_value >= prev_function_value)
		//	break;
		//prev_function_value = new_function_value;

		if(lm_max.second != -1)
			if(lm_max.first <= 128.0 * 128.0 * 128.0)
				LINE_SEARCH(function, variable_mapping, lm_max.first, lm_max.second, function.evaluate_at(variable_mapping), m, direction);
			else
				LINE_SEARCH(function, variable_mapping, 128.0 * 128.0 * 128.0, -1, function.evaluate_at(variable_mapping), m, direction);
		else
			LINE_SEARCH(function, variable_mapping, 128.0, lm_max.second, function.evaluate_at(variable_mapping), m, direction);

#ifndef SAFETY_OFF
		if (++iteration_count > 65) {
			//OutputDebugStringA("too many iterations (bsd)\n");
			break;
			//abort();
		}
#endif
	} while (true);

	//std::string cnt = std::to_string(iteration_count) + "\n";
	//OutputDebugStringA(cnt.c_str());
}

template<typename function_class, ls_function_type<function_class, vector_type> LINE_SEARCH>
void multiplier_steepest_descent(INOUT(function_class) function, INOUT(vector_type) variables) {
	const auto b_size = function.coeff.rows();
	const auto full = function.coeff.cols();

	constexpr value_type ϵ = value_type(0.0001); // range [0, inf)
	value_type τ = value_type(0.5);

	rvector_type gradient((value_type*)_alloca(full * sizeof(value_type)), full);
	vector_type direction((value_type*)_alloca(full * sizeof(value_type)), full);


	function.coeff_totals.noalias() = function.coeff * variables;
	function.μ = value_type(0.5);
	function.λ = vector_type::Zero(b_size);


#ifndef SAFETY_OFF
	size_t iteration_count = 0;
#endif

	value_type prev_function_value = max_value<double>::value;

	//inside iteration
	do {
		gradient.noalias() = e_row_vector::Zero(full);
		function.gradient_at(variables, gradient);
		direction = -gradient;

		for (Eigen::Index i = 0; i < full; ++i) {
			if ((direction(i) < value_type(0.0)) & (variables(i) == value_type(0.0))) {
				direction(i) = value_type(0.0);
			}
		}

		// test for satisfaction
		const auto sn = direction.squaredNorm();
		if (sn <= τ*τ) {
			// do final convergence test

			vector_type difference((value_type*)_alloca(sizeof(value_type) * b_size), b_size);
			difference.noalias() = function.coeff * variables - function.coeff_totals;

			if ((difference.squaredNorm() <= ϵ*ϵ) & (sn <= ϵ*ϵ))
				break;

			function.λ -= difference / function.μ;
			function.μ *= value_type(0.94);
			τ = std::max(τ * value_type(0.5), ϵ);
			continue;
		}

		// perform line search, update solution
		const auto lm_max = max_lambda(variables, direction);
		const value_type m = (gradient * direction)(0, 0);

		//const auto new_function_value = function.evaluate_at(variable_mapping);
		//if (new_function_value >= prev_function_value)
		//	break;
		//prev_function_value = new_function_value;

		if (lm_max.second != -1) {
			if (lm_max.first <= 128.0 * 128.0 * 128.0)
				LINE_SEARCH(function, variables, lm_max.first, lm_max.second, function.evaluate_at(variables), m, direction);
			else
				LINE_SEARCH(function, variables, 128.0 * 128.0 * 128.0, -1, function.evaluate_at(variables), m, direction);
		} else {
			LINE_SEARCH(function, variables, 128.0, lm_max.second, function.evaluate_at(variables), m, direction);
		}

#ifndef SAFETY_OFF
		if (++iteration_count > 300) {
			//OutputDebugStringA("too many iterations (msd)\n");
			break;
			//abort();
		}
#endif
	} while (true);

	//std::string cnt = std::to_string(iteration_count) + "\n";
	//OutputDebugStringA(cnt.c_str());
}

using restart_conditions_type = bool(*)(size_t, size_t, IN(rvector_type), IN(rvector_type), IN(rvector_type));

bool basic_restart_condition(size_t restart_count, size_t variable_count, IN(rvector_type) new_cg, IN(rvector_type) previous_gradient, IN(rvector_type) current_gradient) {
	return (restart_count >= variable_count);
}

bool ensure_descent_direction(size_t restart_count, size_t variable_count, IN(rvector_type) new_cg, IN(rvector_type) previous_gradient, IN(rvector_type) current_gradient) {
	return (restart_count >= variable_count) || (new_cg.transpose() * current_gradient)(0, 0) <= 0.0;
}

bool no_restart_condition(size_t restart_count, size_t variable_count, IN(rvector_type) new_cg, IN(rvector_type) previous_gradient, IN(rvector_type) current_gradient) {
	return false;
}

bool powell_restart_condition(size_t restart_count, size_t variable_count, IN(rvector_type) new_cg, IN(rvector_type) previous_gradient, IN(rvector_type) current_gradient) {
	const auto d_g = -((new_cg.transpose() * current_gradient)(0, 0));
	const auto current_sn = current_gradient.squaredNorm();
	return (restart_count >= variable_count)
		|| abs((previous_gradient.transpose() * current_gradient)(0,0)) >= value_type(0.2) * current_sn || d_g >= value_type(0.0)
		|| (restart_count >= 2 && !(value_type(-1.2) * current_sn <= d_g && d_g <= value_type(-0.8) * current_sn));
}

value_type feasible_norm_squared(const Eigen::Index base, IN(std::vector<var_mapping>) variable_mapping, IN(rvector_type) reduced_n_gradient) {
	value_type sum = value_type(0.0);
	for (IN(auto) v : variable_mapping) {
		if (v.mapped_index >= base && (0 != ((v.current_value > value_type(0.0)) | (reduced_n_gradient(v.mapped_index - base) < value_type(0.0))))) {
			sum += reduced_n_gradient(v.mapped_index - base) * reduced_n_gradient(v.mapped_index - base);
		}
	}
	return sum;
}

template<typename function_class, ls_function_type<function_class, std::vector<var_mapping>> LINE_SEARCH, cg_function_type CG_FUNCTION, restart_conditions_type RESTART_FUNCTION>
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
	size_t iteration_count = 1;
#endif

	size_t restart_count = 0;

	value_type prev_function_value(0);

	//first iteration: steepest descent
	{
		if (conditional_maximize_basis(b_size, coeff, variable_mapping, rank_starts)) {
			is_upper_triangular = coeff.leftCols(b_size).isUpperTriangular();

			if (!is_upper_triangular) {
				LU_store.noalias() = coeff.leftCols(b_size);
				new (&LU_decomp) Eigen::PartialPivLU<Eigen::Ref<matrix_type>>(LU_store);
			}
		}
		gradient.noalias() = e_row_vector::Zero(full);

		function.gradient_at(variable_mapping, gradient);
		reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
		if (is_upper_triangular)
			caclulate_reduced_n_gradient_ut(b_size, remainder, coeff, gradient, reduced_n_gradient);
		else
			caclulate_reduced_n_gradient_LU(b_size, remainder, coeff, LU_decomp, gradient, reduced_n_gradient);
		new_cg.noalias() = reduced_n_gradient;
		prev_reduced_n_gradient.noalias() = reduced_n_gradient;
		prev_cg.noalias() = reduced_n_gradient;

		direction.noalias() = e_vector::Zero(full);
		calcuate_n_direction_steepest(b_size, remainder, variable_mapping, new_cg, direction);
		if (is_upper_triangular)
			calcuate_b_direction_ut(b_size, remainder, coeff, direction);
		else
			calcuate_b_direction_LU(b_size, remainder, coeff, LU_decomp, direction);

		const auto lm_max = max_lambda(variable_mapping, direction);

#ifndef SAFETY_OFF
		if (lm_max.second == -1)
			abort();
#endif
		const value_type m = (gradient * direction)(0, 0);
		LINE_SEARCH(function, variable_mapping, lm_max.first, lm_max.second, prev_function_value = function.evaluate_at(variable_mapping), m, direction);
	}

	//second + iteration
	do {

		/*
		if (variable_mapping.size() > 6) {
			std::string rstr = std::string("iteration ") + std::to_string(iteration_count) + ": " + fp_output_helper(function.evaluate_at(variable_mapping)) + "\n";
			OutputDebugStringA(rstr.c_str());
		}
		/**/

		if (conditional_maximize_basis(b_size, coeff, variable_mapping, rank_starts)) {
			is_upper_triangular = coeff.leftCols(b_size).isUpperTriangular();

			if (!is_upper_triangular) {
				LU_store.noalias() = coeff.leftCols(b_size);
				new (&LU_decomp) Eigen::PartialPivLU<Eigen::Ref<matrix_type>>(LU_store);
			}

			gradient.noalias() = e_row_vector::Zero(full);

			function.gradient_at(variable_mapping, gradient);
			reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
			if (is_upper_triangular)
				caclulate_reduced_n_gradient_ut(b_size, remainder, coeff, gradient, reduced_n_gradient);
			else
				caclulate_reduced_n_gradient_LU(b_size, remainder, coeff, LU_decomp, gradient, reduced_n_gradient);

			new_cg.noalias() = reduced_n_gradient;
			restart_count = 0;
		} else {
			gradient.noalias() = e_row_vector::Zero(full);

			function.gradient_at(variable_mapping, gradient);
			reduced_n_gradient.noalias() = e_row_vector::Zero(remainder);
			if (is_upper_triangular)
				caclulate_reduced_n_gradient_ut(b_size, remainder, coeff, gradient, reduced_n_gradient);
			else
				caclulate_reduced_n_gradient_LU(b_size, remainder, coeff, LU_decomp, gradient, reduced_n_gradient);

			new_cg.noalias() = reduced_n_gradient + CG_FUNCTION(reduced_n_gradient, prev_reduced_n_gradient, prev_cg) * prev_cg;

			++restart_count;
			if (RESTART_FUNCTION(restart_count, remainder, new_cg, prev_reduced_n_gradient, reduced_n_gradient)) {
				restart_count = 0;
				new_cg.noalias() = reduced_n_gradient;
			}
		}

		prev_reduced_n_gradient.noalias() = reduced_n_gradient;
		prev_cg.noalias() = new_cg;

		
		direction.noalias() = e_vector::Zero(full);
		calcuate_n_direction_steepest(b_size, remainder, variable_mapping, new_cg, direction);
		if (is_upper_triangular)
			calcuate_b_direction_ut(b_size, remainder, coeff, direction);
		else
			calcuate_b_direction_LU(b_size, remainder, coeff, LU_decomp, direction);

		// test for satisfaction
		if (feasible_norm_squared(b_size, variable_mapping, reduced_n_gradient) <= ϵ*ϵ)
			break;

		// perform line search, update solution
		const auto lm_max = max_lambda(variable_mapping, direction);

#ifndef SAFETY_OFF
	//	if (lm_max.second == -1)
	//		abort();
#endif
		//if (lm_max.second != -1) {
			// otherwise : no feasible direction was produced
		const value_type m = (gradient * direction)(0, 0);
		const auto new_function_value = function.evaluate_at(variable_mapping);
		if (restart_count == 0) {
			if (new_function_value >= prev_function_value)
				break; // no progress after a full round of cg
			prev_function_value = new_function_value;
		}
		if(m < 0.0) {
			LINE_SEARCH(function, variable_mapping, lm_max.first, lm_max.second, new_function_value, m, direction);

		}

#ifndef SAFETY_OFF
		if (++iteration_count > 100) {
			OutputDebugStringA("hit iteration limit (cg)\n");
			break;
		}
#endif
	} while (true);
}

/*
cg_hestenes_stiefel
cg_hager_zhang
cg_polak_ribiere
cg_polak_ribiere_plus
cg_fletcher_reeves
*/

void sof_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_hager_zhang_ls, cg_hager_zhang, basic_restart_condition>(function, variable_mapping, coeff, rank_starts);
}

void sof_hz_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivitive_minimization_ls, cg_hager_zhang, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_hz_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_backtrack_ls, cg_hager_zhang, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_hz_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_interpolation_ls, cg_hager_zhang, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_hz_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivative_interpolation_ls, cg_hager_zhang, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_hs_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_hager_zhang_ls, cg_hestenes_stiefel, basic_restart_condition>(function, variable_mapping, coeff, rank_starts);
}

void sof_hs_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivitive_minimization_ls, cg_hestenes_stiefel, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_hs_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_backtrack_ls, cg_hestenes_stiefel, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_hs_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_interpolation_ls, cg_hestenes_stiefel, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_hs_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivative_interpolation_ls, cg_hestenes_stiefel, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_pr_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_hager_zhang_ls, cg_polak_ribiere, basic_restart_condition>(function, variable_mapping, coeff, rank_starts);
}

void sof_pr_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivitive_minimization_ls, cg_polak_ribiere, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_pr_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_backtrack_ls, cg_polak_ribiere, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_pr_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_interpolation_ls, cg_polak_ribiere, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_pr_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivative_interpolation_ls, cg_polak_ribiere, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_prp_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_hager_zhang_ls, cg_polak_ribiere_plus, basic_restart_condition>(function, variable_mapping, coeff, rank_starts);
}

void sof_prp_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivitive_minimization_ls, cg_polak_ribiere_plus, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_prp_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_backtrack_ls, cg_polak_ribiere_plus, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_prp_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_interpolation_ls, cg_polak_ribiere_plus, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_prp_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivative_interpolation_ls, cg_polak_ribiere_plus, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_fr_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_hager_zhang_ls, cg_fletcher_reeves, basic_restart_condition>(function, variable_mapping, coeff, rank_starts);
}

void sof_fr_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivitive_minimization_ls, cg_fletcher_reeves, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_fr_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_backtrack_ls, cg_fletcher_reeves, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_fr_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_interpolation_ls, cg_fletcher_reeves, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
}

void sof_fr_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	conjugate_gradient_method<sum_of_functions, update_with_derivative_interpolation_ls, cg_fletcher_reeves, ensure_descent_direction>(function, variable_mapping, coeff, rank_starts);
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


void sof_hz_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	barrier_steepest_descent<sum_of_functions_b, update_with_hager_zhang_ls>(function, variable_mapping, coeff, rank_starts);
}

void sof_dm_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	barrier_steepest_descent<sum_of_functions_b, update_with_derivitive_minimization_ls>(function, variable_mapping, coeff, rank_starts);
}

void sof_bt_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	barrier_steepest_descent<sum_of_functions_b, update_with_backtrack_ls>(function, variable_mapping, coeff, rank_starts);
}

void sof_int_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	barrier_steepest_descent<sum_of_functions_b, update_with_interpolation_ls>(function, variable_mapping, coeff, rank_starts);
}

void sof_dint_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts) {
	barrier_steepest_descent<sum_of_functions_b, update_with_derivative_interpolation_ls>(function, variable_mapping, coeff, rank_starts);
}

void sof_hz_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables) {
	multiplier_steepest_descent<lm_sum_of_functions, update_with_hager_zhang_ls>(function, variables);
}

void sof_dm_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables) {
	multiplier_steepest_descent<lm_sum_of_functions, update_with_derivitive_minimization_ls>(function, variables);
}

void sof_bt_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables) {
	multiplier_steepest_descent<lm_sum_of_functions, update_with_backtrack_ls>(function, variables);
}

void sof_int_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables) {
	multiplier_steepest_descent<lm_sum_of_functions, update_with_interpolation_ls>(function, variables);
}

void sof_dint_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables) {
	multiplier_steepest_descent<lm_sum_of_functions, update_with_derivative_interpolation_ls>(function, variables);
}

value_type sum_of_functions_b::evaluate_at(IN(std::vector<var_mapping>) variable_mapping) const {
	auto eval = sum_of_functions::evaluate_at(variable_mapping);
	for (IN(auto) v : variable_mapping) {
		if (v.mapped_index < n) {
			eval -= v.current_value <= μ * λ[v.mapped_index] ? 
				(λ[v.mapped_index] * v.current_value - v.current_value * v.current_value * value_type(0.5) / μ) : 
				(μ * λ[v.mapped_index]  * λ[v.mapped_index] * value_type(0.5));
		}
	}
	return eval;
}

void sum_of_functions_b::gradient_at(IN(std::vector<var_mapping>) variable_mapping, INOUT(rvector_type) gradient) const {
	sum_of_functions::gradient_at(variable_mapping, gradient);
	for (IN(auto) v : variable_mapping) {
		if (v.mapped_index < n && v.current_value <= μ * λ[v.mapped_index]) {
			gradient(v.mapped_index) -= (λ[v.mapped_index] - v.current_value / μ);
		}
	}
}

void sum_of_functions_b::un_modified_gradient_at(IN(std::vector<var_mapping>) variable_mapping, INOUT(rvector_type) gradient) const {
	sum_of_functions_b::gradient_at(variable_mapping, gradient);
}

value_type sum_of_functions_b::evaluate_at(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
	auto sum = sum_of_functions::evaluate_at(variable_mapping, lambda, direction);
	for (IN(auto) v : variable_mapping) {
		if (v.mapped_index < n) {
			const auto actual_value = v.current_value + lambda *  direction(v.mapped_index);
			sum -= actual_value <= μ * λ[v.mapped_index] ?
				(λ[v.mapped_index] * actual_value - actual_value * actual_value * value_type(0.5) / μ) :
				(μ * λ[v.mapped_index] * λ[v.mapped_index] * value_type(0.5));
		}
	}
	return sum;
}

value_type sum_of_functions_b::gradient_at(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
	auto sum = sum_of_functions::gradient_at(variable_mapping, lambda, direction);
	for (IN(auto) v : variable_mapping) {
		if (v.mapped_index < n) {
			const auto direction_component = direction(v.mapped_index);
			const auto actual_value = v.current_value + lambda *  direction_component;
			if(actual_value <= μ * λ[v.mapped_index])
				sum -= (λ[v.mapped_index] - actual_value / μ) *  direction_component;
		}
	}
	return sum;
}

std::pair<value_type, value_type> sum_of_functions_b::evaluate_at_with_derivative(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
	auto rpair = sum_of_functions::evaluate_at_with_derivative(variable_mapping, lambda, direction);
	for (IN(auto) v : variable_mapping) {
		if (v.mapped_index < n) {
			const auto direction_component = direction(v.mapped_index);
			const auto actual_value = v.current_value + lambda *  direction_component;

			if (actual_value <= μ * λ[v.mapped_index]) {
				rpair.first -= (λ[v.mapped_index] * actual_value - actual_value * actual_value * value_type(0.5) / μ);
				rpair.second -= (λ[v.mapped_index] - actual_value / μ) *  direction_component;
			} else {
				rpair.first -= (μ * λ[v.mapped_index] * λ[v.mapped_index] * value_type(0.5));
			}
		}
	}
	return rpair;
}

value_type lm_sum_of_functions::evaluate_at(IN(vector_type) variables) const {
	const auto rows = coeff.rows();
	vector_type difference((value_type*)_alloca(sizeof(value_type) * rows), rows);
	difference.noalias() = coeff * variables - coeff_totals;

	return sum_of_functions_t<vector_type>::evaluate_at(variables) - (λ.transpose() * difference)(0, 0) + difference.squaredNorm() * value_type(0.5) / μ;
}

void lm_sum_of_functions::gradient_at(IN(vector_type) variables, INOUT(rvector_type) gradient) const {
	const auto rows = coeff.rows();
	vector_type difference((value_type*)_alloca(sizeof(value_type) * rows), rows);
	difference.noalias() = coeff * variables - coeff_totals;

	sum_of_functions_t<vector_type>::gradient_at(variables, gradient);
	gradient -= (λ - (difference / μ)).transpose() * coeff;
}

value_type lm_sum_of_functions::evaluate_at(IN(vector_type) variables, value_type lambda, IN(vector_type) direction) const {
	const auto rows = coeff.rows();
	vector_type difference((value_type*)_alloca(sizeof(value_type) * rows), rows);
	difference.noalias() = coeff * (variables + (lambda * direction)) - coeff_totals;

	return sum_of_functions_t<vector_type>::evaluate_at(variables, lambda, direction) - (λ.transpose() * difference)(0, 0) + difference.squaredNorm() * value_type(0.5) / μ;
}

value_type lm_sum_of_functions::gradient_at(IN(vector_type) variables, value_type lambda, IN(vector_type) direction) const {
	const auto rows = coeff.rows();
	vector_type difference((value_type*)_alloca(sizeof(value_type) * rows), rows);
	difference.noalias() = coeff * (variables + (lambda * direction)) - coeff_totals;

	return sum_of_functions_t<vector_type>::gradient_at(variables, lambda, direction) - ((λ - (difference / μ)).transpose() * coeff * direction)(0, 0);
}

std::pair<value_type, value_type> lm_sum_of_functions::evaluate_at_with_derivative(IN(vector_type) variables, value_type lambda, IN(vector_type) direction) const {
	const auto rows = coeff.rows();
	vector_type difference((value_type*)_alloca(sizeof(value_type) * rows), rows);
	difference.noalias() = coeff * (variables + (lambda * direction)) - coeff_totals;

	const auto rpair = sum_of_functions_t<vector_type>::evaluate_at_with_derivative(variables, lambda, direction);
	return std::make_pair(rpair.first - (λ.transpose() * difference)(0, 0) + difference.squaredNorm() * value_type(0.5) / μ,
		rpair.second - ((λ - (difference / μ)).transpose() * coeff * direction)(0, 0));
}