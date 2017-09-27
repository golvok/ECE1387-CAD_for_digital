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

	BlockID offset_block(const BlockID& bid, int xdiff, int ydiff, bool swap_xy = false) {
		return BlockID(
			util::make_id<XID>(static_cast<XID::IDType>(bid.getX().getValue() + (swap_xy ? ydiff : xdiff))),
			util::make_id<YID>(static_cast<YID::IDType>(bid.getY().getValue() + (swap_xy ? xdiff : ydiff)))
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

struct FullyConnectedConnector {
	using Index = std::int16_t;
	const Index END_VALUE = std::numeric_limits<Index>::max();

	using BlockIndex = BlockID;

	using BlockFanoutIndex = PinGID;

	DeviceInfo dev_info;
	decltype(dev_info.bounds) wire_bb;

	FullyConnectedConnector(const DeviceInfo& dev_info)
		: dev_info(dev_info)
		, wire_bb(dev_info.bounds.min_point(), dev_info.bounds.max_point() + geom::make_point(1,1))
	{ }
	FullyConnectedConnector(const FullyConnectedConnector&) = default;
	FullyConnectedConnector& operator=(const FullyConnectedConnector&) = default;
	FullyConnectedConnector(FullyConnectedConnector&&) = default;
	FullyConnectedConnector& operator=(FullyConnectedConnector&&) = default;

	Index fanout_begin(const RouteElementID& re) const {
		(void)re;
		return next_fanout(re, -1);
	}

	bool is_end_index(const RouteElementID& re, const Index& index) const {
		(void)re;
		return index == END_VALUE;
	}

	Index next_fanout(const RouteElementID& re, Index index) const {
		(void)re;
		auto next = index;
		while (true) {
			next = static_cast<Index>(next + 1);
			if (re.isPin()) {
				if (next > dev_info.track_width - 1) {
					return END_VALUE;
				}
			} else {
				if (next > dev_info.pins_per_block_side*dev_info.num_blocks_adjacent_to_channel + dev_info.track_width*6 - 1) {
					return END_VALUE;
				}
			}

			const auto result = re_from_index(re, next);
			const auto xy = geom::make_point(
				result.getX().getValue(),
				result.getY().getValue()
			);
			if (result.isPin()) {
				if (dev_info.bounds.intersects(xy)) {
					return next;
				}
			} else {
				// allow top row & right col
				if (wire_bb.intersects(xy)) {
					const auto dir = wire_direction(result);
					if (dir == decltype(dir)::HORIZONTAL && xy.x() != wire_bb.maxx()) {
						return next;
					}
					if (dir == decltype(dir)::VERTICAL && xy.y() != wire_bb.maxy()) {
						return next;
					}
				}
			}
		}
	}

	RouteElementID re_from_index(const RouteElementID& re, const Index out_index) const {
		if (re.isPin()) {
			const auto as_pin = re.asPin();
			const auto pin_side = get_block_pin_side(as_pin);
			const bool output_to_horiz = channel_direction_on_this_side(pin_side) == Direction::HORIZONTAL;
			const auto result_index_value = (output_to_horiz ? dev_info.track_width : 0) + (out_index % dev_info.track_width);
			const auto base_re = RouteElementID(re.getX(), re.getY(), util::no_sign_ext_cast<RouteElementID::REIndex>(result_index_value));
			switch (pin_side) {
				case BlockSide::BOTTOM:
					return offset_re_same_index(base_re, 0, 0);
				case BlockSide::RIGHT:
					return offset_re_same_index(base_re, 1, 0);
				case BlockSide::TOP:
					return offset_re_same_index(base_re, 0, 1);
				case BlockSide::LEFT:
					return offset_re_same_index(base_re, 0, 0);
				default:
					return RouteElementID();
			}
		} else {
			const bool is_horiz = wire_direction(re) == Direction::HORIZONTAL;
			if (out_index < dev_info.pins_per_block_side*dev_info.num_blocks_adjacent_to_channel) {
				const auto block_pin_id = static_cast<BlockPinID::IDType>(
					dev_info.num_blocks_adjacent_to_channel*(out_index % dev_info.num_blocks_adjacent_to_channel) + 1
				);
				if (is_horiz) {
					const bool different_xy = out_index % dev_info.num_blocks_adjacent_to_channel;
					const auto yval = static_cast<YID::IDType>(re.getY().getValue() + (different_xy ? -1 : 0));
					return RouteElementID(PinGID(
						BlockID(re.getX(), util::make_id<YID>(yval)),
						util::make_id<BlockPinID>(block_pin_id)
					));
				} else {
					const bool same_xy = out_index % dev_info.num_blocks_adjacent_to_channel;
					const auto xval = static_cast<XID::IDType>(re.getX().getValue() + (same_xy ? 0 : -1));
					return RouteElementID(PinGID(
						BlockID(util::make_id<XID>(xval), re.getY()),
						util::make_id<BlockPinID>(static_cast<BlockPinID::IDType>(block_pin_id + 1))
					));
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

	static BlockFanoutIndex block_fanout_begin(const BlockID& block) {
		return PinGID(
			block,
			util::make_id<BlockPinID>(static_cast<BlockPinID::IDType>(1))
		);
	}

	bool block_fanout_index_is_end(const BlockID& block, const BlockFanoutIndex& index) const {
		(void)block;
		return index.getBlockPin().getValue() > dev_info.pins_per_block_side * 4;
	}

	static BlockFanoutIndex next_block_fanout(const BlockID& block, const BlockFanoutIndex& index) {
		return PinGID(
			block,
			util::make_id<BlockPinID>(static_cast<BlockPinID::IDType>(index.getBlockPin().getValue() + 1))
		);
	}

	static RouteElementID re_from_block_fanout_index(const BlockID& block, const BlockFanoutIndex& index) {
		(void)block;
		return RouteElementID(index);
	}


	BlockIndex blocks_begin() const {
		return BlockID(
			util::make_id<XID>(static_cast<XID::IDType>(dev_info.bounds.minx())),
			util::make_id<YID>(static_cast<YID::IDType>(dev_info.bounds.miny()))
		);
	}

	bool block_index_is_end(const BlockIndex& curr) const {
		return !dev_info.bounds.intersects(curr.getX().getValue(), curr.getY().getValue());
	}

	BlockIndex next_block(const BlockIndex& curr) const {
		auto next = offset_block(curr, 1, 0);
		if (next.getX().getValue() > dev_info.bounds.maxx()) {
			auto in_row = offset_block(curr, 0, 1);
			return BlockID(util::make_id<XID>(static_cast<XID::IDType>(0)), in_row.getY());
		} else {
			return next;
		}
	}

	static BlockID block_from_block_index(const BlockIndex& curr) {
		return curr;
	}

	static BlockSide get_block_pin_side(PinGID pin) {
		switch ((pin.getBlockPin().getValue() - 1) % 4 + 1) {
			case 1:
				return BlockSide::BOTTOM;
			case 2:
				return BlockSide::RIGHT;
			case 3:
				return BlockSide::TOP;
			case 4:
				return BlockSide::LEFT;
			default:
				return BlockSide::OTHER;
		}
	}

	static Direction channel_direction_on_this_side(BlockSide side) {
		switch (side) {
			case BlockSide::BOTTOM:
			case BlockSide::TOP:
				return Direction::HORIZONTAL;
			case BlockSide::RIGHT:
			case BlockSide::LEFT:
				return Direction::VERTICAL;
			default:
				return Direction::OTHER;
		}
	}

	Direction wire_direction(RouteElementID reid) const {
		switch (reid.getIndex()/dev_info.track_width) {
			case 0:
				return Direction::VERTICAL;
			case 1:
				return Direction::HORIZONTAL;
			default:
				return Direction::OTHER;
		}
	}

	auto index_in_channel(RouteElementID reid) const {
		return reid.getIndex() % dev_info.track_width;
	}

};

}

#endif // DEVICE__CONNECTORS_H
