#include "umfpack_interface.hpp"

#include <util/logging.hpp>

int solve() {
	int    n_row = 5;
	int    n_col = n_row;
	std::vector<int> column_starts{0, 2, 5, 9, 10, 12};
	std::vector<int> row_indexes{ 0,  1,  0,   2,  4,  1,  2,  3,   4,  2,  1,  4};
	std::vector<double> a_values{2., 3., 3., -1., 4., 4., -3., 1., 2., 2., 6., 1.};
	std::vector<double> b_vector{8., 45., -3., 3., 19.};
	std::vector<double> x_vector(n_row);

	void *symbolic_data;
	umfpack_di_symbolic(
		n_row,
		n_col,
		column_starts.data(),
		row_indexes.data(),
		a_values.data(),
		&symbolic_data,
		nullptr,
		nullptr
	);

	void *numeric_data;
	umfpack_di_numeric(
		column_starts.data(),
		row_indexes.data(),
		a_values.data(),
		symbolic_data,
		&numeric_data,
		nullptr,
		nullptr
	);

	umfpack_di_free_symbolic(&symbolic_data);

	umfpack_di_solve(
		UMFPACK_A,
		column_starts.data(),
		row_indexes.data(),
		a_values.data(),
		x_vector.data(),
		b_vector.data(),
		numeric_data,
		nullptr,
		nullptr
	);

	umfpack_di_free_numeric(&numeric_data);

	int i = 0;
	for (const auto& x : x_vector) {
		dout(DL::INFO) << "x [" << i << "] = " << x << '\n';
		i++;
	}

	return 0;
}
