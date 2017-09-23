#include "input_parser.hpp"

#include <util/logging.hpp>
#include <util/netlist.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/spirit/home/x3.hpp>

namespace {
    template <typename T>
    struct as_type {
        template <typename Expr>
            auto operator[](Expr&& expr) const {
                return boost::spirit::x3::rule<struct _, T>{"as"} = boost::spirit::x3::as_parser(std::forward<Expr>(expr));
            }
    };

    template <typename T> static const as_type<T> as = {};
}

namespace parsing {
namespace input {

std::tuple<bool, bool> parse_data(std::istream& is) {
	std::stringstream is_as_ss;
	is_as_ss << is.rdbuf();
	auto is_as_string = is_as_ss.str();

	auto indent = dout(DL::DATA_READ1).indentWithTitle("Reading Data");

	namespace x3 = boost::spirit::x3;
	namespace chars = boost::spirit::x3::ascii;

	std::tuple<int, int, std::vector< std::tuple<
		int, int, int, int, int, int
	> > > parse_results;

	auto it = begin(is_as_string);
	const auto it_end = end(is_as_string);
	const bool is_match = x3::phrase_parse( it, it_end,
		x3::int_ >> '\n' >> x3::int_ >> '\n' >> (
			x3::int_ >> x3::int_ >> x3::int_ >> x3::int_ >> x3::int_ >> x3::int_
		) % '\n' >> x3::omit[*chars::space],
		x3::lit(' '),
		parse_results
	);

	using util::BlockID;
	using util::XID;
	using util::YID;
	using std::begin;
	using std::end;

	util::Netlist<BlockID> netlist;

	for (const auto& route_request_parse : get<2>(parse_results)) {

		if (get<0>(route_request_parse) < 0) {
			break;
		}

		netlist.add_connection(
			util::make_id<BlockID>(util::make_id<XID>(get<0>(route_request_parse)), util::make_id<YID>(get<1>(route_request_parse))),
			util::make_id<BlockID>(util::make_id<XID>(get<3>(route_request_parse)), util::make_id<YID>(get<4>(route_request_parse)))
		);
	}

	for (const auto& src_and_sinks : netlist) {
		for (const auto& sink : src_and_sinks.second) {
			dout(DL::INFO) << src_and_sinks.first << " -> " << sink << '\n';
		}
	}

	const bool matches_full = is_match && it == it_end;
	const bool match_is_good = matches_full;

	return {match_is_good, true};
}

}
}
