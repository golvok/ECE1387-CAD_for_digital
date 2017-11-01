#include "placement_flows.hpp"

#include <algo/analytic_placement.hpp>
#include <flows/flows_common.hpp>
#include <graphics/graphics_wrapper.hpp>

#include <array>

namespace flows {
namespace placement {

namespace {

template<typename Points, typename BlockLocations>
Points convert_to_points(const BlockLocations& bl) {
	Points result;
	std::transform(begin(bl), end(bl), std::inserter(result, end(result)),
		[](auto&& id_and_block) {
			return std::make_pair(
				id_and_block.first,
				geom::make_point<double>(
					(double)id_and_block.second.getX().getValue(),
					(double)id_and_block.second.getY().getValue()
				)
			);
		}
	);
	return result;
}


template<typename Self, typename Device, typename FixedBlockLocations, typename AnchorLocations = std::unordered_map<device::AtomID, geom::Point<double>>>
struct APLFlowBase : public FlowBase<APLFlowBase<Self, Device, FixedBlockLocations, AnchorLocations>, Device, FixedBlockLocations> {
	using FlowBaseBaseClass = FlowBase<APLFlowBase, Device, FixedBlockLocations>;
	DECLARE_USING_FLOWBASE_MEMBERS_JUST_MEMBERS(FlowBaseBaseClass)


	template<typename ArgSelf>
	APLFlowBase(const APLFlowBase<ArgSelf, Device, FixedBlockLocations, AnchorLocations>& src)
		: FlowBaseBaseClass(src)
		, anchor_locations(src.anchor_locations)
	{ }

	template<typename ArgSelf>
	APLFlowBase(APLFlowBase<ArgSelf, Device, FixedBlockLocations, AnchorLocations>&& src)
		: FlowBaseBaseClass(src)
		, anchor_locations(src.anchor_locations)
	{ }

	APLFlowBase(
		const FlowBaseBaseClass& fb,
		const AnchorLocations& anchor_locations
	)
		: FlowBaseBaseClass(fb)
		, anchor_locations(anchor_locations)
	{ }

	APLFlowBase(
		const Device& dev,
		const FixedBlockLocations& fixed_block_locations,
		const AnchorLocations& anchor_locations,
		int nThreads
	)
		: FlowBaseBaseClass(
			dev,
			fixed_block_locations,
			nThreads
		)
		, anchor_locations(anchor_locations)
	{ }

	Self withDevice(const Device& newDev) const {
		return {
			newDev,
			fixed_block_locations,
			anchor_locations,
			nThreads
		};
	}

	Self withFixedBlockLocations(const FixedBlockLocations& newFBL) const {
		return {
			dev,
			newFBL,
			anchor_locations,
			nThreads
		};
	}

	Self withAnchorLocations(const AnchorLocations& newALoc) const {
		return {
			dev,
			fixed_block_locations,
			newALoc,
			nThreads
		};
	}

	const AnchorLocations& anchor_locations;
};

#define DECLARE_USING_APLFLOWBASE_MEMBERS(...) \
	using __VA_ARGS__::APLFlowBase; \
	using __VA_ARGS__::anchor_locations; \



template<typename Device, typename FixedBlockLocations>
struct SimpleCliqueSolveFlow : public APLFlowBase<SimpleCliqueSolveFlow<Device, FixedBlockLocations>, Device, FixedBlockLocations> {
	DECLARE_USING_FLOWBASE_MEMBERS(SimpleCliqueSolveFlow, APLFlowBase<SimpleCliqueSolveFlow, Device, FixedBlockLocations>)
	DECLARE_USING_APLFLOWBASE_MEMBERS(APLFlowBase<SimpleCliqueSolveFlow, Device, FixedBlockLocations>)

	SimpleCliqueSolveFlow(const SimpleCliqueSolveFlow&) = default;
	SimpleCliqueSolveFlow(SimpleCliqueSolveFlow&&) = default;

