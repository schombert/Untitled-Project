#pragma once
#include "globalhelpers.h"

#ifdef SAFETY_OFF
#define EIGEN_NO_DEBUG
#else
#define EIGEN_NO_MALLOC
#endif

#define EIGEN_MPL2_ONLY

#include "Dense"

using value_type = double;

using e_vector = Eigen::Matrix<value_type, Eigen::Dynamic, 1>;
using e_row_vector = Eigen::Matrix<value_type, 1, Eigen::Dynamic>;
using e_matrix = Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic>;
using rvector_type = Eigen::Map<e_row_vector, Eigen::Aligned>;
using vector_type = Eigen::Map<e_vector, Eigen::Aligned>;
using matrix_type = Eigen::Map<e_matrix, Eigen::Aligned>;

struct var_mapping {
	value_type current_value;
	static unsigned short get_index(IN(var_mapping) v, size_t i) {
		return v.mapped_index;
	}
	unsigned short mapped_index;
};

template<typename T>
inline unsigned short get_variable_index(IN(std::vector<var_mapping, T>) v, size_t i) {
	return v[i].mapped_index;
}
template<typename T>
inline size_t get_variable_index(IN(T), size_t i) {
	return i;
}

template<typename T>
inline size_t num_variables(IN(std::vector<var_mapping, T>) v) {
	return v.size();
}
inline Eigen::Index num_variables(IN(vector_type) v) {
	return v.rows();
}

template<typename T>
inline value_type get_nth_variable(IN(std::vector<var_mapping, T>) v, size_t i) {
	return v[i].current_value;
}
inline value_type get_nth_variable(IN(vector_type) v, Eigen::Index i) {
	return v(i);
}

template<typename T>
inline void set_nth_variable(INOUT(std::vector<var_mapping, T>) v, size_t i, value_type e) {
	v[i].current_value = e;
}
inline void set_nth_variable(INOUT(vector_type) v, Eigen::Index i, value_type e) {
	v(i) = e;
}

template<typename T>
inline void inc_nth_variable(INOUT(std::vector<var_mapping, T>) v, size_t i, value_type e) {
	v[i].current_value += e;
}
inline void inc_nth_variable(INOUT(vector_type) v, Eigen::Index i, value_type e) {
	v(i) += e;
}

