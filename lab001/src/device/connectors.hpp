#ifndef DEVICE__CONNECTORS_H
#define DEVICE__CONNECTORS_H

#include <device/device.hpp>
#include <parsing/input_parser.hpp>

namespace device {

struct UniversalConnector {
	using Index = std::int16_t;
	const Index END_VALUE = std::numeric_limits<Index>::max();

	parsing::input::DeviceInfo dev_info;

	UniversalConnector(const parsing::input::DeviceInfo& dev_info)
		: dev_info(dev_info)
	{ }
	UniversalConnector(const UniversalConnector&) = default;
	UniversalConnector& operator=(const UniversalConnector&) = default;
	UniversalConnector(UniversalConnector&&) = default;
	UniversalConnector& operator=(UniversalConnector&&) = default;

	Index fanout_begin(const RouteElementID& re) const {
		(void)re;
		return 0;
	}

	bool is_end_index(const RouteElementID& re, const Index& index) const {
		(void)re;
		return index == END_VALUE;
	}

	Index next_fanout(const RouteElementID& re, Index index) const {
		(void)re;
		auto next = static_cast<Index>(index + 1);
		if (re.isPin()) {
			if (next > dev_info.track_width - 1) {
				return END_VALUE;
			} else {
				return next;
			}
		} else {
			if (next > dev_info.pins_per_block*2 + dev_info.track_width*3 - 1) {
				return END_VALUE;
			} else {
				return next;
			}
		}
	}

	RouteElementID re_from_index(const RouteElementID& re, Index index) const {
		if (re.isPin()) {
			return RouteElementID(re.getX(), re.getY(), index);
		} else {
			if (index < dev_info.pins_per_block) {
				return RouteElementID(
					PinGID(
						BlockID(re.getX(), re.getY()),
						util::make_id<BlockPinID>(index)
					)
				);
			} else if (index < dev_info.pins_per_block*2) {
				return RouteElementID(
					PinGID(
						BlockID(re.getX(), util::make_id<YID>(static_cast<YID::IDType>(re.getY().getValue() + 1))),
						util::make_id<BlockPinID>(static_cast<BlockPinID::IDType>(index - dev_info.pins_per_block))
					)
				);
			} else {
				return RouteElementID(re.getX(), re.getY(), index);
			}
		}
	}
};

}

#endif // DEVICE__CONNECTORS_H
