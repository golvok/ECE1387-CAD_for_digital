#ifndef PARSING__ROUTING_INPUT_PARSER_H
#define PARSING__ROUTING_INPUT_PARSER_H

#include <graphics/geometry.hpp>

#include <iosfwd>
#include <vector>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace parsing {
namespace sat_maxer {
namespace input {

struct ParseResult {
};

/**
 * returns the input data for the program to work on
 */
boost::variant<ParseResult, std::string> parse_data(std::istream& is);

} // end namespace parsing
} // end namespace sat_maxer
} // end namespace input

#endif /* PARSING__ROUTING_INPUT_PARSER_H */
