
#include "sat_maxer_cmdargs_parser.hpp"

#include <unordered_set>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

namespace parsing {

namespace sat_maxer {

namespace cmdargs {

MetaConfig::MetaConfig()
	: levels_to_enable(DebugLevel::getDefaultSet())
	, graphics_enabled(false)
{ }

ProgramConfig::ProgramConfig()
	: m_dataFileName()
	, m_variableOrder()
{ }

ParsedArguments::ParsedArguments(int argc_int, char const** argv)
	: m_meta()
	, m_programConfig()
{
	namespace po = boost::program_options;

	po::options_description metaopts("Meta Options");
	po::options_description progopts("Program Options");

	metaopts.add_options()
		("graphics", po::bool_switch(&m_meta.graphics_enabled), "Enable graphics")
		("debug",    "Turn on the most common debugging output options")
	;
	DebugLevel::forEachLevel([&](DebugLevel::Level l) {
		metaopts.add_options()(("DL::" + DebugLevel::getAsString(l)).c_str(), "debug output control flag");
	});

	std::string vo_helpstring;
	for (const auto& vo : allVariableOrders()) {
		for (const auto& s : stringsFor(vo)) {
			vo_helpstring += s;
			vo_helpstring += ", ";
		}
	}
	// remove training comma + space
	vo_helpstring.erase(std::prev(std::prev(end(vo_helpstring))), end(vo_helpstring));

	progopts.add_options()
		("help,h", "print help message")
		("problem-file,f", po::value(&m_programConfig.m_dataFileName)->required(), "The file with the SAT-MAX problem to solve")
		("variable-order,r", po::value<std::string>()->default_value("GBD,MCF,F"), ("Comma or (single-token) space separated list of sort orders, interpreted as a hierarchy with top level first. Valid strings: " + vo_helpstring).c_str())
	;

	po::options_description allopts;
	allopts.add(progopts).add(metaopts);

	po::variables_map vm;
	po::store(po::parse_command_line(argc_int, argv, allopts), vm);

	// check for help flag before call to notify - don't care about required arguments in this case.
	if (vm.count("help")) {
		std::cerr << allopts << std::endl;
		exit(0);
	}

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

	{
		std::vector<std::string> tokens;
		boost::split(tokens, vm["variable-order"].as<std::string>(), boost::is_any_of(", "));
		for (const auto& token : tokens) {
			if (tokens.empty()) { continue; }
			m_programConfig.m_variableOrder.push_back(variableOrderFromString(token));
		}
	}
}

ParsedArguments parse(int arc_int, char const** argv) {
	return ParsedArguments(arc_int, argv);
}

} // end namespace cmdargs

} // end namespace sat_maxer

} // end namespace parsing

