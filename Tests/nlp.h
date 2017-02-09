#pragma once
#include "globalhelpers.h"

#ifdef SAFETY_OFF
#define EIGEN_NO_DEBUG
#else
#define EIGEN_NO_MALLOC
#endif

#define EIGEN_MPL2_ONLY

#include "Dense"

using value_type = float;
using e_vector = Eigen::Matrix<value_type, Eigen::Dynamic, 1>;
using e_row_vector = Eigen::Matrix<value_type, 1, Eigen::Dynamic>;
using e_matrix = Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic>;
using rvector_type = Eigen::Map<e_row_vector, Eigen::Aligned>;
using vector_type = Eigen::Map<e_vector, Eigen::Aligned>;
using matrix_type = Eigen::Map<e_matrix, Eigen::Aligned>;

struct var_mapping {
	value_type current_value;
	unsigned short mapped_index;
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
std::pair<value_type, short> max_lambda(INOUT(std::vector<var_mapping>) variable_mapping, INOUT(vector_type) direction);