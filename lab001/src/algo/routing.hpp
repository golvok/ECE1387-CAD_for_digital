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

template<bool exitAtFirstNoRoute, typename Netlist, typename NetOrder, typename FanoutGenerator>
RouteAllResult<Netlist> route_all(const Netlist& pin_to_pin_netlist, NetOrder&& net_order, FanoutGenerator&& fanout_gen, int ntheads = 1) {
	RouteAllResult<Netlist> result;
	std::unordered_set<device::RouteElementID> used;

	const auto gfx_state_keeper = graphics::get().fpga().pushState(&fanout_gen, true);
	bool encountered_failing_pin = false;

	for (const auto& src_pin : net_order) {
		const auto& src_pin_re = device::RouteElementID(src_pin);
		std::unordered_set<device::RouteElementID> used_by_this_net;
		used_by_this_net.insert(src_pin_re);

		for (const auto& sink_pin : pin_to_pin_netlist.fanout(src_pin)) {
			const auto sink_pin_re = device::RouteElementID(sink_pin);

			if (exitAtFirstNoRoute && encountered_failing_pin) {
				result.unroutedPins().addConnection(src_pin, sink_pin);
				continue;
			}

			auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
				str << "Routing " << src_pin_re << " -> " << sink_pin_re;
			});

			const auto& new_routing = algo::maze_route<device::RouteElementID>(used_by_this_net, sink_pin_re, fanout_gen, [&](auto&& reid) {
				return (reid != sink_pin && reid != src_pin && reid.isPin()) || (used.find(reid) != std::end(used) && used_by_this_net.find(reid) == std::end(used_by_this_net));
			}, ntheads);

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

				if (dout(DL::PIN_BY_PIN_STEP).enabled()) {
					const auto gfx_state_keeper = graphics::get().fpga().pushState(&fanout_gen, {*new_routing}, true);
					graphics::get().waitForPress();
				}
			} else {
				result.unroutedPins().addConnection(src_pin, sink_pin);
				encountered_failing_pin = true;
			}
		}
	}
	return result;
}

} // end namespace algo

#endif // ALGO__ROUTING_H
