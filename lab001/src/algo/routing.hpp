#ifndef ALGO__ROUTING_H
#define ALGO__ROUTING_H

#include <algo/maze_router.hpp>
#include <device/connectors.hpp>
#include <device/device.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <util/logging.hpp>

#include <boost/optional.hpp>

namespace algo {

template<typename UnroutedNetlist>
class RouteAllResult {
	using ResultNetlist = util::Netlist<device::RouteElementID, true>;
public:
	auto& netlist() const { return m_netlist; }
	auto& netlist()       { return m_netlist; }

	auto& unroutedPins() const { return m_unroutedPins; }
	auto& unroutedPins()       { return m_unroutedPins; }
private:
	ResultNetlist m_netlist = {};
	UnroutedNetlist m_unroutedPins = {};
};

template<typename Netlist, typename FanoutGenerator>
RouteAllResult<Netlist> route_all(const Netlist& pin_to_pin_netlist, FanoutGenerator&& fanout_gen) {
	RouteAllResult<Netlist> result;
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

			if (new_routing) {
				boost::optional<device::RouteElementID> prev;
				for (const auto& id : *new_routing) {
					used.insert(id);
					used_by_this_net.insert(id);
					if (prev) {
						result.netlist().addConnection(*prev, id);
					}
					prev = id;
				}

				const auto gfx_state_keeper = graphics::get().fpga().pushState(&fanout_gen, {*new_routing}, true);
				graphics::get().waitForPress();
			} else {
				result.unroutedPins().addConnection(src_pin, sink_pin);
			}
		}
	}
	return result;
}

} // end namespace algo

#endif // ALGO__ROUTING_H
