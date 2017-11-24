
#include <datastructures/cnf_evaluation.hpp>
#include <graphics/graphics_wrapper_sat_maxer.hpp>
#include <parsing/sat_maxer_cmdargs_parser.hpp>
#include <parsing/sat_maxer_datafile_parser.hpp>
#include <util/generator.hpp>
#include <util/graph_algorithms.hpp>
#include <util/lambda_compose.hpp>
#include <util/logging.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include <boost/optional.hpp>

using namespace parsing::sat_maxer;
using namespace parsing::sat_maxer::cmdargs;

int program_main(const ProgramConfig& config);
void satisfy_maximally(const CNFExpression& expression);
void do_optional_input_data_dump(const std::string& data_file_name, const input::ParseResult& pr);

int main(int argc, char const** argv) { try{

	dout.setHighestTitleRank(7);

	const auto parsed_args = cmdargs::parse(argc,argv);
	// enable logging levels
	for (auto& l : parsed_args.meta().getDebugLevelsToEnable()) {
		dout.enable_level(l);
	}

	// enable graphics
	if (parsed_args.meta().shouldEnableGraphics()) {
		graphics::get().enable();
		graphics::get().startThreadsAndOpenWindow();
	}

	const auto result = program_main(parsed_args.programConfig());
	// graphics::get().close();
	graphics::get().join();

	return result;

} catch (std::exception& e) { std::cerr << "Uncaught exception: .what() = \"" << e.what() << '"' << std::endl; return -1; } }

int program_main(const ProgramConfig& config) {

	std::ifstream data_file(config.dataFileName());
	if (!data_file.is_open()) {
		util::print_and_throw<std::invalid_argument>([&](auto&& str) {
			str << "unable to open data file\n";
		});
	}

	auto parse_result = input::parse_data(data_file);
	auto visitor = util::compose_withbase<boost::static_visitor<void>>(
		[&](const std::string& err_str) {
			util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << err_str;
			});
		},
		[&](const input::ParseResult& pr) {
			do_optional_input_data_dump(config.dataFileName(), pr);
			satisfy_maximally(pr.expression());
		}
	);
	apply_visitor(visitor, parse_result);

	return 0;
}

struct Graph {
	struct Vertex {
		std::vector<LiteralID>::const_iterator lit_pos;
		bool inverted;

		Literal literal() const {
			return Literal(inverted, *lit_pos);
		}

		// bool operator==(const Vertex& rhs) const {
		// 	return std::forward_as_tuple(lit_pos, inverted, id)
		// 		== std::forward_as_tuple(rhs.lit_pos, rhs.inverted, rhs.id);
		// }
	};


	template<typename LiteralOrder>
	Graph(const LiteralOrder& lo)
		: m_literal_order(begin(lo), end(lo))
	{ }

	auto fanout(const Vertex& source) const {
		auto src_lit_pos = source.lit_pos;
		return util::make_generator<int> (
			isValid(source) ? 0 : 2,
			[](const int& index) { return index == 2; },
			[](const int& index) { return index + 1; },
			[src_lit_pos](const int& index) { return Vertex{ std::next(src_lit_pos), index == 0 ? false : true }; }
		);
	}

	bool hasNextSibling(const Vertex& v) const {
		return v.inverted == false;
	}

	Vertex nextSibling(Vertex&& v) const {
		v.inverted = true;
		return v;
	}

	bool hasFanout(const Vertex& v) const {
		return std::next(v.lit_pos) != end(m_literal_order);
	}

	auto roots() const {
		return std::array<Vertex, 2>{{
			{begin(m_literal_order), false},
			{begin(m_literal_order), true},
		}};
	}

	bool isValid(const Vertex& v) const {
		return v.lit_pos != end(m_literal_order);
	}
private:
	std::vector<LiteralID> m_literal_order;
};

namespace std {
	template<> struct hash<Graph::Vertex> {
		auto operator()(const Graph::Vertex& v) const {
			return std::hash<std::decay_t<decltype(*v.lit_pos)>>()(*v.lit_pos)
				| std::hash<std::decay_t<decltype(v.inverted)>>()(v.inverted)
				// | std::hash<std::decay_t<decltype(v.id)>>()(v.id)
			;
		}
	};
}

struct Visitor : public util::DefaultGraphVisitor<Graph::Vertex> {
	const CNFExpression& expression;
	CNFEvaluation<std::vector> evaluator;
	int num_partial_settings_explored = 0;
	int num_complete_settings_explored = 0;
	std::vector<Literal> best_solution = {};

	Visitor(const CNFExpression& expression)
		: expression(expression)
		, evaluator(expression)
	{ }

	template<typename StateStack>
	void onExplore(const Graph::Vertex& vertex, const StateStack& state_stack) {
		// dout(DL::INFO) << vertex.literal() << ' ';
		const auto& new_counts = evaluator.addSetting(vertex.literal().id(), !vertex.inverted);
		(void)new_counts;
		(void)state_stack;
	}

	void onLeave(const Graph::Vertex& vertex) {
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

struct GraphicsVisitor : Visitor {
	using Visitor::Visitor;

	bool did_graphics_init = false;

	template<typename StateStack>
	void onExplore(const Graph::Vertex& vertex, const StateStack& state_stack) {
		Visitor::onExplore(vertex, state_stack);

		if (not did_graphics_init) {
			did_graphics_init = true;
			graphics::get().tree().setNumLevelsVisible((int)expression.all_disjunctions().size());
		}

		std::vector<Literal> path;
		for (const auto& state : state_stack) {
			path.push_back(state.vertex.literal());
		}
		graphics::get().tree().addPath(std::move(path));
	}
};

void satisfy_maximally(const CNFExpression& expression) {
	using ID = Graph::Vertex;
	const auto graph = Graph{expression.all_literals()};
	const auto initial_list = graph.roots();


	GraphicsVisitor visitor { expression };
	// Visitor visitor { expression };

	struct State {
		ID vertex;
		int parent_lower_bound;
	};

	std::vector<State> seed_states;
	// for (const auto& id : initial_list) {
	// 	seed_states.emplace_back(State{
	// 		id;
	// 	});
	// }
	seed_states.emplace_back(State{
		*begin(initial_list),
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
					if (state_stack.back().parent_lower_bound < best_cost and !state_stack.back().vertex.inverted) {
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

void do_optional_input_data_dump(const std::string& data_file_name, const input::ParseResult& pr) {
	if (!dout(DL::DATA_READ1).enabled()) {
		return;
	}

	const auto indent = dout(DL::DATA_READ1).indentWithTitle("data file name: \"" + data_file_name + '"');
	for (const auto& disjunction : pr.expression().all_disjunctions()) {
		for (const auto& literal : disjunction) {
			dout(DL::DATA_READ1) << literal << ' ';
		}
		dout(DL::DATA_READ1) << '\n';
	}
}
