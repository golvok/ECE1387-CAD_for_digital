#include "routing.hpp"

#include <algo/maze_router.hpp>
#include <device/connectors.hpp>
#include <device/device.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <util/logging.hpp>

#include <boost/optional.hpp>

namespace algo {

template<typename Netlist, typename FanoutGenerator>
util::Netlist<device::RouteElementID, true> route_all(const Netlist& pin_to_pin_netlist, FanoutGenerator&& fanout_gen) {
	util::Netlist<device::RouteElementID, true> result;
	std::unordered_set<device::RouteElementID> used;

	const auto gfx_state_keeper = graphics::get().fpga().pushState(&fanout_gen, true);

	for (const auto& src_pin : pin_to_pin_netlist.roots()) {
		const auto& src_pin_re = device::RouteElementID(src_pin);
		std::unordered_set<device::RouteElementID> used_by_this_net;
		used_by_this_net.insert(src_pin_re);

		for (const auto& sink_pin : pin_to_pin_netlist.fanout(src_pin)) {
			const auto sink_pin_re = device::RouteElementID(sink_pin);

			auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
				str << "Routing " << src_pin_re << " -> " << sink_pin_re;
			});

			const auto& new_routing = algo::maze_route<device::RouteElementID>(used_by_this_net, sink_pin_re, fanout_gen, [&](auto&& reid) {
				return used.find(reid) != end(used) && used_by_this_net.find(reid) == end(used_by_this_net);
			});

			boost::optional<device::RouteElementID> prev;
			for (const auto& id : new_routing) {
				used.insert(id);
				used_by_this_net.insert(id);
				if (prev) {
					result.addConnection(*prev, id);
				}
				prev = id;
			}

			const auto gfx_state_keeper = graphics::get().fpga().pushState(&fanout_gen, {new_routing}, true);
			graphics::get().waitForPress();
		}
	}
	return result;
}

namespace detail {
	template<typename Netlist>
	struct route_all_instantiator {
		template<typename Device>
		struct with_netlist {
			static auto func(){
				return std::make_tuple(
					&route_all<Netlist, Device>,
					&route_all<Netlist, Device&>,
					&route_all<Netlist, const Device&>
				);
			}
		};
	};

	auto fpga_graphics_data_template_instantiator() {
		return std::make_tuple(
			util::forceInstantiation<route_all_instantiator< util::Netlist<device::PinGID> >::with_netlist>( device::ALL_DEVICES )
		);
	}
}

} // end namespace algo
