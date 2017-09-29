#ifndef ALGO__ROUTING_H
#define ALGO__ROUTING_H

#include <device/device.hpp>
#include <util/netlist.hpp>

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
RouteAllResult<Netlist> route_all(const Netlist& pin_to_pin_netlist, FanoutGenerator&& fanout_gen);

} // end namespace algo

#endif // ALGO__ROUTING_H
