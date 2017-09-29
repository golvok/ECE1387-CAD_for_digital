#ifndef ALGO__MAZE_ROUTER_H
#define ALGO__MAZE_ROUTER_H

#include <graphics/graphics_types.hpp>
#include <util/logging.hpp>

#include <list>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

namespace algo {

namespace detail {

	template<typename ID, typename Device>
	void displayAndWait(std::unordered_map<ID, graphics::t_color>&& colours_to_draw, const Device& device);

	template<
		typename ID,
		typename IDSet,
		typename ID2,
		typename FanoutGenerator,
		typename ShouldIgnore,
		typename Explored,
		typename Wavefront
	>
	void displayWavefront(
		const IDSet& sources,
		const ID2& sink,
		const FanoutGenerator& fanout_gen,
		const ShouldIgnore& should_ignore,
		const Explored& explored,
		const Wavefront& wavefront
	) {
		if (!dout(DL::MAZE_ROUTE_STEP).enabled()) {
			return;
		}

		std::unordered_map<ID, graphics::t_color> colours_to_draw;

		const graphics::t_color    source_colour(0xFF, 0x00, 0x00);
		const graphics::t_color      sink_colour(0x00, 0xFF, 0x00);
		const graphics::t_color   ignored_colour(0x22, 0x22, 0x22);
		const graphics::t_color  explored_colour(0x55, 0x55, 0x55);
		const graphics::t_color wavefront_colour(0xFF, 0x99, 0x00);

		colours_to_draw[sink] = sink_colour;
		for (const auto& node : explored)      { colours_to_draw[node] =  explored_colour; }
		for (const auto& node : sources)       { colours_to_draw[node] =    source_colour; }
		for (const auto& node : wavefront)     { colours_to_draw[node] = wavefront_colour; }
		for (const auto& node : should_ignore) { colours_to_draw[node] =   ignored_colour; }

		displayAndWait(std::move(colours_to_draw), fanout_gen);
	}

}

template<typename ID, typename IDSet, typename ID2, typename FanoutGenerator, typename ShouldIgnore>
boost::optional<std::vector<ID>> maze_route(IDSet&& sources, ID2&& sink, FanoutGenerator&& fanout_gen, ShouldIgnore&& should_ignore) {
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

	bool set_next_graphics_update = false;
	auto next_graphics_update = to_visit.front();
	while (!to_visit.empty()) {
		const auto explore_curr = to_visit.front();
		if (explore_curr == next_graphics_update) {
			detail::displayWavefront<ID>(sources, sink, fanout_gen, std::vector<ID>(), std::vector<ID>(), to_visit);
			set_next_graphics_update = false;
		}
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
				if (!set_next_graphics_update) {
					next_graphics_update = fanout;
					set_next_graphics_update = true;
				}
			} else {
				dout(DL::ROUTE_D3) << "skipping " << fanout << "... ";
			}
		}

		if (!set_next_graphics_update && !to_visit.empty()) {
			next_graphics_update = to_visit.back();
			set_next_graphics_update = true;
		}

		if (explore_curr == sink) {
			dout(DL::ROUTE_D1) << "is the target!\n";
			break;
		}

		data[explore_curr].expanded = true;

		dout(DL::ROUTE_D1) << "done\n";
	}

	detail::displayWavefront<ID>(sources, sink, fanout_gen, std::vector<ID>(), std::vector<ID>(), to_visit);

	dout(DL::ROUTE_D1) << "tracing back... ";

	boost::optional<std::vector<ID>> reversed_result = std::vector<ID>();
	auto traceback_curr = sink;
	while (true) {
		if (sources.find(traceback_curr) != end(sources)) {
			dout(DL::ROUTE_D1) << traceback_curr << '\n';
			break;
		} else if (data[traceback_curr].fanin.empty()) {
			dout(DL::ROUTE_D1) << "couldn't trace back past " << traceback_curr << '\n';
			reversed_result = boost::none;
			break;
		} else {
			dout(DL::ROUTE_D1) << traceback_curr << " -> ";
			reversed_result->push_back(traceback_curr);
			const auto& fanin = data[traceback_curr].fanin;
			traceback_curr = *std::min_element(begin(fanin), end(fanin), [&](const auto& lhs, const auto& rhs) {
				return data[lhs].distance < data[rhs].distance;
			});
		}
	}

	if (reversed_result) {
		std::vector<ID> result;
		for (auto it = reversed_result->rbegin(); it != reversed_result->rend(); ++it) {
			result.push_back(*it);
		}
		return result;
	} else {
		return boost::none;
	}
}

} // end namespace algo

#endif // ALGO__MAZE_ROUTER_H
