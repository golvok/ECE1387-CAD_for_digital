#include "umfpack_interface.hpp"

#include <util/logging.hpp>

#include <umfpack.h>

std::vector<double> umfpack_solve(
	int n_row,
	int n_col,
	std::vector<int> column_starts,
	std::vector<int> row_indexes,
	std::vector<double> a_values,
	std::vector<double> b_vector
) {
	auto throw_not_ok = [&](const auto& status) {
		if (status != UMFPACK_OK) {
			util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "UMFPACK error: " << status;
			});
		}
	};

	void *symbolic_data;
	throw_not_ok(umfpack_di_symbolic(
		n_row,
		n_col,
		column_starts.data(),
		row_indexes.data(),
		a_values.data(),
		&symbolic_data,
		nullptr,
		nullptr
	));

	void *numeric_data;
	throw_not_ok(umfpack_di_numeric(
		column_starts.data(),
		row_indexes.data(),
		a_values.data(),
		symbolic_data,
		&numeric_data,
		nullptr,
		nullptr
	));

	umfpack_di_free_symbolic(&symbolic_data);

	std::vector<double> x_vector(n_row);
	throw_not_ok(umfpack_di_solve(
		UMFPACK_A,
		column_starts.data(),
		row_indexes.data(),
		a_values.data(),
		x_vector.data(),
		b_vector.data(),
		numeric_data,
		nullptr,
		nullptr
	));

	umfpack_di_free_numeric(&numeric_data);

	return x_vector;
}
