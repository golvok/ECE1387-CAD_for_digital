#include "anaplace_datafile_parser.hpp"

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
namespace anaplace {
namespace input {

boost::variant<ParseResult, std::string> parse_data(std::istream& is) {
	std::stringstream is_as_ss;
	is_as_ss << is.rdbuf();
	auto is_as_string = is_as_ss.str();

	auto indent = dout(DL::DATA_READ1).indentWithTitle("Reading Data");

	namespace x3 = boost::spirit::x3;
	namespace chars = boost::spirit::x3::ascii;
	// using device::XID;

	boost::tuple<
		std::vector<
			boost::tuple<
				uint, std::vector< uint >
			>
		>
		, int
		, std::vector<
			boost::tuple<uint, float, float>
		>
	> 
	parse_results;

	auto it = begin(is_as_string);
	const auto it_end = end(is_as_string);
	const bool is_match = x3::phrase_parse( it, it_end,
		+(
			x3::uint_ >> -x3::lit("->") >> +x3::uint_ >> -x3::omit[x3::int_] >> x3::eol
		)
		>> x3::int_ >> x3::eol
		>> +(
			x3::uint_ >> x3::float_ >> x3::float_ >> x3::eol
		)
		>> -x3::omit[x3::int_]
		>> x3::omit[*chars::space]
		>> x3::eoi,
		chars::blank,
		parse_results
	);

	using std::begin;
	using std::end;

	const bool matches_full = is_match && it == it_end;
	if (!matches_full) {
		auto err_stream = std::stringstream();
		err_stream << "parse fail here/junk at end of file: ";
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

	std::unordered_map<int, device::AtomID> netid_to_src;
	util::Netlist<device::AtomID, false> input_graph;
	util::Netlist<device::AtomID, false> netlist_orig;
	std::unordered_map<device::AtomID, device::BlockID> fixed_block_locations;

	auto try_add_block = [&](int netID, const device::AtomID& atomID) {
		const auto src_find_results = netid_to_src.find(netID);
		if (src_find_results == end(netid_to_src)) {
			netid_to_src.emplace(netID, atomID);
			input_graph.addLoneNode(atomID);
			netlist_orig.addLoneNode(atomID);
		} else {
			input_graph.addConnection(src_find_results->second, atomID);
			netlist_orig.addConnection(src_find_results->second, atomID);
			input_graph.addConnection(atomID, src_find_results->second);
		}
	};

	for (const auto& block : boost::get<0>(parse_results)) {
		const auto atomID = util::make_id<device::AtomID>(boost::get<0>(block));
		for (const auto& netID : boost::get<1>(block)) {
			try_add_block(netID, atomID);
		}
	}

	device::AtomID last_static_atom = util::make_id<device::AtomID>();
	for (const auto& static_block : boost::get<2>(parse_results)) {
		const auto atomID = util::make_id<device::AtomID>(boost::get<0>(static_block));
		last_static_atom = atomID;
		fixed_block_locations.emplace(
			atomID,
			device::BlockID(
				util::make_id<device::XID>(static_cast<int16_t>(boost::get<1>(static_block))),
				util::make_id<device::YID>(static_cast<int16_t>(boost::get<2>(static_block)))
			)
		);
	}

	util::Netlist<device::AtomID, false> netlist;

	struct State {};
	const auto start = last_static_atom;
	netlist.addLoneNode(start);
	input_graph.for_all_descendant_edges(start, State{},
		[&](const auto& atoms, const State& s) {
			netlist.addConnection(atoms.parent, atoms.curr);
			return s;
		}
	);

	return ParseResult{netlist, netlist_orig, fixed_block_locations};
}

}
}
}
