#include "placement_flows.hpp"

#include <algo/analytic_placement.hpp>
#include <flows/flows_common.hpp>
#include <graphics/graphics_wrapper.hpp>

namespace flows {
namespace placement {

namespace {

template<typename Device, typename FixedBlockLocations>
struct SimpleCliqueSolveFlow : public FlowBase<Device, FixedBlockLocations> {
	using FlowBase<Device, FixedBlockLocations>::FlowBase;
	using FlowBase<Device, FixedBlockLocations>::dev;
	using FlowBase<Device, FixedBlockLocations>::nThreads;
	using FlowBase<Device, FixedBlockLocations>::fixed_block_locations;

	SimpleCliqueSolveFlow(const SimpleCliqueSolveFlow&) = default;
	SimpleCliqueSolveFlow(SimpleCliqueSolveFlow&&) = default;
	SimpleCliqueSolveFlow(FlowBase<Device, FixedBlockLocations>&& fb) : FlowBase<Device, FixedBlockLocations>(std::move(fb)) { }
	SimpleCliqueSolveFlow(const FlowBase<Device, FixedBlockLocations>& fb) : FlowBase<Device, FixedBlockLocations>(fb) { }

	auto flow_main(
		const std::vector<std::vector<device::AtomID>>& net_members
	) const {
		const auto indent = dout(DL::INFO).indentWithTitle("Simple Clique Solve Flow");

		const auto& result = apl::exact_solution(
			net_members,
			fixed_block_locations,
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
			result,
			true
		);
		graphics::get().waitForPress();

		return result;
	}
};

} // end anon namespace

device::PlacementDevice make_default_device_description(const std::vector<std::vector<device::AtomID>>& net_members) {
	// const auto block_conut = std::distance(begin(netlist.all_ids()), end(netlist.all_ids()));
	// const auto width = static_cast<int>(std::lround(std::ceil(std::sqrt(block_conut))));
	// return device::PlacementDevice(device::PlacementDevice::Bounds(0, 0, width, width));
	(void)net_members;
	return device::PlacementDevice(device::PlacementDevice::Bounds(0, 0, 4, 4));
}

void simple_clique_solve(
	const std::vector<std::vector<device::AtomID>>& net_members,
	const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
	const device::PlacementDevice& device
) {
	SimpleCliqueSolveFlow<decltype(device), decltype(fixed_block_locations)>(
		device,
		fixed_block_locations,
		1
	).flow_main(
		net_members
	);
}

}
}
