#include "placement_flows.hpp"

#include <algo/analytic_placement.hpp>
#include <flows/flows_common.hpp>
#include <graphics/graphics_wrapper.hpp>

namespace flows {
namespace placement {

namespace {

template<typename Device, typename FixedBlockLocations>
struct SimpleCliqueSolveFlow : public FlowBase<Device, FixedBlockLocations> {
	DECLARE_USING_FLOBASE_MEMBERS(Device, FixedBlockLocations)

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
