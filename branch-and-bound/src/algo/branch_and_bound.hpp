#ifndef ALGO__BRANCH_AND_BOUND_HPP
#define ALGO__BRANCH_AND_BOUND_HPP

#include <vector>

template<typename Vertex, typename Graph, typename Visitor>
void branchAndBound(Visitor&& visitor, Graph&& graph) {
	struct State {
		Vertex vertex;
		int parent_lower_bound;
	};

	std::vector<State> seed_states;
	seed_states.emplace_back(State{
		*begin(graph.roots()),
		-100,
	});

	auto best_cost = visitor.evalPartialSolution(std::vector<State>()).upper_bound;

	for (const auto& seed_state : seed_states) {
		std::vector<State> state_stack { seed_state, };

		while (!state_stack.empty()) {
			const auto& state = state_stack.back();

			visitor.onExplore(state.vertex, state_stack);
			auto cost = visitor.evalPartialSolution(state_stack);

			const bool skip_branch = cost.lower_bound >= best_cost;
			if (cost.is_complete_solution) {
				best_cost = std::min(best_cost, cost.upper_bound);
				if (!skip_branch) {
					visitor.onNewBest(best_cost, state_stack);
				}
			}

			if (skip_branch || !graph.hasFanout(state.vertex)) {
				while (!state_stack.empty()) {
					visitor.onLeave(state_stack.back().vertex);
					if (state_stack.back().parent_lower_bound < best_cost && graph.hasNextSibling(state.vertex)) {
						state_stack.back().vertex = graph.nextSibling(std::move(state_stack.back().vertex));
						break;
					} else {
						state_stack.pop_back();
					}
				}
			} else {
				state_stack.push_back(State{
					*begin(graph.fanout(state.vertex)),
					cost.lower_bound
				});
			}
		}
	}

	visitor.onComplete();
}

#endif /* ALGO__BRANCH_AND_BOUND_HPP */
