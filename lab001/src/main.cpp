
#include <device/connectors.hpp>
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

int program_main(const std::string& data_file_name);
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

	const auto result = program_main(parsed_args.getDataFileName());

	graphics::get().close();
	graphics::get().join();

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
			device::FullyConnectedConnector connector(pr.device_info);
			device::Device<device::FullyConnectedConnector> dev(
				pr.device_info, connector
			);


			auto pin = std::next(pr.pin_to_pin_netlist.begin())->first;
			auto pin_re = device::RouteElementID(pin);
			auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
				str << "fanout of " << pin << " (as REID " << pin_re << ')';
			});

			auto pin_fanouts = dev.fanout(pin_re);
			// for (auto& reID : pin_fanouts) {
			// 	dout(DL::INFO) << reID << '\n';
			// }
			for (auto reID_it = begin(pin_fanouts); reID_it != end(pin_fanouts); ++reID_it) {
				const auto& re = *reID_it;
				dout(DL::INFO) << re << '\n';
				auto pin_fanout_fanouts = dev.fanout(re);
				for (auto pff_it = begin(pin_fanout_fanouts); pff_it != end(pin_fanout_fanouts); ++pff_it) {
					dout(DL::INFO) << '\t' << *pff_it << '\n';
					auto pin_fanout_fanout_fanouts = dev.fanout(*pff_it);
					for (auto pfff_it = begin(pin_fanout_fanout_fanouts); pfff_it != end(pin_fanout_fanout_fanouts); ++pfff_it) {
						dout(DL::INFO) << '\t' << '\t' << *pfff_it << '\n';
					}
				}
			}

			graphics::get().fpga().setFCDev(&dev);
			graphics::get().waitForPress();
			graphics::get().fpga().setFCDev(nullptr);
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
