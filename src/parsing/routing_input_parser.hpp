#ifndef PARSING__ROUTING_INPUT_PARSER_H
#define PARSING__ROUTING_INPUT_PARSER_H

#include <device/device.hpp>
#include <util/netlist.hpp>

#include <iosfwd>
#include <tuple>
#include <vector>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace parsing {
namespace routing {
namespace input {

struct ParseResult {
	device::DeviceInfo device_info{};
	util::Netlist<device::PinGID> pin_to_pin_netlist{};
	std::vector<std::pair<device::PinGID, device::PinGID>> pin_order_in_input = {};
};

/**
 * returns the input data for the program to work on
 */
boost::variant<ParseResult, std::string> parse_data(std::istream& is, boost::optional<device::DeviceTypeID> default_device_type);

} // end namespace parsing
} // end namespace routing
} // end namespace input

#endif /* PARSING__ROUTING_INPUT_PARSER_H */