class linear_test_function {
private:
	const std::function<value_type(value_type)> f;
	const std::function<value_type(value_type)> f_prime;
public:
	linear_test_function(IN(std::function<value_type(value_type)>) fin, IN(std::function<value_type(value_type)>) fpin) : f(fin), f_prime(fpin) {};
	value_type evaluate_at(INOUT(std::vector<var_mapping>) variable_mapping) const {
		return f(variable_mapping[0].current_value);
	}
	void gradient_at(IN(std::vector<var_mapping>) variable_mapping, INOUT(rvector_type) gradient) const {
		gradient(0) = f_prime(variable_mapping[0].current_value);
	}
	value_type evaluate_at(INOUT(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
		return f(variable_mapping[0].current_value +
			lambda * direction(Eigen::Index(0)));
	}
	value_type gradient_at(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
		return f_prime(variable_mapping[0].current_value + lambda * direction(0));
	}
	std::pair<value_type, value_type> evaluate_at_with_derivative(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const {
		const auto x = variable_mapping[0].current_value + lambda * direction(0);
		return std::make_pair(f(x), f_prime(x) * direction(0));
	}
};

template<typename T>
class sum_of_functions_t {
private:
	std::vector<std::pair<unsigned short, unsigned short>> variable_map;
	std::vector<std::function<value_type(const value_type* const)>> functions;
	std::vector<std::function<value_type(const value_type* const, const value_type* const)>> function_derivatives;
	std::vector<std::function<std::pair<value_type,value_type>(const value_type* const, const value_type* const)>> combined_functions;
	unsigned int max_function_size;
public:
	sum_of_functions_t() : max_function_size(0) {};
	void add_function(IN(std::function<value_type(const value_type* const)>) f, IN(std::function<value_type(const value_type* const, const value_type* const)>) f_p, IN(std::vector<unsigned short>) variables);
	void add_function(IN(std::function<value_type(const value_type* const)>) f, IN(std::function<value_type(const value_type* const, const value_type* const)>) f_p, IN(std::function<std::pair<value_type, value_type>(const value_type* const, const value_type* const)>) cf, IN(std::vector<unsigned short>) variables);
	template<typename FT>
	void add_function_t(IN(FT) ft, IN(std::vector<unsigned short>) variables) {
		add_function(std::get<0>(ft), std::get<1>(ft), std::get<2>(ft), variables);
	}
	void add_function(IN(std::function<std::pair<value_type, value_type>(const value_type* const, const value_type* const)>) cf, IN(std::vector<unsigned short>) variables);
	value_type evaluate_at(IN(T) variable_mapping) const;
	void gradient_at(IN(T) variable_mapping, INOUT(rvector_type) gradient) const;
	value_type evaluate_at(IN(T) variable_mapping, value_type lambda, IN(vector_type) direction) const;
	value_type gradient_at(IN(T) variable_mapping, value_type lambda, IN(vector_type) direction) const;
	std::pair<value_type, value_type> evaluate_at_with_derivative(IN(T) variable_mapping, value_type lambda, IN(vector_type) direction) const;
};

template<typename T>
void sum_of_functions_t<T>::add_function(IN(std::function<value_type(const value_type*const)>) f, IN(std::function<value_type(const value_type* const, const value_type* const)>) f_p, IN(std::vector<unsigned short>) variables) {
	max_function_size = std::max(max_function_size, unsigned int(variables.size()));
	unsigned short fnum = static_cast<unsigned short>(functions.size());
	for (const auto v : variables) {
		variable_map.emplace_back(fnum, v);
	}
	functions.push_back(f);
	function_derivatives.push_back(f_p);
	combined_functions.emplace_back([f_p, f](const value_type* const a, const value_type* const b) { return std::make_pair(f(a), f_p(a, b)); });
}

template<typename T>
void sum_of_functions_t<T>::add_function(IN(std::function<value_type(const value_type* const)>) f, IN(std::function<value_type(const value_type* const, const value_type* const)>) f_p, IN(std::function<std::pair<value_type, value_type>(const value_type* const, const value_type* const)>) cf, IN(std::vector<unsigned short>) variables) {
	max_function_size = std::max(max_function_size, unsigned int(variables.size()));
	unsigned short fnum = static_cast<unsigned short>(functions.size());
	for (const auto v : variables) {
		variable_map.emplace_back(fnum, v);
	}
	functions.push_back(f);
	function_derivatives.push_back(f_p);
	combined_functions.push_back(cf);
}

template<typename T>
void sum_of_functions_t<T>::add_function(IN(std::function<std::pair<value_type, value_type>(const value_type* const, const value_type* const)>) cf, IN(std::vector<unsigned short>) variables) {
	max_function_size = std::max(max_function_size, unsigned int(variables.size()));
	unsigned short fnum = static_cast<unsigned short>(functions.size());
	for (const auto v : variables) {
		variable_map.emplace_back(fnum, v);
	}
	combined_functions.push_back(cf);
	functions.emplace_back([cf, sz = variables.size()](const value_type* const a) {
		IN_P(value_type) space = (value_type*)_alloca(sz * sizeof(value_type));
		memset(space, 0, sz * sizeof(value_type));
		return cf(a, space).first; });
	function_derivatives.emplace_back([cf](const value_type* const a, const value_type* const b) { return cf(a, b).second; });
}

template<typename T>
value_type sum_of_functions_t<T>::evaluate_at(IN(T) variable_mapping) const {
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
		variable_store[array_index] = get_nth_variable(variable_mapping, pr.second);
		++array_index;
	}
	current_sum += functions[current_function](variable_store);
	return current_sum;
}

template<typename T>
void sum_of_functions_t<T>::gradient_at(IN(T) variable_mapping, INOUT(rvector_type) gradient) const {
	IN_P(value_type) location = (value_type*)_alloca(max_function_size * sizeof(value_type));
	IN_P(value_type) direction = (value_type*)_alloca(max_function_size * sizeof(value_type));

	const auto mx = variable_mapping.size();
	for(decltype(variable_mapping.size()) i = 0; i < mx; ++i) {
		gradient(get_variable_index(variable_mapping, i)) = value_type(0.0);

		unsigned short current_function = 0;
		unsigned short array_index = 0;
		bool touches_variable = false;

		for (IN(auto) pr : variable_map) {
			if (pr.first != current_function) {
				if (touches_variable) {
					gradient(get_variable_index(variable_mapping, i)) += function_derivatives[current_function](location, direction);
					touches_variable = false;
				}
				current_function = pr.first;
				array_index = 0;
			}
			if (pr.second == i) {
				direction[array_index] = value_type(1.0);
				location[array_index] = get_nth_variable(variable_mapping, pr.second);
				touches_variable = true;
			} else {
				location[array_index] = get_nth_variable(variable_mapping, pr.second);
				direction[array_index] = value_type(0.0);
			}
			++array_index;
		}

		if (touches_variable) {
			gradient(get_variable_index(variable_mapping, i)) += function_derivatives[current_function](location, direction);
		}
	}
}

template<typename T>
value_type sum_of_functions_t<T>::evaluate_at(IN(T) variable_mapping, value_type lambda, IN(vector_type) direction) const {
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
		variable_store[array_index] = get_nth_variable(variable_mapping, pr.second) + direction(get_variable_index(variable_mapping, pr.second)) * lambda;
		++array_index;
	}
	current_sum += functions[current_function](variable_store);
	return current_sum;
}

