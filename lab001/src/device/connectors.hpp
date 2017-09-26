#ifndef DEVICE__CONNECTORS_H
#define DEVICE__CONNECTORS_H

#include <device/device.hpp>
#include <parsing/input_parser.hpp>

namespace device {

namespace {
	RouteElementID offset_re_new_index(const RouteElementID& re, int xdiff, int ydiff, int index, bool swap_xy = false) {
		return RouteElementID(
			util::make_id<XID>(static_cast<XID::IDType>(re.getX().getValue() + (swap_xy ? ydiff : xdiff))),
			util::make_id<YID>(static_cast<YID::IDType>(re.getY().getValue() + (swap_xy ? xdiff : ydiff))),
			static_cast<std::int16_t>(index)
		);
	}

	RouteElementID offset_re_same_index(const RouteElementID& re, int xdiff, int ydiff, bool swap_xy = false) {
		return RouteElementID(
			util::make_id<XID>(static_cast<XID::IDType>(re.getX().getValue() + (swap_xy ? ydiff : xdiff))),
			util::make_id<YID>(static_cast<YID::IDType>(re.getY().getValue() + (swap_xy ? xdiff : ydiff))),
			static_cast<std::int16_t>(re.getIndex())
		);
	}
}


/*
 *   001122334
 * 4 +-+-+-+-+
 * 3 |X|X|X|X|
 * 3 +-+-+-+-+
 * 2 |X|X|X|X|
 * 2 +-+-+-+-+
 * 1 |X|X|X|X|
 * 1 +-+-+-+-+
 * 0 |X|X|X|X|
 * 0 +-+-+-+-+
 */

struct UniversalConnector {
	using Index = std::int16_t;
	const Index END_VALUE = std::numeric_limits<Index>::max();

	parsing::input::DeviceInfo dev_info;
	decltype(dev_info.bounds) wire_bb;

	UniversalConnector(const parsing::input::DeviceInfo& dev_info)
		: dev_info(dev_info)
		, wire_bb(dev_info.bounds.min_point(), dev_info.bounds.max_point() + geom::make_point(1,1))
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
		if (re.isPin()) {
			auto next = static_cast<Index>(index + 1);
			if (next > dev_info.track_width - 1) {
				return END_VALUE;
			} else {
				return next;
			}
		} else {
			auto next = index;
			while (true) {
				next = static_cast<Index>(next + 1);
				if (next > dev_info.pins_per_block_side*dev_info.num_blocks_adjacent_to_channel + dev_info.track_width*6 - 1) {
					return END_VALUE;
				} else {
					auto result = re_from_index(re, next);
					if (dev_info.bounds.intersects(result.getX().getValue(), result.getY().getValue())) {
						return next;
					} else {
						// allow top row & right col
						if (!result.isPin() && wire_bb.intersects(result.getX().getValue(), result.getY().getValue())) {
							return next;
						}
					}
				}
			}
		}
	}

	RouteElementID re_from_index(const RouteElementID& re, const Index out_index) const {
		if (re.isPin()) {
			const auto as_pin = re.asPin();
			const auto pin_side = (as_pin.getBlockPin().getValue() - 1) % 4 + 1;
			const bool output_to_horiz = pin_side % 2 == 1;
			const auto result_index_value = (output_to_horiz ? dev_info.track_width : 0) + (out_index % dev_info.track_width);
			const auto base_re = RouteElementID(re.getX(), re.getY(), util::no_sign_ext_cast<RouteElementID::REIndex>(result_index_value));
			switch (pin_side) {
				case 1:
					return offset_re_same_index(base_re, 0, 0);
				case 2:
					return offset_re_same_index(base_re, 1, 0);
				case 3:
					return offset_re_same_index(base_re, 0, 1);
				case 4:
					return offset_re_same_index(base_re, 0, 0);
				default:
					return RouteElementID();
			}
		} else {
			const bool is_horiz = re.getIndex()/dev_info.track_width;
			if (out_index < dev_info.pins_per_block_side*dev_info.num_blocks_adjacent_to_channel) {
				const auto block_pin_id = static_cast<BlockPinID::IDType>(
					dev_info.num_blocks_adjacent_to_channel*(out_index % dev_info.num_blocks_adjacent_to_channel) + 1
				);
				if (is_horiz) {
					const bool different_xy = out_index % dev_info.num_blocks_adjacent_to_channel;
					const auto yval = static_cast<YID::IDType>(re.getY().getValue() + (different_xy ? -1 : 0));
					return PinGID(
						BlockID(re.getX(), util::make_id<YID>(yval)),
						util::make_id<BlockPinID>(block_pin_id)
					);
				} else {
					const bool same_xy = out_index % dev_info.num_blocks_adjacent_to_channel;
					const auto xval = static_cast<XID::IDType>(re.getX().getValue() + (same_xy ? 0 : -1));
					return PinGID(
						BlockID(util::make_id<XID>(xval), re.getY()),
						util::make_id<BlockPinID>(static_cast<BlockPinID::IDType>(block_pin_id + 1))
					);
				}
			} else {
				const auto out_index2 = out_index - dev_info.pins_per_block_side*dev_info.num_blocks_adjacent_to_channel;
				const auto dest_channel = out_index2/dev_info.track_width;

				// note modulo in the line in the switch block.
				// This has the effecte of swapping the vert/horiz-ness of each fanout
				const auto track_number = out_index2 % dev_info.track_width + (is_horiz ? dev_info.track_width : 0);
				switch (dest_channel) {
					case 0:
						return offset_re_new_index(re,  0,  0, (track_number + dev_info.track_width) % (dev_info.track_width*2), is_horiz);
					case 1:
						return offset_re_new_index(re,  0,  1, (track_number                       ) % (dev_info.track_width*2), is_horiz);
					case 2:
						return offset_re_new_index(re,  1,  0, (track_number + dev_info.track_width) % (dev_info.track_width*2), is_horiz);
					case 3:
						return offset_re_new_index(re,  1, -1, (track_number + dev_info.track_width) % (dev_info.track_width*2), is_horiz);
					case 4:
						return offset_re_new_index(re,  0, -1, (track_number                       ) % (dev_info.track_width*2), is_horiz);
					case 5:
						return offset_re_new_index(re,  0, -1, (track_number + dev_info.track_width) % (dev_info.track_width*2), is_horiz);
					default:
						return RouteElementID();
				}
			}
		}
	}
};

}

#endif // DEVICE__CONNECTORS_H
