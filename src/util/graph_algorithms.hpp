#ifndef UTIL__GRAPH_ALGORITHMS_H
#define UTIL__GRAPH_ALGORITHMS_H

#include <list>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/range/iterator_range.hpp>

namespace util {

template<typename VertexID>
class DefaultGraphVisitor {
public:
	template<typename VertexCollection>
	void onWaveStart(const VertexCollection&) { }

	void onExplore(const VertexID&) { }

	void onSkippedExplore(const VertexID&) { }

	void onFanout(const VertexID&, const VertexID&) { }

	void onSkippedFanout(const VertexID&, const VertexID&) { }

	void onExploreStart() { }

	void onExploreEnd() { }

	void onNextWaveCalcStart() { }

	void onNextWaveCalcEnd() { }

	void onDataEntryStart() { }

	void onDataEntryEnd() { }

	void onWaveEnd() { }
};

namespace detail {
	struct AlwaysFalse {
		template<typename... T>
		bool operator()(T&&...) { return false; }
	};

	template<template <typename...> class Map, typename... InitialParams>
	struct BasicMapMaker {
		template<typename... RestParams>
		auto makeMap() const {
			return Map<InitialParams..., RestParams...>();
		}
	};
}

template<
	typename ID,
	typename MapGen = detail::BasicMapMaker<std::unordered_map, ID> >
class GraphAlgo {
private:
	int NTHREADS = 1;
	MapGen vertexMapGen = {};

public:
	GraphAlgo() { }

	template<typename NewMapGen>
	GraphAlgo(
		int NTHREADS,
		NewMapGen&& vertexMapGen
	)
		: NTHREADS(NTHREADS)
		, vertexMapGen(std::forward<NewMapGen>(vertexMapGen))
	{ }

	auto withThreads(int NTHREADS) const {
		return GraphAlgo<
			ID,
			MapGen
		>(
			NTHREADS,
			vertexMapGen
		);
	}

	template<typename NewMapGen>
	auto withMapGen(NewMapGen&& newVertexMapGen) const {
		return GraphAlgo<
			ID,
			std::decay_t<NewMapGen>
		>(
			NTHREADS,
			std::forward<NewMapGen>(newVertexMapGen)
		);
	}

	template<typename... Params>
	auto makeVertexMap() const {
		return this->vertexMapGen.template makeMap<Params...>();
	}

template<typename FanoutGen, typename InitialList, typename Visitor, typename ShouldIgnore = detail::AlwaysFalse>
void breadthFirstVisit(FanoutGen&& fanout_gen, const InitialList& initial_list, Visitor&& visitor, ShouldIgnore&& should_ignore = ShouldIgnore()) const {
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

template<typename FanoutGen, typename InitialList, typename IsTarget, typename Visitor, typename ShouldIgnore = detail::AlwaysFalse>
auto wavedBreadthFirstVisit(FanoutGen&& fanout_gen, const InitialList& initial_list, IsTarget&& isTarget, Visitor&& visitor, ShouldIgnore&& should_ignore = ShouldIgnore()) const {
	struct VertexData {
		std::vector<ID> fanin = {};
	};

	auto data = makeVertexMap<VertexData>();

	struct ExploreData {
		ID parent;
		ID fanout;
	};

	std::vector<ID> curr_wave = {};
	struct WaveData {
		std::vector<ExploreData> next_wave = {};
		void clear() {
			next_wave.clear();
		}
	};

	std::vector<WaveData> waveData(NTHREADS);

	for (const auto& vertex : initial_list) {
		curr_wave.push_back(vertex);
		data[vertex];
	}

	// the follewing are cleared and reused
	std::vector<ExploreData> explorations_to_new_nodes;
	std::unordered_set<ID> in_next_wave;

	while(true) {
		visitor.onWaveStart(curr_wave);
		visitor.onExploreStart();

		const auto expand_wave_code = [&](int ithread) {
			const auto& num_data_per_thread = 1 + ((curr_wave.size()-1)/NTHREADS); // ronuds up
			const auto& my_curr_wave_begin_index = ithread*num_data_per_thread;
			const auto& my_curr_wave_end_index =   (ithread + 1)*num_data_per_thread + 1;
			const auto& my_curr_wave = boost::make_iterator_range(
				std::next(begin(curr_wave), std::min(curr_wave.size(), my_curr_wave_begin_index)),
				std::next(begin(curr_wave), std::min(curr_wave.size(), my_curr_wave_end_index))
			);
			auto& my_next_wave = waveData[ithread].next_wave;

			for (const auto& id : my_curr_wave) {
				if (should_ignore(id)) {
					visitor.onSkippedExplore(id);
					continue;
				} else {
					visitor.onExplore(id);
					for (const auto& fanout : fanout_gen.fanout(id)) {
						if (data.find(fanout) == end(data) && !should_ignore(fanout)) {
							my_next_wave.emplace_back(ExploreData{id, fanout});
							visitor.onFanout(id, fanout);
						} else {
							visitor.onSkippedFanout(id, fanout);
						}
					}
				}
			}
		};

		if (NTHREADS == 1) {
			expand_wave_code(0);
		} else {
			std::vector<std::thread> threads;

			for (int ithread = 0; ithread < NTHREADS; ++ithread) {
				threads.emplace_back(expand_wave_code, ithread);
			}

			for (auto& thread : threads) {
				thread.join();
			}
		}

		visitor.onExploreEnd();
		visitor.onNextWaveCalcStart();

		bool found_target = false;
		explorations_to_new_nodes.clear();
		in_next_wave.clear();
		curr_wave.clear();
		for (auto& waveDatum : waveData) {
			for (auto& exploreData : waveDatum.next_wave) {
				if (data.find(exploreData.fanout) == end(data)) {
					explorations_to_new_nodes.emplace_back(exploreData);
					if (in_next_wave.find(exploreData.fanout) == end(in_next_wave)) {
						in_next_wave.emplace(exploreData.fanout);
						curr_wave.push_back(exploreData.fanout);
					}
				}

				if (isTarget(exploreData.fanout)) {
					found_target = true;
				}

			}
			waveDatum.clear();
		}

		visitor.onNextWaveCalcEnd();
		visitor.onDataEntryStart();

		for (const auto& exploreData : explorations_to_new_nodes) {
			data[exploreData.fanout].fanin.emplace_back(exploreData.parent);
		}

		if (found_target || curr_wave.empty()) {
			break;
		}

		visitor.onDataEntryEnd();
		visitor.onWaveEnd();
	}

	return data;
}

}; // end class GraphAlgo

} // end namespace util

#endif // UTIL__GRAPH_ALGORITHMS_H
