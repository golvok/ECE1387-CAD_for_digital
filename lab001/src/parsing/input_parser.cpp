#include "input_parser.hpp"

#include <util/logging.hpp>
#include <util/iteration_utils.hpp>

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

boost::variant<ParseResult, std::string> parse_data(std::istream& is) {
	std::stringstream is_as_ss;
	is_as_ss << is.rdbuf();
	auto is_as_string = is_as_ss.str();

	auto indent = dout(DL::DATA_READ1).indentWithTitle("Reading Data");

	namespace x3 = boost::spirit::x3;
	namespace chars = boost::spirit::x3::ascii;
	using device::XID;
	using device::YID;
	using device::BlockPinID;
	using device::BlockID;
	using device::PinGID;

	std::tuple<int, int, std::vector< std::tuple<
		XID::IDType, YID::IDType, BlockPinID::IDType, XID::IDType, YID::IDType, BlockPinID::IDType
	> > > parse_results;

	auto it = begin(is_as_string);
	const auto it_end = end(is_as_string);
	const bool is_match = x3::phrase_parse( it, it_end,
		x3::int_ >> '\n' >> x3::int_ >> '\n' >> (
			x3::short_ >> x3::short_ >> x3::short_ >> x3::short_ >> x3::short_ >> x3::short_
		) % '\n' >> x3::omit[*chars::space],
		x3::lit(' '),
		parse_results
	);

	using std::begin;
	using std::end;

	DeviceInfo device_info;
	device_info.bounds = geom::BoundBox<int>(0,0,get<0>(parse_results),get<0>(parse_results));
	device_info.track_width = get<1>(parse_results);
	device_info.pins_per_block = 4;

	util::Netlist<PinGID> netlist;

	for (const auto& route_request_parse : get<2>(parse_results)) {

		if (get<0>(route_request_parse) < 0) {
			break;
		}

		netlist.add_connection(
			util::make_id<PinGID>(
				util::make_id<BlockID>(
					util::make_id<XID>(get<0>(route_request_parse)),
					util::make_id<YID>(get<1>(route_request_parse))
				),
				util::make_id<BlockPinID>(get<2>(route_request_parse))
			),
			util::make_id<PinGID>(
				util::make_id<BlockID>(
					util::make_id<XID>(get<3>(route_request_parse)),
					util::make_id<YID>(get<4>(route_request_parse))
				),
				util::make_id<BlockPinID>(get<5>(route_request_parse))
			)
		);
	}

	const bool matches_full = is_match && it == it_end;
	if (!matches_full) {
		auto err_stream = std::stringstream();
		err_stream << "junk at end of file: ";
		int count = 0;
		for (auto& c : util::make_iterable(std::make_pair(it, it_end))) {
			err_stream << c;
			if (count > 100) {
				err_stream << " ...";
				break;
			}
		}
		return err_stream.str();
	}

	return ParseResult{device_info, netlist};
}

}
}
