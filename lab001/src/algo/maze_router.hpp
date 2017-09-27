#ifndef ALGO__MAZE_ROUTER_H
#define ALGO__MAZE_ROUTER_H

#include <device/connectors.hpp>
#include <device/device.hpp>
#include <util/logging.hpp>

#include <list>
#include <unordered_map>
#include <vector>

namespace algo {

// template<typename ID, typename ID1, typename ID2, typename FanoutGenerator>
// std::vector<ID> maze_route(ID1&& source, ID2&& sink, FanoutGenerator&& fanout_gen);

template<typename ID, typename ID1, typename ID2, typename FanoutGenerator>
std::vector<ID> maze_route(ID1&& source, ID2&& sink, FanoutGenerator&& fanout_gen) {
	std::list<ID> to_visit;
	to_visit.push_back(source);

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

		dout(DL::INFO) << "examining " << explore_curr << "... ";

		if (data[explore_curr].expanded) {
			dout(DL::INFO) << "already seen it\n";
			continue;
		}

		for (const auto& fanout : fanout_gen.fanout(explore_curr)) {
			data[fanout].fanin.push_back(explore_curr);
			if (!data[fanout].put_in_queue && !data[fanout].expanded) {
				dout(DL::INFO) << "adding " << fanout << "... ";
				data[fanout].distance = data[explore_curr].distance + 1;
				data[fanout].put_in_queue = true;
				to_visit.push_back(fanout);
			} else {
				dout(DL::INFO) << "skipping " << fanout << "... ";
			}
		}

		if (explore_curr == sink) {
			dout(DL::INFO) << "is the target!\n";
			break;
		}

		data[explore_curr].expanded = true;

		dout(DL::INFO) << "done\n";
	}

	dout(DL::INFO) << "tracing back...\n";

	std::vector<ID> result;
	auto traceback_curr = sink;
	while (true) {
		result.push_back(traceback_curr);
		if (traceback_curr == source || data[traceback_curr].fanin.empty()) {
			break;
		}
		const auto& fanin = data[traceback_curr].fanin;
		traceback_curr = *std::min_element(begin(fanin), end(fanin), [&](const auto& lhs, const auto& rhs) {
			return data[lhs].distance < data[rhs].distance;
		});
	}

	return result;
}

// extern template
// std::vector<device::RouteElementID> maze_route<device::RouteElementID, const device::RouteElementID&, const device::RouteElementID&, const device::Device<device::FullyConnectedConnector>&>(
// 	const device::RouteElementID& source, const device::RouteElementID& dest, const device::Device<device::FullyConnectedConnector>& fanout_gen
// );

} // end namespace algo

#endif // ALGO__MAZE_ROUTER_H
