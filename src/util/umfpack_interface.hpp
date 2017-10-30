#ifndef UTIL__UMFPACK_INTERFACE_H
#define UTIL__UMFPACK_INTERFACE_H

#include<vector>

std::vector<double> umfpack_solve(
	int n_row,
	int n_col,
	std::vector<int> column_starts,
	std::vector<int> row_indexes,
	std::vector<double> a_values,
	std::vector<double> b_vector
);

inline std::vector<double> umfpack_solve_square(
	int side_length,
	std::vector<int> column_starts,
	std::vector<int> row_indexes,
	std::vector<double> a_values,
	std::vector<double> b_vector
) {
	return umfpack_solve(
		side_length,
		side_length,
		column_starts,
		row_indexes,
		a_values,
		b_vector
	);
}

#endif // UTIL__UMFPACK_INTERFACE_H