	auto flow_main(
		const std::vector<std::vector<device::AtomID>>& net_members
	) const {
		const auto indent = dout(DL::INFO).indentWithTitle("Simple Clique Solve Flow");

		const auto& result = apl::exact_solution(
			net_members,
			anchor_locations,
			dev,
			[&](const device::AtomID& a1, const device::AtomID& a2, const auto& net_size) {
				(void)a1; (void)a2;
				return 2.0/(double)net_size;
			}
		);

		if (dout(DL::APL_D1).enabled()) {
			{const auto indent = dout(DL::APL_D1).indentWithTitle("Solution");
				util::print_assoc_container(result, dout(DL::APL_D1));
				dout(DL::APL_D1) << '\n';
			}
		}

		const auto graphics_keeper = graphics::get().fpga().pushPlacingState(
			net_members,
			fixed_block_locations,
			anchor_locations,
			result,
			true
		);
		graphics::get().waitForPress();

		return result;
	}
};

template<typename Device, typename FixedBlockLocations>
struct CliqueAndSpreadFLow : public APLFlowBase<CliqueAndSpreadFLow<Device, FixedBlockLocations>, Device, FixedBlockLocations> {
	DECLARE_USING_FLOWBASE_MEMBERS(CliqueAndSpreadFLow, APLFlowBase<CliqueAndSpreadFLow, Device, FixedBlockLocations>)
	DECLARE_USING_APLFLOWBASE_MEMBERS(APLFlowBase<CliqueAndSpreadFLow, Device, FixedBlockLocations>)

	CliqueAndSpreadFLow(const CliqueAndSpreadFLow&) = default;
	CliqueAndSpreadFLow(CliqueAndSpreadFLow&&) = default;

	template<typename ShouldStop>
	auto flow_main(
		const std::vector<std::vector<device::AtomID>>& net_members,
		ShouldStop&& should_stop
	) const {
		const auto indent = dout(DL::INFO).indentWithTitle("Clique And Spread Flow");
		using AtomID = device::AtomID;

		auto current_anchor_locations = anchor_locations; // copy
		auto current_net_members = net_members; // copy
		auto current_bb = dev.info().bounds(); // copy

		util::IDGenerator<AtomID> atom_id_gen(100000); // TODO have the file parser return something better

		for (int num_spreadings = 0; /* always */; ++num_spreadings) {
			if (should_stop(num_spreadings)) {
				break;
			}

			const auto current_result = SimpleCliqueSolveFlow<Device, FixedBlockLocations>(*this)
				.withAnchorLocations(current_anchor_locations)
				.flow_main(current_net_members);

			const auto centroid = [&]() {
				std::vector<decltype(begin(current_result))> x_order;
				for (auto it = begin(current_result); it != end(current_result); ++it) {
					x_order.push_back(it);
				}
				std::vector<decltype(begin(current_result))> y_order(x_order);

				std::sort(begin(x_order), end(x_order), [&](auto& lhs, auto& rhs) {
					return lhs->second.x() < rhs->second.x();
				});
				std::sort(begin(y_order), end(y_order), [&](auto& lhs, auto& rhs) {
					return lhs->second.y() < rhs->second.y();
				});

				return geom::make_point(x_order[x_order.size()/2]->second.x(), y_order[y_order.size()/2]->second.y());
			}();

			auto assignments = assign_to_quadrants(
				current_bb,
				centroid,
				current_result,
				atom_id_gen
			);

			{const auto indent = dout(DL::INFO).indentWithTitle("New Assignments");
			for (const auto new_net : assignments.new_nets) {
				dout(DL::INFO) << new_net[0] << " -> " << new_net[1] << '\n';
			}}

			{ const auto indent = dout(DL::APL_D1).indentWithTitle("New Anchors");
			for (const auto id_and_loc : assignments.new_anchor_locations) {
				dout(DL::APL_D1) << id_and_loc.first << " @ " << id_and_loc.second << '\n';
			}}


			std::move(begin(assignments.new_nets), end(assignments.new_nets), std::back_inserter(current_net_members));
			std::move(begin(assignments.new_anchor_locations), end(assignments.new_anchor_locations), std::inserter(current_anchor_locations, end(current_anchor_locations)));
		}
	}

