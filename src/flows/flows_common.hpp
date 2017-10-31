#ifndef FLOWS__FLOWS_COMMON_H
#define FLOWS__FLOWS_COMMON_H

namespace flows {

template<typename Device>
class FlowBase {
public:
	FlowBase(const FlowBase&) = default;
	FlowBase(FlowBase&&) = default;

	FlowBase(
		const Device& dev,
		int nThreads
	)
		: dev(dev)
		, nThreads(nThreads)
	{ }

	FlowBase withDevice(const Device& newDev) const {
		return {
			newDev,
			nThreads
		};
	}

	const Device& dev;
	const int nThreads;
};

} // end namespace flows

#endif // FLOWS__FLOWS_COMMON_H
