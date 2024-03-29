#ifndef UTIL__NETLIST_H
#define UTIL__NETLIST_H

#include <util/generator.hpp>

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <sstream>

#include <boost/range/iterator_range.hpp>

namespace util {

template<typename NODE_ID, bool IS_FOREST = true>
class Netlist {
	using ConnectionStorage = std::unordered_map<NODE_ID,std::unordered_set<NODE_ID>>;
public:
	Netlist()
		: connections()
		, m_roots()
	{ }

	void addLoneNode(const NODE_ID& source) {
		auto source_location_and_wasnt_there = connections.insert({source, {}});
		if (source_location_and_wasnt_there.second) {
			m_roots.insert(source);
		}
	}

	void addConnection(const NODE_ID& source, const NODE_ID& sink) {
		auto source_location_and_wasnt_there = connections.insert({source, {}});
		source_location_and_wasnt_there.first->second.emplace(sink);
		if (source_location_and_wasnt_there.second) {
			m_roots.insert(source);
		}

		auto sink_location_and_wasnt_there = connections.insert({sink, {}});
		if (!sink_location_and_wasnt_there.second) {
			const auto num_erased = m_roots.erase(sink);
			if (IS_FOREST && num_erased == 0) {
				std::stringstream err_str;
				err_str << "adding connection " << source << " -> " << sink << " shorted two trees together: \n";
				for (const auto& id : connections) {
					err_str << id.first << " -> ";
					for (const auto& fanout : id.second) {
						err_str << fanout << ", ";
					}
					err_str << '\n';
				}
				throw std::invalid_argument(err_str.str());
			}
		}
	}

	bool empty() const { return connections.empty(); }

	auto& roots() const { return m_roots; }
	auto& roots()       { return m_roots; }

	boost::iterator_range<typename ConnectionStorage::mapped_type::const_iterator> fanout(const NODE_ID source) const {
		const auto lookup_result = connections.find(source);
		if (lookup_result == end(connections)) {
			return { };
		} else {
			return { begin(lookup_result->second), end(lookup_result->second) };
		}
	}

	auto all_ids() const {
		return util::xrange_forward_pe<typename decltype(connections)::const_iterator>(
			begin(connections),
			end(connections),
			[](auto& elem) {
				return elem->first;
			}
		);
	}

	template<typename VisitorState, typename Visitor>
	auto for_all_descendants(NODE_ID start, VisitorState&& initial_state, Visitor&& visitor) const {
		std::list<std::pair<NODE_ID, VisitorState>> to_visit;
		std::unordered_set<NODE_ID> visited;
		to_visit.emplace_back(start, initial_state);

		while (!to_visit.empty()) {
			const auto& curr_and_state = to_visit.front();

			if (IS_FOREST || visited.insert(curr_and_state.first).second == true) {

				auto new_state = visitor(curr_and_state.first, curr_and_state.second);

				for (const auto& node : fanout(curr_and_state.first)) {
					to_visit.emplace_back(node, new_state);
				}
			}

			to_visit.pop_front();
		}
	}

	template<typename VisitorState, typename Visitor>
	auto for_all_descendant_edges(NODE_ID start, VisitorState&& initial_state, Visitor&& visitor) const {
		struct State {
			NODE_ID parent;
			VisitorState visitor_state;
		};
		struct Params {
			NODE_ID curr;
			NODE_ID parent;
		};

		for_all_descendants(start, State{start, initial_state}, [&](const NODE_ID& node, const State& state) {
			if (node != start) {
				return State{node, visitor(Params{node, state.parent}, state.visitor_state)};
			} else {
				return State{node, state.visitor_state};
			}
		});
	}
private:
	ConnectionStorage connections;
	std::unordered_set<NODE_ID> m_roots;
};

} // end namespace util

#endif // UTIL__NETLIST_H