	template<typename AtomRange, typename AtomIDGen>
	static auto assign_to_quadrants(
		geom::BoundBox<double> working_bounds,
		geom::Point<double> centroid,
		AtomRange&& moveable_atom_locations,
		AtomIDGen&& atom_id_gen
	) {
		using AtomID = device::AtomID;

		std::array<std::pair<AtomID, geom::Point<double>>, 4> anchor_ids_and_locs{{
			{ atom_id_gen.gen_id(), (working_bounds.get_center() + geom::Point<double>(working_bounds.max_point())     )/2.0 },
			{ atom_id_gen.gen_id(), (working_bounds.get_center() + geom::Point<double>(working_bounds.minxmaxy_point()))/2.0 },
			{ atom_id_gen.gen_id(), (working_bounds.get_center() + geom::Point<double>(working_bounds.min_point())     )/2.0 },
			{ atom_id_gen.gen_id(), (working_bounds.get_center() + geom::Point<double>(working_bounds.maxxminy_point()))/2.0 },
		}};

		auto quadrant_for = [&](const auto& point) {
			if (point.x() < centroid.x()) {
				if (point.y() < centroid.y()) {
					return anchor_ids_and_locs[2];
				} else {
					return anchor_ids_and_locs[1];
				}
			} else {
				if (point.y() < centroid.y()) {
					return anchor_ids_and_locs[3];
				} else {
					return anchor_ids_and_locs[0];
				}
			}
		};

		struct Result {
			std::vector<std::vector<AtomID>> new_nets{};
			std::vector<std::pair<AtomID, geom::Point<double>>> new_anchor_locations{};
		} result;

		std::copy(begin(anchor_ids_and_locs), end(anchor_ids_and_locs), std::back_inserter(result.new_anchor_locations));

		for (const auto& atom_and_loc : moveable_atom_locations) {
			const auto& anchor_id = quadrant_for(atom_and_loc.second).first;

			std::vector<AtomID> list = {atom_and_loc.first, anchor_id};
			result.new_nets.emplace_back(std::move(list));
		}

		return result;
	}
};

} // end anon namespace

device::PlacementDevice make_default_device_description(const std::vector<std::vector<device::AtomID>>& net_members) {
	std::unordered_set<device::AtomID> unique_atoms;
	for (const auto& net : net_members) {
		for (const auto& atom : net) {
			unique_atoms.insert(atom);
		}
	}
	const auto width = static_cast<int>(std::lround(std::ceil(std::sqrt(unique_atoms.size()))));
	return device::PlacementDevice(device::PlacementDevice::Bounds(0, 0, width, width));
}

void simple_clique_solve(
	const std::vector<std::vector<device::AtomID>>& net_members,
	const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
	const device::PlacementDevice& device
) {
	SimpleCliqueSolveFlow<std::decay_t<decltype(device)>, std::decay_t<decltype(fixed_block_locations)>>(
		device,
		fixed_block_locations,
		convert_to_points<std::unordered_map<device::AtomID, geom::Point<double>>>(fixed_block_locations),
		1
	).flow_main(
		net_members
	);
}

void clique_and_spread(
	const std::vector<std::vector<device::AtomID>>& net_members,
	const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
	const device::PlacementDevice& device,
	int max_spreadings
) {
	CliqueAndSpreadFLow<std::decay_t<decltype(device)>, std::decay_t<decltype(fixed_block_locations)>>(
		device,
		fixed_block_locations,
		convert_to_points<std::unordered_map<device::AtomID, geom::Point<double>>>(fixed_block_locations),
		1
	).flow_main(
		net_members,
		[&](auto& num_spreadings) {
			return num_spreadings > max_spreadings;
		}
	);
}


}
}
