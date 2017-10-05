#ifndef UTIL__GRAPH_ALGORITHMS_H
#define UTIL__GRAPH_ALGORITHMS_H

#include <unordered_map>

namespace util {

namespace detail {
	struct AlwaysFalse {
		template<typename... T>
		bool operator()(T&&...) { return false; }
	};
}

template<typename ID, typename FanoutGen, typename InitialList, typename Visitor, typename ShouldIgnore = detail::AlwaysFalse>
void breadthFirstVisit(FanoutGen&& fanout_gen, const InitialList& initial_list, Visitor&& visitor, ShouldIgnore&& should_ignore = ShouldIgnore()) {
	struct VertexData {
		bool put_in_queue = false;
		bool expanded = false;
	};

	std::list<ID> to_visit;
	for (const auto& vertex : initial_list) {
		to_visit.push_back(vertex);
	}

	std::unordered_map<ID, VertexData> data;

	while (!to_visit.empty()) {
		const auto explore_curr = to_visit.front();
		to_visit.pop_front();

		if (data[explore_curr].expanded) {
			continue;
		} else if (should_ignore(explore_curr)) {
			continue;
		}

		for (const auto& fanout : fanout_gen.fanout(explore_curr)) {
			if (!data[fanout].put_in_queue && !data[fanout].expanded && !should_ignore(fanout)) {
				data[fanout].put_in_queue = true;
				to_visit.push_back(fanout);
			}
		}

		visitor(explore_curr);
	}

}

} // end namespace util

#endif // UTIL__GRAPH_ALGORITHMS_H
