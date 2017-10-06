#ifndef ALGO__MAZE_ROUTER_H
#define ALGO__MAZE_ROUTER_H

#include <graphics/graphics_types.hpp>
#include <util/graph_algorithms.hpp>
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
		// dout(DL::INFO) << "wavefront size = " << wavefront.size() << '\n';
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

template<typename VertexID, typename OnWaveStart>
struct Visitor : public util::DefaultGraphVisitor<VertexID> {
	Visitor(OnWaveStart&& ows) : onWaveStart(std::move(ows)) { }
	Visitor(const OnWaveStart& ows) : onWaveStart(ows) { }

	OnWaveStart onWaveStart;

	void onExplore(VertexID id) {
		dout(DL::ROUTE_D1) << "visit: " << id << '\n';
	}
};

template<typename ID, typename IDSet, typename ID2, typename FanoutGenerator, typename ShouldIgnore>
boost::optional<std::vector<ID>> maze_route(IDSet&& sources, ID2&& sink, FanoutGenerator&& fanout_gen, ShouldIgnore&& should_ignore) {

	const auto onWaveStart = [&](const auto& wave) {
		detail::displayWavefront<ID>(sources, sink, fanout_gen, std::vector<ID>(), std::vector<ID>(), wave);
	};

	const auto data2 = util::wavedBreadthFirstVisit(fanout_gen, sources, sink, Visitor<ID, decltype(onWaveStart)>(onWaveStart), should_ignore);

	dout(DL::ROUTE_D1) << "tracing2back... ";

	boost::optional<std::vector<ID>> reversed_result = std::vector<ID>();
	auto traceback_curr = sink;
	while (true) {
		reversed_result->push_back(traceback_curr);
		if (sources.find(traceback_curr) != end(sources)) {
			dout(DL::ROUTE_D1) << traceback_curr << '\n';
			break;
		} else {
			const auto& found_data = data2.find(traceback_curr);
			if (found_data == end(data2) || found_data->second.fanin.empty()) {
				dout(DL::ROUTE_D1) << "couldn't trace back past " << traceback_curr << '\n';
				reversed_result = boost::none;
				break;
			} else {
				dout(DL::ROUTE_D1) << traceback_curr << " -> ";
				traceback_curr = found_data->second.fanin.front();
			}
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
