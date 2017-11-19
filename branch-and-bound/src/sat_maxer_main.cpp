
#include <graphics/graphics_wrapper_sat_maxer.hpp>
#include <parsing/sat_maxer_cmdargs_parser.hpp>
#include <parsing/sat_maxer_datafile_parser.hpp>
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

int main(int argc, char const** argv) {

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

	graphics::get().close();
	graphics::get().join();

	return result;
}

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
		int id;

		Literal literal() const {
			return Literal(inverted, *lit_pos);
		}

		bool operator==(const Vertex& rhs) const {
			return std::forward_as_tuple(lit_pos, inverted, id)
				== std::forward_as_tuple(rhs.lit_pos, rhs.inverted, rhs.id);
		}
	};


	template<typename LiteralOrder>
	Graph(const LiteralOrder& lo)
		: m_literal_order(begin(lo), end(lo))
		, next_id(0)
	{ }

	auto fanout(const Vertex& v) const {
		return std::array<Vertex, 2>{{
			{std::next(v.lit_pos), true, ++next_id},
			{std::next(v.lit_pos), false, ++next_id},
		}};
	}

	auto roots() const {
		return std::array<Vertex, 2>{{
			{begin(m_literal_order), true, ++next_id},
			{begin(m_literal_order), false, ++next_id},
		}};
	}


	bool isValid(const Vertex& v) const {
		return v.lit_pos != end(m_literal_order);
	}
private:
	std::vector<LiteralID> m_literal_order;
	mutable int next_id;
};

namespace std {
	template<> struct hash<Graph::Vertex> {
		auto operator()(const Graph::Vertex& v) const {
			return std::hash<std::decay_t<decltype(*v.lit_pos)>>()(*v.lit_pos)
				| std::hash<std::decay_t<decltype(v.inverted)>>()(v.inverted)
				| std::hash<std::decay_t<decltype(v.id)>>()(v.id)
			;
		}
	};
}

void satisfy_maximally(const CNFExpression& expression) {
	const auto graphAlgo = util::GraphAlgo<Graph::Vertex>();

	const auto graph = Graph{expression.all_literals()};

	struct Visitor : public util::DefaultGraphVisitor<Graph::Vertex> {
		void onExplore(const Graph::Vertex& vertex) const {
			// TODO update lower bound
			dout(DL::INFO) << vertex.literal() << ' ';
			(void)vertex;
		}
	} visitor;

	graphAlgo.wavedBreadthFirstVisit(
		graph,
		graph.roots(),
		[&](const auto&) {
			// want to explore untill tree is exhausted
			return false;
		},
		visitor,
		[&](const auto& vertex) {
			// TODO check againts lower bound too
			return !graph.isValid(vertex);
		}
	);
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
