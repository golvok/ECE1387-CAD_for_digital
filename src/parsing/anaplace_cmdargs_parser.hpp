
#ifndef PARSING__ROUTING_CMDARGS_PARSER_H
#define PARSING__ROUTING_CMDARGS_PARSER_H

#include <util/logging.hpp>
#include <device/device.hpp>

#include <string>

#include <boost/optional.hpp>

namespace parsing {

namespace anaplace {

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
	friend ParsedArguments parse(int arc_int, char** argv);
	
	MetaConfig();

	/// The printing levels that should be enabled. Duplicate entries are possible & allowed
	std::vector<DebugLevel::Level> levels_to_enable;
	bool graphics_enabled;
};

struct ProgramConfig {
	const std::string& dataFileName() const { return m_dataFileName; }
	int nThreads() const { return m_nThreads; }

private:
	friend struct ParsedArguments;
	friend ParsedArguments parse(int arc_int, char** argv);

	ProgramConfig();

	std::string m_dataFileName;
	int m_nThreads;
};

struct ParsedArguments {
	const MetaConfig& meta() const { return m_meta; }
	const ProgramConfig& programConfig() const { return m_programConfig; }

private:
	friend ParsedArguments parse(int arc_int, char** argv);

	MetaConfig m_meta;
	ProgramConfig m_programConfig;

	ParsedArguments(int arc_int, char** argv);
};

/**
 * call this function to parse and return a ParsedArguments object from the
 * arguments, which then can be queried about the various options.
 */ 
ParsedArguments parse(int arc_int, char** argv);

} // end namespace cmdargs

} // end namespace anaplace

} // end namespace parsing

#endif /* PARSING__ROUTING_CMDARGS_PARSER_H */
