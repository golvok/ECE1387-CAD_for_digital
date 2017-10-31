#ifndef ALGO__ANALYTIC_PLACEMENT
#define ALGO__ANALYTIC_PLACEMENT

#include <device/placement_device.hpp>
#include <util/logging.hpp>
#include <util/netlist.hpp>
#include <util/umfpack_interface.hpp>

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>

namespace apl {

template<typename Weighter>
std::unordered_map<device::AtomID, geom::Point<double>> exact_solution(
	const std::vector<std::vector<device::AtomID>>& net_members,
	const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
	const device::PlacementDevice& device,
	Weighter&& weight_between_for_net
) {
	struct NetIDTag { const int DEFAULT_VALUE = -1; };
	using NetID = util::ID<int, NetIDTag>;

	std::unordered_map<NetID, std::vector<device::AtomID>, typename util::MyHash<NetID>::type> net_members_lists;
	std::unordered_map<device::AtomID, std::vector<NetID>> nets_of;
	// vector (movable atom) movable_atoms;
	std::unordered_map<device::AtomID, int> movable_atom_row;
	std::vector<device::AtomID> atom_order;

	(void) device;

	{
		std::unordered_set<device::AtomID> seen_before;
		int row_index = 0;
		int next_net_id = 0;
		for (const auto& atoms : net_members) {
			const auto netID = util::make_id<NetID>(next_net_id);
			for (const auto& atom : atoms) {
				nets_of[atom].push_back(netID);
				net_members_lists[netID].push_back(atom);
				if (seen_before.insert(atom).second) {
					atom_order.push_back(atom);
					if (fixed_block_locations.find(atom) == end(fixed_block_locations)) {
						movable_atom_row[atom] = row_index;
						row_index += 1;
					}
				}
			}
			next_net_id += 1;
		}
	}

	using Weight = double;
	std::vector<std::map<int, Weight>> weight_matrix_columns;
	struct {
		std::vector<Weight> x{};
		std::vector<Weight> y{};
	} right_hand_side;

	for (const device::AtomID& atom : atom_order) {
		if (fixed_block_locations.find(atom) != end(fixed_block_locations)) {
			continue;
		}

		weight_matrix_columns.emplace_back();
		right_hand_side.x.emplace_back(0);
		right_hand_side.y.emplace_back(0);
		auto& col = weight_matrix_columns.back();

		Weight sum = 0;
		for (const auto& net : nets_of[atom]) {
			for (const device::AtomID& connected_atom : net_members_lists[net]) {
				if (connected_atom != atom) {
					const auto weight_from_net = weight_between_for_net(atom, connected_atom, net_members_lists[net].size());
					if (weight_from_net != 0) {
						const auto location_lookup = fixed_block_locations.find(connected_atom);
						if (location_lookup == end(fixed_block_locations)) {
							// if movable
							col[movable_atom_row[connected_atom]] += -weight_from_net;
						} else {
							// if not movable
							right_hand_side.x.back() += weight_from_net * location_lookup->second.getX().getValue();
							right_hand_side.y.back() += weight_from_net * location_lookup->second.getY().getValue();
						}
						sum += weight_from_net;
					}
				}
			}
		}

		col[movable_atom_row[atom]] = sum;
	}

	std::vector<int> col_starts;
	std::vector<int> rows_numbers;
	std::vector<Weight> values;
	int col_start = 0;
	for (const auto& col : weight_matrix_columns) {
		col_starts.push_back(col_start);
		for (const auto& row_and_value : col) {
			values.push_back(row_and_value.second);
			rows_numbers.push_back(row_and_value.first);
			col_start += 1;
		}
	}
	col_starts.push_back(col_start); // need to tell it the end

	if (dout(DL::APL_D2).enabled()) {
		{const auto indent = dout(DL::APL_D2).indentWithTitle("Matrix & RHS For Solving");
		for (int irow = 0; irow < (int)movable_atom_row.size(); ++irow) {
			dout(DL::APL_D2) << std::find_if(begin(movable_atom_row), end(movable_atom_row), [&](const auto& elem) {
				return elem.second == irow;
			})->first << "\t|";
			for (int icol = 0; icol < (int)movable_atom_row.size(); ++icol) {
				const auto lookup = weight_matrix_columns[icol].find(irow);
				if (lookup == end(weight_matrix_columns[icol])) {
					dout(DL::APL_D2) << 0.0 << '\t';
				} else {
					dout(DL::APL_D2) << lookup->second << '\t';
				}
			}
			dout(DL::APL_D2) << "| x_" << irow << "\t| = | " << right_hand_side.x[irow]
			           << "\t\t| y_" << irow << "\t| = | " << right_hand_side.y[irow] << '\n';
		}}
		{const auto indent = dout(DL::APL_D2).indentWithTitle("Data for UMFPACK");
			{const auto indent = dout(DL::APL_D2).indentWithTitle("Column Starts");
				util::print_container(col_starts, dout(DL::APL_D2));
				dout(DL::APL_D2) << '\n';
			}
			{const auto indent = dout(DL::APL_D2).indentWithTitle("Row Numbers");
				util::print_container(rows_numbers, dout(DL::APL_D2));
				dout(DL::APL_D2) << '\n';
			}
			{const auto indent = dout(DL::APL_D2).indentWithTitle("Values");
				util::print_container(values, dout(DL::APL_D2));
				dout(DL::APL_D2) << '\n';
			}
			{const auto indent = dout(DL::APL_D2).indentWithTitle("RHS X Values");
				util::print_container(right_hand_side.x, dout(DL::APL_D2));
				dout(DL::APL_D2) << '\n';
			}
			{const auto indent = dout(DL::APL_D2).indentWithTitle("RHS Y Values");
				util::print_container(right_hand_side.y, dout(DL::APL_D2));
				dout(DL::APL_D2) << '\n';
			}
		}
	}

	auto x_result = umfpack_solve_square((int)movable_atom_row.size(), col_starts, rows_numbers, values, right_hand_side.x);
	auto y_result = umfpack_solve_square((int)movable_atom_row.size(), col_starts, rows_numbers, values, right_hand_side.y);

	std::unordered_map<device::AtomID, geom::Point<double>> result;

	{int i = 0;
	for (const device::AtomID& atom : atom_order) {
		if (fixed_block_locations.find(atom) == end(fixed_block_locations)) {
			result.emplace(
				atom,
				geom::make_point(x_result[i], y_result[i])
			);
			i += 1;
		}
	}}

	return result;
}

} // end namespace apl

#endif // ALGO__ANALYTIC_PLACEMENT
