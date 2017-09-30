
#include <flows/flows.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <parsing/cmdargs_parser.hpp>
#include <parsing/input_parser.hpp>
#include <util/lambda_compose.hpp>
#include <util/logging.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include <boost/optional.hpp>

struct ProgramConfig {
	std::string data_file_name;
	bool fanout_test;
};

int program_main(const ProgramConfig& config);

void do_optional_input_data_dump(const std::string& data_file_name, const parsing::input::ParseResult& pr);

int main(int argc, char const** argv) {

	dout.setHighestTitleRank(7);

	const auto parsed_args = parsing::cmdargs::parse(argc,argv);
	// enable logging levels
	for (auto& l : parsed_args.getDebugLevelsToEnable()) {
		dout.enable_level(l);
	}

	// enable graphics
	if (parsed_args.shouldEnableGraphics()) {
		graphics::get().enable();
		graphics::get().startThreadsAndOpenWindow();
	}

	const auto result = program_main(ProgramConfig{
		parsed_args.getDataFileName(),
		parsed_args.shouldDoFanoutTest(),
	});

	graphics::get().close();
	graphics::get().join();

	return result;
}

int program_main(const ProgramConfig& config) {

	std::ifstream data_file(config.data_file_name);

	auto parse_result = parsing::input::parse_data(data_file);
	auto visitor = util::compose<boost::static_visitor<void>>(
		[&](const std::string& err_str) {
			util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << err_str;
			});
		},
		[&](const parsing::input::ParseResult& pr) {
			do_optional_input_data_dump(config.data_file_name, pr);
			if (config.fanout_test) {
				flows::fanout_test(pr.device_info);
			}
			flows::route_as_is(pr.device_info, pr.pin_to_pin_netlist);
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
	for (const auto& source : pr.pin_to_pin_netlist.roots()) {
		for (const auto& sink : pr.pin_to_pin_netlist.fanout(source)) {
			dout(DL::DATA_READ1) << source << " -> " << sink << '\n';
		}
	}
}
