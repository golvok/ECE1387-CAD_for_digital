
#ifndef PARSING__ROUTING_CMDARGS_PARSER_H
#define PARSING__ROUTING_CMDARGS_PARSER_H

#include <util/logging.hpp>

#include <string>

#include <boost/optional.hpp>

namespace parsing {

namespace sat_maxer {

namespace cmdargs {

struct ParsedArguments;

struct MetaConfig {
	/**
	 * Return the list of debug levels that shold be enabled given the command line options
	 * NOTE: May contain duplicates.
	 */
	const std::vector<DebugLevel::Level> getDebugLevelsToEnable() const {
		return levels_to_enable;
	}

	bool shouldEnableGraphics() const  { return graphics_enabled; }

private:
	friend struct ParsedArguments;
	friend ParsedArguments parse(int arc_int, char const** argv);
	
	MetaConfig();

	/// The printing levels that should be enabled. Duplicate entries are possible & allowed
	std::vector<DebugLevel::Level> levels_to_enable;
	bool graphics_enabled;
};

struct ProgramConfig {
	const std::string& dataFileName() const { return m_dataFileName; }

private:
	friend struct ParsedArguments;
	friend ParsedArguments parse(int arc_int, char const** argv);

	ProgramConfig();

	std::string m_dataFileName;
};

struct ParsedArguments {
	const MetaConfig& meta() const { return m_meta; }
	const ProgramConfig& programConfig() const { return m_programConfig; }

private:
	friend ParsedArguments parse(int arc_int, char const** argv);

	MetaConfig m_meta;
	ProgramConfig m_programConfig;

	ParsedArguments(int arc_int, char const** argv);
};

/**
 * call this function to parse and return a ParsedArguments object from the
 * arguments, which then can be queried about the various options.
 */ 
ParsedArguments parse(int arc_int, char const** argv);

} // end namespace cmdargs

} // end namespace sat_maxer

} // end namespace parsing

#endif /* PARSING__ROUTING_CMDARGS_PARSER_H */
