#include "routing.hpp"

#include <algo/maze_router.hpp>
#include <device/connectors.hpp>
#include <device/device.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <util/logging.hpp>

namespace algo {

template<typename Netlist, typename FanoutGenerator>
int route_all(const Netlist& pin_to_pin_netlist, FanoutGenerator&& fanout_gen) {
	for (const auto& src_sinks : pin_to_pin_netlist) {
		const auto src_pin_re = device::RouteElementID(src_sinks.first);
		for (const auto& sink_pin : src_sinks.second) {
			const auto sink_pin_re = device::RouteElementID(sink_pin);

			auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
				str << "Routing " << src_pin_re << " -> " << sink_pin_re;
			});
			const auto path = algo::maze_route<device::RouteElementID>(src_pin_re, sink_pin_re, fanout_gen, [&](auto&& reid) {
				(void)reid;
				return false;
			});
			util::print_container(path, dout(DL::INFO));
			dout(DL::INFO) << '\n';

			graphics::get().fpga().clearPaths();
			graphics::get().fpga().addPath(path);
			graphics::get().waitForPress();
			graphics::get().fpga().clearPaths();
		}
	}
	return 0;
}

template int route_all(const util::Netlist<device::PinGID>& pin_to_pin_netlist,       device::Device<device::FullyConnectedConnector>&  fanout_gen);
template int route_all(const util::Netlist<device::PinGID>& pin_to_pin_netlist, const device::Device<device::FullyConnectedConnector>&  fanout_gen);
template int route_all(const util::Netlist<device::PinGID>& pin_to_pin_netlist,       device::Device<device::FullyConnectedConnector>&& fanout_gen);

} // end namespace algo
