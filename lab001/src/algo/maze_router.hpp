#ifndef ALGO__MAZE_ROUTER_H
#define ALGO__MAZE_ROUTER_H

#include <device/connectors.hpp>
#include <device/device.hpp>
#include <util/logging.hpp>

#include <list>
#include <unordered_map>
#include <vector>

namespace algo {

template<typename ID, typename IDSet, typename ID2, typename FanoutGenerator, typename ShouldIgnore>
std::vector<ID> maze_route(IDSet&& sources, ID2&& sink, FanoutGenerator&& fanout_gen, ShouldIgnore&& should_ignore) {
	std::list<ID> to_visit;

	dout(DL::ROUTE_D1) << "starting with: ";
	for (const auto& source : sources) {
		dout(DL::ROUTE_D1) << source << ' ';
		to_visit.push_back(source);
	}
	dout(DL::ROUTE_D1) << '\n';

	struct VertexData {
		std::vector<ID> fanin{};
		int distance = 0;
		bool put_in_queue = false;
		bool expanded = false;
	};

	std::unordered_map<ID, VertexData> data;

	while (!to_visit.empty()) {
		const auto explore_curr = to_visit.front();
		to_visit.pop_front();

		dout(DL::ROUTE_D1) << "examining " << explore_curr << "... ";

		if (data[explore_curr].expanded) {
			dout(DL::ROUTE_D1) << "already seen it\n";
			continue;
		} else if (should_ignore(explore_curr)) {
			dout(DL::ROUTE_D1) << "should be ignored\n";
			continue;
		}

		for (const auto& fanout : fanout_gen.fanout(explore_curr)) {
			data[fanout].fanin.push_back(explore_curr);
			if (!data[fanout].put_in_queue && !data[fanout].expanded && !should_ignore(fanout)) {
				dout(DL::ROUTE_D2) << "adding " << fanout << "... ";
				data[fanout].distance = data[explore_curr].distance + 1;
				data[fanout].put_in_queue = true;
				to_visit.push_back(fanout);
			} else {
				dout(DL::ROUTE_D3) << "skipping " << fanout << "... ";
			}
		}

		if (explore_curr == sink) {
			dout(DL::ROUTE_D1) << "is the target!\n";
			break;
		}

		data[explore_curr].expanded = true;

		dout(DL::ROUTE_D1) << "done\n";
	}

	dout(DL::ROUTE_D1) << "tracing back... ";

	std::vector<ID> result;
	auto traceback_curr = sink;
	while (true) {
		dout(DL::ROUTE_D1) << traceback_curr << " -> ";
		result.push_back(traceback_curr);
		if (sources.find(traceback_curr) != end(sources) || data[traceback_curr].fanin.empty()) {
			break;
		}
		const auto& fanin = data[traceback_curr].fanin;
		traceback_curr = *std::min_element(begin(fanin), end(fanin), [&](const auto& lhs, const auto& rhs) {
			return data[lhs].distance < data[rhs].distance;
		});
	}

	dout(DL::ROUTE_D1) << " (SRC) \n";

	return result;
}

} // end namespace algo

#endif // ALGO__MAZE_ROUTER_H
