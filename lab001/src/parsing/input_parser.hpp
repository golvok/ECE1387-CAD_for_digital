#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include <device/device.hpp>
#include <graphics/geometry.hpp>
#include <util/netlist.hpp>

#include <iosfwd>
#include <tuple>

#include <boost/variant.hpp>

namespace parsing {
namespace input {

struct DeviceInfo {
	geom::BoundBox<int> bounds{};
	int track_width = -1;
};

struct ParseResult {
	DeviceInfo device_info{};
	util::Netlist<device::PinGID> pin_to_pin_netlist{};
};

/**
 * returns the input data for the program to work on
 */
boost::variant<ParseResult, std::string> parse_data(std::istream& is);

} // end namespace parsing
} // end namespace input

#endif /* INPUT_PARSER_H */
