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

	std::unordered_map<int, std::vector<device::AtomID>> net_members;
	for (const auto& block : boost::get<0>(parse_results)) {
		const auto atomID = util::make_id<device::AtomID>(boost::get<0>(block));
		for (const auto& netID : boost::get<1>(block)) {
			net_members[netID].push_back(atomID);
		}
	}

	std::unordered_map<device::AtomID, device::BlockID> fixed_block_locations;
	for (const auto& static_block : boost::get<2>(parse_results)) {
		const auto atomID = util::make_id<device::AtomID>(boost::get<0>(static_block));
		fixed_block_locations.emplace(
			atomID,
			device::BlockID(
				util::make_id<device::XID>(static_cast<int16_t>(boost::get<1>(static_block))),
				util::make_id<device::YID>(static_cast<int16_t>(boost::get<2>(static_block)))
			)
		);
	}

	std::vector<std::vector<device::AtomID>*> net_member_lists;
	for (auto& net_and_members : net_members) {
		std::partition(begin(net_and_members.second), end(net_and_members.second),
			[&](const auto& elem) {
				return fixed_block_locations.find(elem) != end(fixed_block_locations);
			}
		);
		net_member_lists.push_back(&net_and_members.second);
	}

	std::sort(begin(net_member_lists), end(net_member_lists),
		[&](auto& lhs, auto& rhs) {
			return std::forward_as_tuple(fixed_block_locations.find(lhs->front()) == end(fixed_block_locations), lhs->size())
				< std::forward_as_tuple(fixed_block_locations.find(rhs->front()) == end(fixed_block_locations), rhs->size());
		}
	);

	for (const auto& net_member_list : net_member_lists) {
		for (const auto& atom : *net_member_list) {
			dout(DL::INFO) << atom << ' ';
		}
		dout(DL::INFO) << '\n';
	}

	struct State {
		using ChoiceVectorPtrIt = decltype(begin(net_member_lists));
		using ChoiceIter = decltype(begin(**begin(net_member_lists)));
		ChoiceIter choice;
		ChoiceVectorPtrIt choice_list;
	};

	std::vector<State> choice_stack;
	std::unordered_map<device::AtomID, decltype(*begin(net_member_lists))> net_choices;

	choice_stack.emplace_back(State{
		begin(**begin(net_member_lists)),
		begin(net_member_lists)
	});


	size_t record = 0;
	while (true) {
		if (net_choices.size() + 10 < record) {
			record = net_choices.size();
			dout(DL::INFO) << "significant backtrack\n";
		}
		if (net_choices.size() > record) {
			record = net_choices.size();
			dout(DL::INFO) << "now at: " << record << '/' << net_members.size() << '\n';
		}
		if (net_choices.size() == net_members.size()) {
			break;
		}

		const auto& curr = choice_stack.back();
		const auto unused_atom_lookup = std::find_if(curr.choice, end(**curr.choice_list),
			[&](const auto& atom) {
				return net_choices.find(atom) == end(net_choices);
			}
		);

		if (unused_atom_lookup == end(**curr.choice_list)) {
			// previous choince was bad. revert.
			choice_stack.pop_back();
			if (choice_stack.empty()) {
				return "couldn't convert netlist";
			} else {
				net_choices.erase(*choice_stack.back().choice);
				std::advance(choice_stack.back().choice, 1);
			}
		} else {
			choice_stack.back().choice = unused_atom_lookup;
			net_choices.emplace(*curr.choice, *curr.choice_list);
			// dout(DL::INFO) << *curr.choice << '\n';
			auto next_choice_list = std::next(curr.choice_list);
			choice_stack.emplace_back(State{
				begin(**next_choice_list),
				next_choice_list
			});
		}
	}

	util::Netlist<device::AtomID, false> netlist;
	for (const auto& choice_and_net_members : net_choices) {
		for (const auto& atom : *choice_and_net_members.second) {
			if (atom != choice_and_net_members.first) {
				netlist.addConnection(choice_and_net_members.first, atom);
			}
		}
	}

	return ParseResult{netlist, netlist, fixed_block_locations};
}

}
}
}
