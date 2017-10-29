
#include <flows/placement_flows.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <parsing/anaplace_cmdargs_parser.hpp>
#include <parsing/anaplace_datafile_parser.hpp>
#include <util/lambda_compose.hpp>
#include <util/logging.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include <boost/optional.hpp>

using namespace parsing::anaplace;
using namespace parsing::anaplace::cmdargs;

int program_main(const ProgramConfig& config);

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

	auto parse_result = input::parse_data(data_file);
	auto visitor = util::compose_withbase<boost::static_visitor<void>>(
		[&](const std::string& err_str) {
			util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << err_str;
			});
		},
		[&](const input::ParseResult& pr) {
			do_optional_input_data_dump(config.dataFileName(), pr);
			const auto& device_desc = flows::placement::make_default_device_description(pr.netlist());

			flows::placement::simple_clique_solve(pr.netlist(), pr.fixedBlockLocations(), device_desc);
		}
	);
	apply_visitor(visitor, parse_result);

	return 0;
}

void do_optional_input_data_dump(const std::string& data_file_name, const input::ParseResult& pr) {
	if (!dout(DL::DATA_READ1).enabled()) {
		return;
	}

	const auto indent = dout(DL::DATA_READ1).indentWithTitle("Input Data Dump");
	dout(DL::DATA_READ1) << "using input file: " << data_file_name << '\n';

	auto dump_netlist = [](const auto& netlist) {
		dout(DL::DATA_READ1) << "roots:";
		util::print_container(netlist.roots(), dout(DL::DATA_READ1));
		dout(DL::DATA_READ1) << '\n';
		for (const auto& node : netlist.all_ids()) {
			dout(DL::DATA_READ1) << node << "->";
			for (const auto& fanout_node : netlist.fanout(node)) {
				dout(DL::DATA_READ1) << fanout_node << ", ";
			}
			dout(DL::DATA_READ1) << '\n';
		}
	};

	{const auto indent = dout(DL::DATA_READ1).indentWithTitle("As-Parsed Netlist");
	dump_netlist(pr.netlistAsParsed());}

	{const auto indent = dout(DL::DATA_READ1).indentWithTitle("Main Netlist");
	dump_netlist(pr.netlist());}
}
