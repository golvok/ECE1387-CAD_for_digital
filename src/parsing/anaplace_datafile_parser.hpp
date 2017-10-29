#ifndef PARSING__ROUTING_INPUT_PARSER_H
#define PARSING__ROUTING_INPUT_PARSER_H

#include <device/placement_ids.hpp>
#include <util/netlist.hpp>
#include <graphics/geometry.hpp>

#include <iosfwd>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace parsing {
namespace anaplace {
namespace input {

struct ParseResult {
	      auto& netlist()       { return m_netlist; }
	const auto& netlist() const { return m_netlist; }

	      auto& netlistAsParsed()       { return m_netlist_as_parsed; }
	const auto& netlistAsParsed() const { return m_netlist_as_parsed; }

	      auto& fixedBlockLocations()       { return m_fixed_block_locations; }
	const auto& fixedBlockLocations() const { return m_fixed_block_locations; }

	util::Netlist<device::AtomID, false> m_netlist;
	util::Netlist<device::AtomID, false> m_netlist_as_parsed;
	std::unordered_map<device::AtomID, device::BlockID> m_fixed_block_locations;
};

/**
 * returns the input data for the program to work on
 */
boost::variant<ParseResult, std::string> parse_data(std::istream& is);

} // end namespace parsing
} // end namespace anaplace
} // end namespace input

#endif /* PARSING__ROUTING_INPUT_PARSER_H */
