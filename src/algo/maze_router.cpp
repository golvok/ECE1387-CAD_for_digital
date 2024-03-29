#include "maze_router.hpp"

#include <device/connectors.hpp>
#include <device/device.hpp>
#include <graphics/graphics_wrapper_fpga.hpp>
#include <util/template_utils.hpp>

namespace algo {

namespace detail {

	template<typename ID, typename Device>
	void displayAndWait(std::unordered_map<ID, graphics::t_color>&& colours_to_draw, const Device& device) {
		const auto gfx_state_keeper = graphics::get().fpga().pushRoutingState(&device, std::move(colours_to_draw), false);
		graphics::get().waitForPress();
	}

	template<typename Device>
	struct displayAndWait_instantiator {
		static auto func(){
			return &displayAndWait<device::RouteElementID, Device>;
		}
	};

	auto maze_router_template_instantiator() {
		return std::make_tuple(
			util::forceInstantiation<displayAndWait_instantiator>(device::ALL_DEVICES)
		);
	}

}

} // end namespace algo
