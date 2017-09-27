#ifndef ALGO__ROUTING_H
#define ALGO__ROUTING_H

#include <device/device.hpp>
#include <util/netlist.hpp>

#include <vector>

namespace algo {

template<typename Netlist, typename FanoutGenerator>
std::vector<std::vector<device::RouteElementID>> route_all(const Netlist& pin_to_pin_netlist, FanoutGenerator&& fanout_gen);

} // end namespace algo

#endif // ALGO__ROUTING_H