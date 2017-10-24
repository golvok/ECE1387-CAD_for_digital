#include "input_parser.hpp"

#include <util/logging.hpp>
#include <util/iteration_utils.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/tuple.hpp>

#ifndef ECF_BOOST_COMPAT
	#include <boost/spirit/home/x3.hpp>
#else
	#include <boost/spirit/include/qi.hpp>
	#include <boost/spirit/include/phoenix_core.hpp>
	#include <boost/spirit/include/phoenix_operator.hpp>
	#define x3 qi // please be lenient...
#endif

namespace parsing {
namespace input {

boost::variant<ParseResult, std::string> parse_data(std::istream& is, boost::optional<device::DeviceTypeID> default_device_type) {
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

	boost::tuple<int, int, std::vector< boost::tuple<
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

	device::DeviceInfo device_info{
		*default_device_type,
		geom::BoundBox<int>(0,0,boost::get<0>(parse_results)-1,boost::get<0>(parse_results)-1),
		boost::get<1>(parse_results),
		1,
		2,
	};

	util::Netlist<PinGID> netlist;

	for (const auto& route_request_parse : boost::get<2>(parse_results)) {

		if (boost::get<0>(route_request_parse) < 0) {
			break;
		}

		netlist.addConnection(
			util::make_id<PinGID>(
				util::make_id<BlockID>(
					util::make_id<XID>(boost::get<0>(route_request_parse)),
					util::make_id<YID>(boost::get<1>(route_request_parse))
				),
				util::make_id<BlockPinID>(boost::get<2>(route_request_parse))
			),
			util::make_id<PinGID>(
				util::make_id<BlockID>(
					util::make_id<XID>(boost::get<3>(route_request_parse)),
					util::make_id<YID>(boost::get<4>(route_request_parse))
				),
				util::make_id<BlockPinID>(boost::get<5>(route_request_parse))
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
