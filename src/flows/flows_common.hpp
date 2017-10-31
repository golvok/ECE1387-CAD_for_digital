#ifndef FLOWS__FLOWS_COMMON_H
#define FLOWS__FLOWS_COMMON_H

namespace flows {

template<typename Device, typename FixedBlockLocations = int>
class FlowBase {
public:
	FlowBase(const FlowBase&) = default;
	FlowBase(FlowBase&&) = default;

	FlowBase(
		const Device& dev,
		int nThreads
	)
		: FlowBase(
			dev,
			{},
			nThreads
		)
	{ }

	FlowBase(
		const Device& dev,
		const FixedBlockLocations& fixed_block_locations,
		int nThreads
	)
		: dev(dev)
		, fixed_block_locations(fixed_block_locations)
		, nThreads(nThreads)
	{ }

	FlowBase withDevice(const Device& newDev) const {
		return {
			newDev,
			nThreads
		};
	}

	const Device& dev;
	const FixedBlockLocations& fixed_block_locations;
	const int nThreads;
};

} // end namespace flows

#endif // FLOWS__FLOWS_COMMON_H
