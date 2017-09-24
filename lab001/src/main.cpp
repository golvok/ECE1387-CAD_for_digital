
#include <graphics/graphics.hpp>
#include <parsing/cmdargs_parser.hpp>
#include <parsing/input_parser.hpp>
#include <util/lambda_compose.hpp>
#include <util/logging.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>

int program_main(const std::string& data_file_name);
void do_optional_input_data_dump(const std::string& data_file_name, const parsing::input::ParseResult& pr);

int main(int argc, char const** argv) {


	dout.setHighestTitleRank(7);

	const auto parsed_args = parsing::cmdargs::parse(argc,argv);

	// enable graphics
	std::vector<std::thread> g_threads;
	if (parsed_args.shouldEnableGraphics()) {
		g_threads.emplace_back([&](){
			std::string title;
			for (int i = 0; i < argc; ++i) {
				title += argv[i];
				if (i != argc-1) {
					title += ' ';
				}
			}
			graphics::init_graphics(title, graphics::WHITE);
			graphics::event_loop([](float, float, graphics::t_event_buttonPressed){}, nullptr, nullptr, [](){});
		});
	}

	// enable logging levels
	for (auto& l : parsed_args.getDebugLevelsToEnable()) {
		dout.enable_level(l);
	}

	const auto result = program_main(parsed_args.getDataFileName());

	for (auto& t : g_threads) {
		t.join();
	}

	return result;
}

int program_main(const std::string& data_file_name) {

	std::ifstream data_file(data_file_name);

	auto parse_result = parsing::input::parse_data(data_file);
	auto visitor = util::compose<boost::static_visitor<void>>(
		[&](const std::string& err_str) {
			util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << err_str;
			});
		},
		[&](const parsing::input::ParseResult& pr) {
			do_optional_input_data_dump(data_file_name, pr);
		}
	);
	apply_visitor(visitor, parse_result);

	return 0;
}

void do_optional_input_data_dump(const std::string& data_file_name, const parsing::input::ParseResult& pr) {
	if (!dout(DL::DATA_READ1).enabled()) {
		return;
	}

	const auto indent = dout(DL::DATA_READ1).indentWithTitle("Input Data Dump");
	dout(DL::DATA_READ1) << "using input file: " << data_file_name << '\n';
	for (const auto& src_and_sinks : pr.pin_to_pin_netlist) {
		for (const auto& sink : src_and_sinks.second) {
			dout(DL::DATA_READ1) << src_and_sinks.first << " -> " << sink << '\n';
		}
	}
}
