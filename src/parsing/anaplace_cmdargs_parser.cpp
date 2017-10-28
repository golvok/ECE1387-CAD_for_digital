
#include "anaplace_cmdargs_parser.hpp"

#include <device/connectors.hpp>

#include <unordered_set>

#include <boost/program_options.hpp>

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
	namespace po = boost::program_options;

	po::options_description metaopts("Meta Options");
	po::options_description progopts("Program Options");

	metaopts.add_options()
		("graphics", "Enable graphics")
		("debug",    "Turn on the most common debugging options")
	;
	DebugLevel::forEachLevel([&](DebugLevel::Level l) {
		metaopts.add_options()(("DL::" + DebugLevel::getAsString(l)).c_str(), "");
	});

	progopts.add_options()
		("circuit", po::value(&m_programConfig.m_dataFileName), "The circuit to use")
		("num-threads", po::value(&m_programConfig.m_nThreads), "The maximum nuber of simultaneous threads to use")
	;

	po::options_description allopts;
	allopts.add(metaopts).add(progopts);

	po::variables_map vm;
	po::store(po::parse_command_line(argc_int, argv, allopts), vm);
	po::notify(vm);

	if (vm.count("debug")) {
		auto debug_levels = DebugLevel::getStandardDebug();
		m_meta.levels_to_enable.insert(end(m_meta.levels_to_enable),begin(debug_levels),end(debug_levels));
	}

	DebugLevel::forEachLevel([&](DebugLevel::Level l) {
		if (vm.count("DL::" + DebugLevel::getAsString(l))) {
			auto levels_in_chain = DebugLevel::getAllShouldBeEnabled(l);
			m_meta.levels_to_enable.insert(end(m_meta.levels_to_enable),begin(levels_in_chain),end(levels_in_chain));
		}
	});

}

ParsedArguments parse(int arc_int, char const** argv) {
	return ParsedArguments(arc_int, argv);
}

} // end namespace cmdargs

} // end namespace anaplace

} // end namespace parsing

