#ifndef PARSING__ROUTING_INPUT_PARSER_H
#define PARSING__ROUTING_INPUT_PARSER_H

#include <util/netlist.hpp>

#include <iosfwd>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace parsing {
namespace anaplace {
namespace input {

struct ParseResult {
};

/**
 * returns the input data for the program to work on
 */
boost::variant<ParseResult, std::string> parse_data(std::istream& is);

} // end namespace parsing
} // end namespace anaplace
} // end namespace input

#endif /* PARSING__ROUTING_INPUT_PARSER_H */
