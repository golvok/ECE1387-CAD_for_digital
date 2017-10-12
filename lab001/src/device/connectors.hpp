#ifndef DEVICE__CONNECTORS_H
#define DEVICE__CONNECTORS_H

#include <device/device.hpp>
#include <parsing/input_parser.hpp>
#include <util/graph_algorithms.hpp>
#include <util/logging.hpp>
#include <util/netlist.hpp>
#include <util/template_utils.hpp>

#include <memory>
#include <mutex>
#include <shared_mutex>

#include <boost/optional.hpp>
#include <boost/thread/shared_mutex.hpp>

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

	const DeviceInfo dev_info;
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
		return next_fanout(re, -1);
	}

	bool is_end_index(const RouteElementID& re, const Index& index) const {
		(void)re;
		return index == END_VALUE;
	}

	Index next_fanout(const RouteElementID& re, Index index) const {
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
				const auto track_number = index_in_channel(out_index2) + (is_horiz ? dev_info.track_width : 0);
				switch (dest_channel) {
					case 0:
						return offset_re_new_index(re,  0,  0, (track_number + dev_info.track_width) % (dev_info.track_width*2), is_horiz);
					case 1:
						return offset_re_new_index(re,  0, -1, (track_number                       ) % (dev_info.track_width*2), is_horiz);
					case 2:
						return offset_re_new_index(re, -1,  0, (track_number + dev_info.track_width) % (dev_info.track_width*2), is_horiz);
					case 3:
						return offset_re_new_index(re, -1,  1, (track_number + dev_info.track_width) % (dev_info.track_width*2), is_horiz);
					case 4:
						return offset_re_new_index(re,  0,  1, (track_number                       ) % (dev_info.track_width*2), is_horiz);
					case 5:
						return offset_re_new_index(re,  0,  1, (track_number + dev_info.track_width) % (dev_info.track_width*2), is_horiz);
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

	int index_in_channel(int index) const {
		return index % dev_info.track_width;
	}

	auto index_in_channel(RouteElementID reid) const {
		return index_in_channel(reid.getIndex());
	}

};

class WiltonConnector : public FullyConnectedConnector {
public:
	using FullyConnectedConnector::FullyConnectedConnector;

	Index next_fanout(const RouteElementID& re, Index index) const {
		auto next = index;
		while (true) {
			next = FullyConnectedConnector::next_fanout(re, next);
			if (next == END_VALUE) {
				return END_VALUE;
			}
			const auto& result = re_from_index(re, next);
			if (re.isPin()) {
				return next;
			} else if (result.isPin()) {
				return next;
			} else {
				const auto sides = sides_of_common_switchbox({re, result});
				if (
					fanout_allowed_wilton({index_in_channel(    re.getIndex()), sides.first},  {index_in_channel(result.getIndex()), sides.second}) ||
					fanout_allowed_wilton({index_in_channel(result.getIndex()), sides.second}, {index_in_channel(    re.getIndex()), sides.first})
				) {
					return next;
				}
			}
		}
	}

	std::pair<BlockSide,BlockSide> sides_of_common_switchbox(std::pair<const RouteElementID&, const RouteElementID&> reids) const {
		const auto dirs = std::make_pair(wire_direction(reids.first), wire_direction(reids.second));
		const auto locs = std::make_pair(
			geom::make_point(reids.first.getX().getValue(),  reids.first.getY().getValue()),
			geom::make_point(reids.second.getX().getValue(), reids.second.getY().getValue())
		);

		return sides_of_common_switchbox(dirs, locs);
	}

	template<typename Dirs, typename Locs>
	static std::pair<BlockSide,BlockSide> sides_of_common_switchbox(Dirs dirs, Locs locs) {

		if (dirs.first == Direction::OTHER || dirs.second == Direction::OTHER) {
			return {BlockSide::OTHER, BlockSide::OTHER};
		} else if (dirs.first == dirs.second) {
			// if vert, smaller y has bottom, other has top
			// if horiz, smaller x has left, other has right
			if (dirs.first == Direction::VERTICAL) {
				if (locs.first.y() < locs.second.y()) {
					return {BlockSide::BOTTOM, BlockSide::TOP};
				} else {
					return {BlockSide::TOP, BlockSide::BOTTOM};
				}
			} else if (dirs.first == Direction::HORIZONTAL) {
				if (locs.first.x() < locs.second.x()) {
					return {BlockSide::LEFT, BlockSide::RIGHT};
				} else {
					return {BlockSide::RIGHT, BlockSide::LEFT};
				}
			} else {
				return {BlockSide::OTHER, BlockSide::OTHER};
			}
		} else {
			// if xy same, vert gets top & horiz gets right
			// else if (abs of x and y diffs \leq 1 && have same sign) vert gets bottom & horiz gets left
			// else error

			// -- or --

			// if vert above or at horiz, vert -> top
			// 	else vert -> bottom
			// if vert to the left or at horiz, horiz -> right
			// 	else horiz -> left
			if (dirs.first == Direction::VERTICAL) {
				const auto delta_to_vert = locs.first - locs.second;
				return std::make_pair(
					delta_to_vert.y() >= 0 ? BlockSide::TOP   : BlockSide::BOTTOM,
					delta_to_vert.x() <= 0 ? BlockSide::RIGHT : BlockSide::LEFT
				);
			} else {
				const auto delta_to_vert = locs.second - locs.first;
				return std::make_pair(
					delta_to_vert.x() <= 0 ? BlockSide::RIGHT : BlockSide::LEFT,
					delta_to_vert.y() >= 0 ? BlockSide::TOP   : BlockSide::BOTTOM
				);
			}
		}
	}

	bool fanout_allowed_wilton(std::pair<int, BlockSide> source, std::pair<int, BlockSide> sink) const {
		if (oppositeSide(source.second) == sink.second && source.first == sink.first) {
			return true; // opposite side, same channel => connect
		}

		const auto w = dev_info.track_width;
		switch (source.second) {
			case BlockSide::LEFT:
				return sink.second == BlockSide::TOP    && (w - source.first) % w == sink.first;
			case BlockSide::TOP:
				return sink.second == BlockSide::RIGHT  && (source.first + 1) % w == sink.first;
			case BlockSide::RIGHT:
				return sink.second == BlockSide::BOTTOM && (2*w - 2 - source.first) % w == sink.first;
			case BlockSide::BOTTOM:
				return sink.second == BlockSide::LEFT   && (source.first + 1) % w == sink.first;
			default:
				return true;
		}
	}
};

template<typename BaseConnector>
class FanoutCachingConnector : public BaseConnector {
	mutable std::unordered_map<RouteElementID, std::unique_ptr<std::vector<RouteElementID>>> cache = {};
	mutable std::unique_ptr<boost::shared_mutex> mutex = std::make_unique<boost::shared_mutex>();
public:
	using BaseConnector::BaseConnector;

	auto fanout_begin(const RouteElementID& re) const {
		using std::begin;
		return begin(get_fanout(re));
	}

	template<typename Index>
	bool is_end_index(const RouteElementID& re, const Index& index) const {
		using std::end;
		return index == end(get_fanout(re));
	}

	template<typename Index>
	auto next_fanout(const RouteElementID& re, const Index& index) const {
		(void)re;
		return std::next(index);
	}

	template<typename Index>
	auto re_from_index(const RouteElementID& re, const Index& out_index) const {
		(void)re;
		return *out_index;
	}

	const auto& get_fanout(const RouteElementID& re) const {
		mutex->lock_shared();
		const auto find_result = cache.find(re);
		if (find_result == end(cache)) {
			mutex->unlock_shared();
			std::lock_guard<decltype(*mutex)> lock_holder(*mutex);
			const auto find_result_with_write_perms = cache.find(re);
			if (find_result == end(cache)) {
				return *cache.emplace(re, std::make_unique<std::vector<RouteElementID>>(compute_all_fanout(re))).first->second;
			} else {
				return *find_result_with_write_perms->second;
			}
		} else {
			const auto& result = *find_result->second;
			mutex->unlock_shared();
			return result;
		}
	}

	auto compute_all_fanout(const RouteElementID& re) const {
		std::vector<RouteElementID> result;
		for (
			auto it = BaseConnector::fanout_begin(re);
			!BaseConnector::is_end_index(re, it);
			it = BaseConnector::next_fanout(re, it)
		) {
			result.emplace_back(BaseConnector::re_from_index(re, it));
		}
		return result;
	}
};

template<typename BaseConnector>
class FanoutPreCachingConnector : public BaseConnector {
	using CacheElement = std::vector<RouteElementID>;
	std::unordered_map<RouteElementID, CacheElement> cache;
public:
	FanoutPreCachingConnector(const DeviceInfo& dev_info)
		: BaseConnector(dev_info)
		, cache(make_cache(dev_info))
	{ }
	FanoutPreCachingConnector(const FanoutPreCachingConnector&) = default;
	FanoutPreCachingConnector& operator=(const FanoutPreCachingConnector&) = default;
	FanoutPreCachingConnector(FanoutPreCachingConnector&&) = default;
	FanoutPreCachingConnector& operator=(FanoutPreCachingConnector&&) = default;

	struct Index {
		CacheElement::const_iterator curr;
		const CacheElement* cache_element;

		bool operator==(const Index& rhs) const {
			return std::forward_as_tuple(curr, cache_element) == std::forward_as_tuple(rhs.curr, rhs.cache_element);
		}
	};

	Index fanout_begin(const RouteElementID& re) const {
		using std::begin;
		const auto& cache_element = get_fanout(re);
		return { begin(cache_element), &cache_element };
	}

	bool is_end_index(const RouteElementID& re, const Index& index) const {
		(void)re;
		using std::end;
		return index.curr == end(*index.cache_element);
	}

	Index next_fanout(const RouteElementID& re, const Index& index) const {
		(void)re;
		return { std::next(index.curr), index.cache_element };
	}

	auto re_from_index(const RouteElementID& re, const Index& out_index) const {
		(void)re;
		return *out_index.curr;
	}

	const auto& get_fanout(const RouteElementID& re) const {
		const auto find_result = cache.find(re);
		if (find_result == end(cache)) {
			throw std::runtime_error("don't have cached connections for a route element");
		} else {
			return find_result->second;
		}
	}

	auto compute_all_fanout(const RouteElementID& re) const {
		std::vector<RouteElementID> result;
		for (
			auto it = BaseConnector::fanout_begin(re);
			!BaseConnector::is_end_index(re, it);
			it = BaseConnector::next_fanout(re, it)
		) {
			result.emplace_back(BaseConnector::re_from_index(re, it));
		}
		return result;
	}

	static auto make_cache(const DeviceInfo& dev_info) {
		Device<BaseConnector> device(dev_info);

		std::vector<RouteElementID> initial_list;
		for (const auto& block : device.blocks()) {
			for (const auto& pin : device.fanout(block)) {
				initial_list.push_back(pin);
			}
		}

		decltype(FanoutPreCachingConnector::cache) result;
		util::GraphAlgo<RouteElementID>().breadthFirstVisit(device, initial_list, [&](auto& re) {
			for (const auto& fanout : device.fanout(re)) {
				result[re].emplace_back(fanout);
			}
		});

		return result;
	}
};

#define ALL_DEVICES_COMMA_SEP \
	device::Device<device::FanoutPreCachingConnector<device::WiltonConnector>>, \
	device::Device<device::FanoutPreCachingConnector<device::FullyConnectedConnector>>, \
	\
	device::Device<device::FanoutCachingConnector<device::WiltonConnector>>, \
	device::Device<device::FanoutCachingConnector<device::FullyConnectedConnector>>, \
	\
	device::Device<device::WiltonConnector>, \
	device::Device<device::FullyConnectedConnector>

/* #define ALL_DEVICES_COMMA_SEP_REF \
// 	device::Device<device::FanoutPreCachingConnector<device::WiltonConnector>>&, \
// 	device::Device<device::FanoutPreCachingConnector<device::FullyConnectedConnector>>&, \
// 	\
// 	device::Device<device::FanoutCachingConnector<device::WiltonConnector>>&, \
// 	device::Device<device::FanoutCachingConnector<device::FullyConnectedConnector>>&, \
// 	\
// 	device::Device<device::WiltonConnector>&, \
// 	device::Device<device::FullyConnectedConnector>&
*/

/* #define ALL_DEVICES_COMMA_SEP_CONST_REF \
// 	const device::Device<device::FanoutPreCachingConnector<device::WiltonConnector>>&, \
// 	const device::Device<device::FanoutPreCachingConnector<device::FullyConnectedConnector>>&, \
// 	\
// 	const device::Device<device::FanoutCachingConnector<device::WiltonConnector>>&, \
// 	const device::Device<device::FanoutCachingConnector<device::FullyConnectedConnector>>&, \
// 	\
// 	const device::Device<device::WiltonConnector>&, \
// 	const device::Device<device::FullyConnectedConnector>&
*/

static const util::type_vector<
	ALL_DEVICES_COMMA_SEP
> ALL_DEVICES;

// static const util::type_vector<
// 	ALL_DEVICES_COMMA_SEP_REF
// > ALL_DEVICES_REF;

// static const util::type_vector<
// 	ALL_DEVICES_COMMA_SEP_CONST_REF
// > ALL_DEVICES_CONST_REF;

namespace DeviceType {
	static const DeviceTypeID Wilton = util::make_id<DeviceTypeID>(1);
	static const DeviceTypeID FullyConnected = util::make_id<DeviceTypeID>(2);

	static const DeviceTypeID Wilton_Cached = util::make_id<DeviceTypeID>(3);
	static const DeviceTypeID FullyConnected_Cached = util::make_id<DeviceTypeID>(4);

	static const DeviceTypeID Wilton_PreCached = util::make_id<DeviceTypeID>(5);
	static const DeviceTypeID FullyConnected_PreCached = util::make_id<DeviceTypeID>(6);

	inline boost::optional<DeviceTypeID> parseFromString(const std::string& s) {
		if (s == "wilton") {
			return Wilton;
		} else if (s == "fc" || s == "fully_connected") {
			return FullyConnected;
		} else if (s == "wilton-cached") {
			return Wilton_Cached;
		} else if (s == "fc-cached" || s == "fully_connected-cached") {
			return FullyConnected_Cached;
		} else if (s == "wilton-precached") {
			return Wilton_PreCached;
		} else if (s == "fc-precached" || s == "fully_connected-precached") {
			return FullyConnected_PreCached;
		} else {
			return boost::none;
		}
	}
}

} // end namespace device

#endif // DEVICE__CONNECTORS_H
