
#include "anaplace_cmdargs_parser.hpp"

#include <device/connectors.hpp>

#include <unordered_set>

namespace parsing {

namespace anaplace {

namespace cmdargs {

MetaConfig::MetaConfig()
	: levels_to_enable(DebugLevel::getDefaultSet())
	, graphics_enabled(false)
{ }

ProgramConfig::ProgramConfig()
	: m_dataFileName()
	, m_nThreads(2)
{ }

ParsedArguments::ParsedArguments(int argc_int, char const** argv)
	: m_meta()
	, m_programConfig()
{
	uint arg_count = argc_int;
	std::vector<std::string> args;
	std::unordered_set<std::ptrdiff_t> used;

	// add all but first
	for (uint i = 1; i < arg_count; ++i) {
		args.emplace_back(argv[i]);
	}

	{
		auto arg_it = std::find(begin(args),end(args),"--graphics");
		if (arg_it != end(args)) {
			m_meta.graphics_enabled = true;
			used.insert(std::distance(begin(args), arg_it));
		}
	}

	{
		const auto arg_it = std::find(begin(args),end(args),"--debug");
		if (arg_it != end(args)) {
			auto debug_levels = DebugLevel::getStandardDebug();
			m_meta.levels_to_enable.insert(end(m_meta.levels_to_enable),begin(debug_levels),end(debug_levels));
			used.insert(std::distance(begin(args), arg_it));
		}
	}

	std::string prefix("--DL::");
	for (auto arg_it = begin(args); arg_it != end(args); ++arg_it) {
		const auto arg = *arg_it;
		if (std::mismatch(begin(prefix),end(prefix),begin(arg),end(arg)).first != end(prefix)) {
			continue;
		}

		const auto level_string = arg.substr(prefix.size(),std::string::npos);
		auto result = DebugLevel::getFromString(level_string);
		if (result.second) {
			auto levels_in_chain = DebugLevel::getAllShouldBeEnabled(result.first);
			m_meta.levels_to_enable.insert(end(m_meta.levels_to_enable),begin(levels_in_chain),end(levels_in_chain));
			used.insert(std::distance(begin(args), arg_it));
		} else {
			util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "don't understand debug level " << level_string;
			});
		}
	}

	{
		auto data_flag_it = std::find(begin(args),end(args),"--data-file");
		if (data_flag_it == end(args)) {
			util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "--data-file is required";
			});
		} else {
			auto data_file_it = std::next(data_flag_it);
			if (data_file_it == end(args)) {
				util::print_and_throw<std::invalid_argument>([&](auto&& str) {
					str << "--data-file requires an argument";
				});
			} else {
				m_programConfig.m_dataFileName = *data_file_it;
				used.insert(std::distance(begin(args), data_flag_it));
				used.insert(std::distance(begin(args), data_file_it));
			}
		}
	}

	{
		auto thread_flag_it = std::find(begin(args),end(args),"--num-threads");
		if (thread_flag_it != end(args)) {
			auto thread_number_it = std::next(thread_flag_it);
			if (thread_number_it == end(args)) {
				util::print_and_throw<std::invalid_argument>([&](auto&& str) {
					str << "--num-threads requires an argument";
				});
			} else {
				std::size_t pos = thread_number_it->size();
				auto result = std::stoi(*thread_number_it);
				if (pos != thread_number_it->size()) {
					util::print_and_throw<std::invalid_argument>([&](auto&& str) {
						str << "--num-threads argument is malformed";
					});
				}
				m_programConfig.m_nThreads = result;
				used.insert(std::distance(begin(args), thread_flag_it));
				used.insert(std::distance(begin(args), thread_number_it));
			}
		}
	}

	std::vector<size_t> unused;
	for (size_t i = 0; i < args.size(); ++i) {
		if (used.find(i) == end(used)) {
			unused.push_back(i);
		}
	}

	if (!unused.empty()) {
		util::print_and_throw<std::invalid_argument>([&](auto&& str) {
			str << "didn't understand the following arguments:\n";
			for (const auto& index : unused) {
				str << '\t' << args[index] << '\n';
			}
		});
	}

}

ParsedArguments parse(int arc_int, char const** argv) {
	return ParsedArguments(arc_int, argv);
}

} // end namespace cmdargs

} // end namespace anaplace

} // end namespace parsing