template<typename T>
value_type sum_of_functions_t<T>::gradient_at(IN(T) variable_mapping, value_type lambda, IN(vector_type) direction) const {
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

		const auto dv = direction(get_variable_index(variable_mapping, pr.second));
		location[array_index] = get_nth_variable(variable_mapping, pr.second) + dv * lambda;
		direction_a[array_index] = dv;

		++array_index;
	}

	sum += function_derivatives[current_function](location, direction_a);

	return sum;
}

template<typename T>
std::pair<value_type, value_type> sum_of_functions_t<T>::evaluate_at_with_derivative(IN(T) variable_mapping, value_type lambda, IN(vector_type) direction) const {
	IN_P(value_type) location = (value_type*)_alloca(max_function_size * sizeof(value_type));
	IN_P(value_type) direction_a = (value_type*)_alloca(max_function_size * sizeof(value_type));

	size_t outer_index = 0;
	value_type deriv_sum = value_type(0.0);
	value_type f_sum = value_type(0.0);

	unsigned short current_function = 0;
	unsigned short array_index = 0;

	for (IN(auto) pr : variable_map) {
		if (pr.first != current_function) {
			const auto cf_r = combined_functions[current_function](location, direction_a);
			f_sum += cf_r.first;
			deriv_sum += cf_r.second;

			current_function = pr.first;
			array_index = 0;
		}

		const auto dv = direction(get_variable_index(variable_mapping, pr.second));
		location[array_index] = get_nth_variable(variable_mapping, pr.second) + dv * lambda;
		direction_a[array_index] = dv;

		++array_index;
	}

	const auto cf_r = combined_functions[current_function](location, direction_a);
	f_sum += cf_r.first;
	deriv_sum += cf_r.second;

	return std::make_pair(f_sum, deriv_sum);
}

using sum_of_functions = sum_of_functions_t<std::vector<var_mapping>>;

class sum_of_functions_b : public sum_of_functions {
public:
	value_type μ; // barrier multiplier
	Eigen::Index n; // number of barriered variables
	value_type* λ; // λ estimate

