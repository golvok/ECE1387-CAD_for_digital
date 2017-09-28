#include "maze_router.hpp"

#include <device/connectors.hpp>
#include <device/device.hpp>
#include <graphics/graphics_wrapper.hpp>

namespace algo {

namespace detail {

	template<typename ID, typename Device>
	void displayAndWait(std::unordered_map<ID, graphics::t_color>&& colours_to_draw, const Device& device) {
		const auto gfx_state_keeper = graphics::get().fpga().pushState(&device, std::move(colours_to_draw));
		graphics::get().waitForPress();
	}

	template void displayAndWait(std::unordered_map<device::RouteElementID, graphics::t_color>&& colours_to_draw, const device::Device<device::FullyConnectedConnector>& device);
}

} // end namespace algo
