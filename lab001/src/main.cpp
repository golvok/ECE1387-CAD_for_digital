
#include <algo/routing.hpp>
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
			device::WiltonConnector connector(pr.device_info);
			const device::Device<device::WiltonConnector> dev(
				pr.device_info, connector
			);

			const auto gfx_state_keeper = graphics::get().fpga().pushState(&dev);
			graphics::get().waitForPress();

			bool do_view_reset = true;
			for (int i = 0; i < dev.info().track_width*2; ++i) {
				device::RouteElementID test_re(
					util::make_id<device::XID>((int16_t)2),
					util::make_id<device::YID>((int16_t)4),
					(int16_t)i
				);

				std::unordered_map<device::RouteElementID, graphics::t_color> colours;
				colours[test_re] = graphics::t_color(0,0xFF,0);

				dout(DL::INFO) << test_re << '\n';
				for (const auto& fanout : dev.fanout(test_re)) {
					colours[fanout] = graphics::t_color(0xFF,0,0);
					dout(DL::INFO) << '\t' << fanout << '\n';
				}

				const auto gfx_state_keeper = graphics::get().fpga().pushState(&dev, std::move(colours), do_view_reset);
				do_view_reset = false;
				graphics::get().waitForPress();
			}

			const auto result = algo::route_all(pr.pin_to_pin_netlist, dev);
			for (const auto& source : result.unroutedPins().all_ids()) {
				for (const auto& sink : result.unroutedPins().fanout(source)) {
					dout(DL::WARN) << "failed to route " << source << " -> " << sink << '\n';
				}
			}

			const auto gfx_state_keeper_final_routes = graphics::get().fpga().pushState(&dev, result.netlist());
			graphics::get().waitForPress();

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
