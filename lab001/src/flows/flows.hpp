#ifndef FLOWS__FLOWS_H
#define FLOWS__FLOWS_H

#include <device/connectors.hpp>
#include <device/device.hpp>
#include <util/netlist.hpp>

namespace flows {

void fanout_test(const device::DeviceInfo& dev_desc, int nThreads = 1);
void track_width_exploration(const device::DeviceInfo& dev_desc, const util::Netlist<device::PinGID>& pin_to_pin_netlist, int nThreads = 1);
void route_as_is(const device::DeviceInfo& dev_desc, const util::Netlist<device::PinGID>& pin_to_pin_netlist, int nThreads = 1);

} // end namespace flow

#endif // FLOWS__FLOWS_H
