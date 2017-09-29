#include "maze_router.hpp"

#include <device/connectors.hpp>
#include <device/device.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <util/template_utils.hpp>

namespace algo {

namespace detail {

	template<typename ID, typename Device>
	void displayAndWait(std::unordered_map<ID, graphics::t_color>&& colours_to_draw, const Device& device) {
		const auto gfx_state_keeper = graphics::get().fpga().pushState(&device, std::move(colours_to_draw), false);
		graphics::get().waitForPress();
	}

	template<typename Device>
	struct displayAndWait_instantiator {
		static auto func(){
			return &displayAndWait<device::RouteElementID, Device>;
		}
	};

	void maze_router_template_instantiator() {
		util::forceInstantiation<displayAndWait_instantiator>(device::ALL_DEVICES);
	}

}

} // end namespace algo