	sum_of_functions_b() : μ(1.0), n(0), λ(nullptr) {};
	value_type evaluate_at(IN(std::vector<var_mapping>) variable_mapping) const;
	void gradient_at(IN(std::vector<var_mapping>) variable_mapping, INOUT(rvector_type) gradient) const;
	void un_modified_gradient_at(IN(std::vector<var_mapping>) variable_mapping, INOUT(rvector_type) gradient) const;
	value_type evaluate_at(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const;
	value_type gradient_at(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const;
	std::pair<value_type, value_type> evaluate_at_with_derivative(IN(std::vector<var_mapping>) variable_mapping, value_type lambda, IN(vector_type) direction) const;
};

class lm_sum_of_functions : public sum_of_functions_t<vector_type> {
public:
	value_type μ; // barrier multiplier
	vector_type λ; // λ estimate
	vector_type coeff_totals;
	matrix_type coeff;

	lm_sum_of_functions(value_type* λ_storage, value_type* totals_storage, value_type* coeff_storage, Eigen::Index c_rows, Eigen::Index c_cols) : μ(0.5), λ(λ_storage, c_rows), coeff_totals(totals_storage, c_rows), coeff(coeff_storage, c_rows, c_cols) {};
	value_type evaluate_at(IN(vector_type) variables) const;
	void gradient_at(IN(vector_type) variables, INOUT(rvector_type) gradient) const;
	value_type evaluate_at(IN(vector_type) variables, value_type lambda, IN(vector_type) direction) const;
	value_type gradient_at(IN(vector_type) variables, value_type lambda, IN(vector_type) direction) const;
	std::pair<value_type, value_type> evaluate_at_with_derivative(IN(vector_type) variables, value_type lambda, IN(vector_type) direction) const;
};


void vector_times_ut_inverse(IN(rvector_type) vector, IN(matrix_type) coeff, INOUT(rvector_type) result);
void ut_inverse_times_vector(IN(matrix_type) coeff, IN(vector_type) vector, INOUT(vector_type) result);
void vector_times_LU_inverse(IN(rvector_type) vector, IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, INOUT(rvector_type) result);
void LU_inverse_times_vector(IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, IN(vector_type) vector, INOUT(vector_type) result);
void caclulate_reduced_n_gradient_ut(Eigen::Index rows, Eigen::Index remainder, IN(matrix_type) coeff, IN(rvector_type) gradient, INOUT(rvector_type) n_result);
void calcuate_n_direction_steepest(const Eigen::Index b_size, const Eigen::Index remainder, IN(std::vector<var_mapping>) variable_mapping, IN(rvector_type) reduced_n_gradient, INOUT(vector_type) direction);
void calcuate_n_direction_mokhtar(const Eigen::Index b_size, const Eigen::Index remainder, IN(std::vector<var_mapping>) variable_mapping, IN(rvector_type) reduced_n_gradient, INOUT(vector_type) direction);
void calcuate_b_direction_ut(Eigen::Index rows, Eigen::Index remainder, IN(matrix_type) coeff, INOUT(vector_type) direction);
void caclulate_reduced_n_gradient_LU(Eigen::Index rows, Eigen::Index remainder, IN(matrix_type) coeff, IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, IN(rvector_type) gradient, INOUT(rvector_type) n_result);
void calcuate_b_direction_LU(Eigen::Index rows, Eigen::Index remainder, IN(matrix_type) coeff, IN(Eigen::PartialPivLU<Eigen::Ref<matrix_type>>) lu_decomposition, INOUT(vector_type) direction);
std::pair<value_type, short> max_lambda(IN(std::vector<var_mapping>) variable_mapping, INOUT(vector_type) direction);
std::pair<value_type, short> max_lambda(IN(vector_type) variables, INOUT(vector_type) direction);
void setup_rank_map(IN(matrix_type) coeff, IN(std::vector<var_mapping>) variable_mapping, INOUT(flat_multimap<unsigned short, unsigned short>) rank_starts);

std::pair<value_type, size_t> backtrack_linear_steepest_descent(IN(linear_test_function) func, value_type max_value);
std::pair<value_type, size_t> interpolation_linear_steepest_descent(IN(linear_test_function) func, value_type max_value);
std::pair<value_type, size_t> derivative_interpolation_linear_steepest_descent(IN(linear_test_function) func, value_type max_value);
std::pair<value_type, size_t> hager_zhang_linear_steepest_descent(IN(linear_test_function) func, value_type max_value);
std::pair<value_type, size_t> derivitive_minimization_steepest_descent(IN(linear_test_function) func, value_type max_value);

void sof_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hz_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hz_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hz_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hz_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hs_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hs_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hs_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hs_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_hs_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_pr_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_pr_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_pr_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_pr_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_pr_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_prp_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_prp_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_prp_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_prp_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_prp_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_fr_hz_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_fr_dm_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_fr_bt_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_fr_int_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_fr_dint_conjugate_gradient(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);

void sof_hz_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_m_hz_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_dm_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_m_dm_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_bt_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_m_bt_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_int_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_m_int_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_dint_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_m_dint_steepest_descent(IN(sum_of_functions) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);

void sof_hz_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_dm_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_bt_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_int_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);
void sof_dint_steepest_descent_b(INOUT(sum_of_functions_b) function, INOUT(std::vector<var_mapping>) variable_mapping, INOUT(matrix_type) coeff, IN(flat_multimap<unsigned short, unsigned short>) rank_starts);

void sof_hz_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables);
void sof_dm_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables);
void sof_bt_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables);
void sof_int_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables);
void sof_dint_steepest_descent_m(INOUT(lm_sum_of_functions) function, INOUT(vector_type) variables);