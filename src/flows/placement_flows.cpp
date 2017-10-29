#include "placement_flows.hpp"

#include <util/umfpack_interface.hpp>

namespace flows {
namespace placement {

device::PlacementDevice make_default_device_description(const util::Netlist<device::AtomID, false>& netlist) {
	const auto block_conut = std::distance(begin(netlist.all_ids()), end(netlist.all_ids()));
	const auto width = static_cast<int>(std::lround(std::ceil(std::sqrt(block_conut))));
	return device::PlacementDevice(device::PlacementDevice::Bounds(0, 0, width, width));
}

void simple_clique_solve(
	const util::Netlist<device::AtomID, false>& netlist,
	const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
	const device::PlacementDevice& device
) {
	(void)netlist;
	(void)fixed_block_locations;
	(void)device;
	solve();
}

}
}