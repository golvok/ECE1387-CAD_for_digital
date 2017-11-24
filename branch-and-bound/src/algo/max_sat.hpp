#ifndef ALGO__MAX_SAT_HPP
#define ALGO__MAX_SAT_HPP

#include <datastructures/cnf_evaluation.hpp>
#include <util/logging.hpp>

template<typename Vertex>
struct DefaultMaxSatVisitor {
	const CNFExpression& expression;
	CNFEvaluation<std::vector> evaluator;
	int num_partial_settings_explored = 0;
	int num_complete_settings_explored = 0;
	std::vector<Literal> best_solution = {};

	DefaultMaxSatVisitor(const CNFExpression& expression)
		: expression(expression)
		, evaluator(expression)
	{ }

	template<typename StateStack>
	void onExplore(const Vertex& vertex, const StateStack& state_stack) {
		// dout(DL::INFO) << vertex.literal() << ' ';
		const auto& new_counts = evaluator.addSetting(vertex.literal().id(), !vertex.inverted);
		(void)new_counts;
		(void)state_stack;
	}

	void onLeave(const Vertex& vertex) {
		const auto& new_counts = evaluator.removeSetting(vertex.literal().id(), !vertex.inverted);
		// dout(DL::INFO) << vertex.literal() << ": leavecounts={fc=" << new_counts.false_count << ", uc=" << new_counts.undecidable_count << "}\n";
		(void)new_counts;
	}


	template<typename T>
	auto evalPartialSolution(const T& partial_solution) {
		(void)partial_solution;

		auto& counts = evaluator.getCounts();
		// dout(DL::INFO) << "counts={fc=" << counts.false_count << ", uc=" << counts.undecidable_count << "}";

		struct Result {
			int lower_bound;
			int upper_bound;
			bool is_complete_solution;
		} result {
			counts.false_count,
			counts.false_count + counts.undecidable_count,
			counts.undecidable_count == 0,
		};

		if (result.is_complete_solution) {
			num_complete_settings_explored += 1;
		} else {
			num_partial_settings_explored += 1;
		}

		if ((num_partial_settings_explored + num_complete_settings_explored) % 1000000 == 0) {
			dout(DL::INFO) << "Status: ";
			printExploredMessage(dout(DL::INFO));
			dout(DL::INFO) << "\n";
		}

		// dout(DL::INFO) << ", result={lb=" << result.lower_bound << ", ub=" << result.upper_bound << ", ics=" << std::boolalpha << result.is_complete_solution << "}\n";
		return result;
	}

	template<typename Cost, typename StateStack>
	void onNewBest(const Cost& best_cost, const StateStack& state_stack) {
		best_solution.clear();
		std::transform(begin(state_stack), end(state_stack), std::back_inserter(best_solution), [=](auto&& s) { return s.vertex.literal(); });
		dout(DL::INFO) << "New best: cost=" << best_cost << " settings=";
		util::print_container(best_solution, dout(DL::INFO));
		dout(DL::INFO) << "\n";
	}

	template<typename Stream>
	void printExploredMessage(Stream&& stream) {
		stream << "explored "
			<< num_partial_settings_explored << " partial settings and "
			<< num_complete_settings_explored << " complete settings. "
			<< "(total = " << (num_partial_settings_explored + num_complete_settings_explored) << ')'
		;
	}

	void onComplete() {
		dout(DL::INFO) << "Done. In the end:\n\t";
		printExploredMessage(dout(DL::INFO));
		dout(DL::INFO) << "\n\tbest solution: ";
		util::print_container(best_solution, dout(DL::INFO));
		dout(DL::INFO) << "\n";
	}
};

#endif /* ALGO__MAX_SAT_HPP */
