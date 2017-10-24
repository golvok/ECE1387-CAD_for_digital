
#ifndef PARSING__ROUTING_CMDARGS_PARSER_H
#define PARSING__ROUTING_CMDARGS_PARSER_H

#include <util/logging.hpp>
#include <device/device.hpp>

#include <string>

#include <boost/optional.hpp>

namespace parsing {

namespace routing {

namespace cmdargs {

class ParsedArguments {
public:
	/**
	 * Return the list of debug levels that shold be enabled given the command line options
	 * NOTE: May contain duplicates.
	 */
	const std::vector<DebugLevel::Level> getDebugLevelsToEnable() const {
		return levels_to_enable;
	}

	/**
	 * Should the current invocation of the program display graphics?
	 */
	bool shouldEnableGraphics() const  { return graphics_enabled; }
	bool shouldDoFanoutTest() const { return fanout_test; }
	bool shouldJustRouteAsIs() const { return route_as_is; }
	const auto& deviceTypeOverride() const { return device_type_override; }
	const boost::optional<int>& channelWidthOverride() const { return channel_width_override; }
	const std::string& getDataFileName() const { return data_file_name; }
	int nThreads() const { return m_nThreads; }

private:
	friend ParsedArguments parse(int arc_int, char const** argv);

	bool graphics_enabled;
	bool fanout_test;
	bool route_as_is;
	boost::optional<int> channel_width_override;
	boost::optional<device::DeviceTypeID> device_type_override;

	/// The printing levels that should be enabled. Duplicate entries are possible & allowed
	std::vector<DebugLevel::Level> levels_to_enable;

	std::string data_file_name;

	int m_nThreads;

	ParsedArguments(int arc_int, char const** argv);
};

/**
 * call this function to parse and return a ParsedArguments object from the
 * arguments, which then can be queried about the various options.
 */
ParsedArguments parse(int arc_int, char const** argv);

} // end namespace cmdargs

} // end namespace routing

} // end namespace parsing

#endif /* PARSING__ROUTING_CMDARGS_PARSER_H */
