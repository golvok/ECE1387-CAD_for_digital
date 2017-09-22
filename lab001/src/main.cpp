
#include <graphics/graphics.hpp>
#include <parsing/cmdargs_parser.hpp>
#include <util/logging.hpp>

#include <thread>

int program_main(const std::string& data_file_name);

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
	dout(DL::INFO) << data_file_name << '\n';
	return 0;
}