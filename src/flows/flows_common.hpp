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

	template<typename FixedBlockLocations_Param>
	FlowBase<Device, FixedBlockLocations_Param> withFixedBlockLocations(const FixedBlockLocations_Param& newFBL) const {
		return {
			dev,
			newFBL,
			nThreads
		};
	}

	const Device& dev;
	const FixedBlockLocations& fixed_block_locations;
	const int nThreads;
};

#define DECLARE_USING_FLOBASE_MEMBERS(dev_tparam, fbl_tparam) \
	using FlowBase<dev_tparam, fbl_tparam>::FlowBase; \
	using FlowBase<dev_tparam, fbl_tparam>::dev; \
	using FlowBase<dev_tparam, fbl_tparam>::nThreads; \
	using FlowBase<dev_tparam, fbl_tparam>::fixed_block_locations; \
	using FlowBase<dev_tparam, fbl_tparam>::withDevice; \
	using FlowBase<dev_tparam, fbl_tparam>::withFixedBlockLocations; \


} // end namespace flows

#endif // FLOWS__FLOWS_COMMON_H
