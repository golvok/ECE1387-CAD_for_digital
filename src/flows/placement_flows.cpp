#include "placement_flows.hpp"

#include <algo/analytic_placement.hpp>
#include <graphics/graphics_wrapper.hpp>

namespace flows {
namespace placement {

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
	const auto indent = dout(DL::INFO).indentWithTitle("Simple Clique Solve Flow");
	
	const auto& result = apl::exact_solution(
		net_members,
		fixed_block_locations,
		device
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
}

}
}