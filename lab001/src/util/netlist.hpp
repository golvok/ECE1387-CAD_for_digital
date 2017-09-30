#ifndef UTIL__NETLIST_H
#define UTIL__NETLIST_H

#include <util/generator.hpp>

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
				err_str << "attempt to add connection " << source << " -> " << sink << " that will short two trees together: \n";
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
private:
	ConnectionStorage connections;
	std::unordered_set<NODE_ID> m_roots;
};

} // end namespace util

#endif // UTIL__NETLIST_H
