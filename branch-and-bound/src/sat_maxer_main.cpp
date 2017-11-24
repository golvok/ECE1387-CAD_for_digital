
#include <algo/branch_and_bound.hpp>
#include <algo/max_sat.hpp>
#include <datastructures/cnf_tree.hpp>
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

int program_main(const ProgramConfig& config, bool enable_graphics_calls);
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

	const auto result = program_main(parsed_args.programConfig(), parsed_args.meta().shouldEnableGraphics());
	// graphics::get().close();
	graphics::get().join();

	return result;

} catch (std::exception& e) { std::cerr << "Uncaught exception: .what() = \"" << e.what() << '"' << std::endl; return -1; } }


template<typename Vertex>
struct GraphicsVisitor : DefaultMaxSatVisitor<Vertex> {
	using DefaultMaxSatVisitor<Vertex>::DefaultMaxSatVisitor;
	using DefaultMaxSatVisitor<Vertex>::expression;

	bool did_graphics_init = false;

	template<typename StateStack>
	void onExplore(const Vertex& vertex, const StateStack& state_stack) {
		DefaultMaxSatVisitor<Vertex>::onExplore(vertex, state_stack);

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

int program_main(const ProgramConfig& config, bool enable_graphics_calls) {

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
			const auto graph = Graph{pr.expression().all_literals()};
			using ID = decltype(graph)::Vertex;

			if (enable_graphics_calls) {
				GraphicsVisitor<ID> visitor { pr.expression() };
				branchAndBound<ID>(visitor, graph);
			} else {
				DefaultMaxSatVisitor<ID> visitor { pr.expression() };
				branchAndBound<ID>(visitor, graph);
			}
		}
	);
	apply_visitor(visitor, parse_result);

	return 0;
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
