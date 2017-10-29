#ifndef FLOWS__PLACEMENT_FLOWS_H
#define FLOWS__PLACEMENT_FLOWS_H

#include <device/placement_ids.hpp>
#include <device/placement_device.hpp>
#include <graphics/geometry.hpp>
#include <util/netlist.hpp>

#include <unordered_map>

namespace flows {
namespace placement {

device::PlacementDevice make_default_device_description(const util::Netlist<device::AtomID, false>& netlist);

void simple_clique_solve(
	const util::Netlist<device::AtomID, false>& netlist,
	const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
	const device::PlacementDevice& device
);

}
}

#endif // FLOWS__PLACEMENT_FLOWS_H
