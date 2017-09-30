
#include "cmdargs_parser.hpp"

namespace parsing {

namespace cmdargs {

ParsedArguments::ParsedArguments(int argc_int, char const** argv)
	: graphics_enabled(false)
	, fanout_test(false)
	, levels_to_enable(DebugLevel::getDefaultSet())
	, data_file_name()
 {
	uint arg_count = argc_int;
	std::vector<std::string> args;

	// add all but first
	for (uint i = 1; i < arg_count; ++i) {
		args.emplace_back(argv[i]);
	}

	if (std::find(begin(args),end(args),"--graphics") != end(args)) {
		graphics_enabled = true;
	}

	if (std::find(begin(args),end(args),"--debug") != end(args)) {
		auto debug_levels = DebugLevel::getStandardDebug();
		levels_to_enable.insert(end(levels_to_enable),begin(debug_levels),end(debug_levels));
	}

	if (std::find(begin(args),end(args),"--fanout-test") != end(args)) {
		fanout_test = true;
	}

	std::string prefix("--DL::");
	for (auto& arg : args) {
		if (std::mismatch(begin(prefix),end(prefix),begin(arg),end(arg)).first != end(prefix)) {
			continue;
		}

		const auto level_string = arg.substr(prefix.size(),std::string::npos);
		auto result = DebugLevel::getFromString(level_string);
		if (result.second) {
			auto levels_in_chain = DebugLevel::getAllShouldBeEnabled(result.first);
			levels_to_enable.insert(end(levels_to_enable),begin(levels_in_chain),end(levels_in_chain));
		} else {
			util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "don't understand debug level " << level_string;
			});
		}
	}

	{
		auto data_flag_it = std::find(begin(args),end(args),"--data-file");
		if (data_flag_it != end(args)) {
			auto data_file_it = std::next(data_flag_it);
			if (data_file_it != end(args)) {
				data_file_name = *data_file_it;
			}
		}
	}

}

ParsedArguments parse(int arc_int, char const** argv) {
	return ParsedArguments(arc_int, argv);
}

} // end namespace cmdargs

} // end namespace parsing

