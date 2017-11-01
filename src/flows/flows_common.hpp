#ifndef FLOWS__FLOWS_COMMON_H
#define FLOWS__FLOWS_COMMON_H

namespace flows {

template<typename Device, typename FixedBlockLocations = int>
class FlowBaseCommonData {
public:
	FlowBaseCommonData(const FlowBaseCommonData&) = default;
	FlowBaseCommonData(FlowBaseCommonData&&) = default;

	FlowBaseCommonData(
		const Device& dev,
		int nThreads
	)
		: FlowBaseCommonData(
			dev,
			{},
			nThreads
		)
	{ }

	FlowBaseCommonData(
		const Device& dev,
		const FixedBlockLocations& fixed_block_locations,
		int nThreads
	)
		: dev(dev)
		, fixed_block_locations(fixed_block_locations)
		, nThreads(nThreads)
	{ }

	const Device& dev;
	const FixedBlockLocations& fixed_block_locations;
	const int nThreads;
};

template<typename Self, typename Device, typename FixedBlockLocations = int>
struct FlowBase : FlowBaseCommonData<Device, FixedBlockLocations> {
	FlowBase(const FlowBase&) = default;
	FlowBase(FlowBase&&) = default;

	FlowBase(const FlowBaseCommonData<Device, FixedBlockLocations>& fbcd) : FlowBaseCommonData<Device, FixedBlockLocations>(fbcd) { }
	FlowBase(FlowBaseCommonData<Device, FixedBlockLocations>&& fbcd) : FlowBaseCommonData<Device, FixedBlockLocations>(std::move(fbcd)) { }
	
	using FlowBaseCommonData<Device, FixedBlockLocations>::FlowBaseCommonData;
	using FlowBaseCommonData<Device, FixedBlockLocations>::dev;
	using FlowBaseCommonData<Device, FixedBlockLocations>::fixed_block_locations;
	using FlowBaseCommonData<Device, FixedBlockLocations>::nThreads;

	Self withDevice(const Device& newDev) const {
		return {
			newDev,
			nThreads
		};
	}

	Self withFixedBlockLocations(const FixedBlockLocations& newFBL) const {
		return {
			dev,
			newFBL,
			nThreads
		};
	}
};

#define DECLARE_USING_FLOWBASE_MEMBERS_JUST_MEMBERS(...) \
	using __VA_ARGS__::FlowBase; \
	using __VA_ARGS__::dev; \
	using __VA_ARGS__::nThreads; \
	using __VA_ARGS__::fixed_block_locations; \


#define DECLARE_USING_FLOWBASE_MEMBERS(Self, ...) \
	using __VA_ARGS__::withDevice; \
	using __VA_ARGS__::withFixedBlockLocations; \
	DECLARE_USING_FLOWBASE_MEMBERS_JUST_MEMBERS(__VA_ARGS__) \
	Self(const __VA_ARGS__& fb) : __VA_ARGS__(fb) { } \
	Self(const __VA_ARGS__&& fb) : __VA_ARGS__(std::move(fb)) { } \


} // end namespace flows

#endif // FLOWS__FLOWS_COMMON_H
