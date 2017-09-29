#ifndef DEVICE__DEVICE_H
#define DEVICE__DEVICE_H

#include <graphics/geometry.hpp>
#include <util/bit_tools.hpp>
#include <util/id.hpp>
#include <util/generator.hpp>
#include <util/print_printable.hpp>

#include <cstdint>
#include <ostream>

#include <boost/operators.hpp>

namespace device {

struct XIDTag { static const std::uint16_t DEFAULT_VALUE = 0x8000; };
struct YIDTag { static const std::uint16_t DEFAULT_VALUE = 0x8000; };
struct BlockPinIDTag { static const std::uint16_t DEFAULT_VALUE = 0x8000; };
using XID = util::ID<std::int16_t, XIDTag>;
using YID = util::ID<std::int16_t, YIDTag>;
using BlockPinID = util::ID<std::int16_t, BlockPinIDTag>;

using XYIDPair = std::pair<XID,YID>;

struct BlockIDTag { static const std::uint32_t DEFAULT_VALUE = 0x80008000; };
class BlockID : public util::ID <std::uint32_t, BlockIDTag>, public util::print_printable {
public:
	using ID::IDType;
private:
	static IDType makeValueFromXY(XID x, YID y) { return (util::no_sign_ext_cast<IDType>(x.getValue()) << 16) | util::no_sign_ext_cast<IDType>(y.getValue()); }
	static XYIDPair xyFromValue(IDType id)      { return {xFromValue(id), yFromValue(id)}; }
	static XID xFromValue(IDType id)            { return util::make_id<XID>(util::no_sign_ext_cast<XID::IDType>(id >> 16)); }
	static YID yFromValue(IDType id)            { return util::make_id<YID>(util::no_sign_ext_cast<YID::IDType>(id & 0x0000FFFF)); }

public:
	BlockID(XID x, YID y) : ID(makeValueFromXY(x,y)) { }
	using ID::ID;

	auto getX() const { return xFromValue(getValue()); }
	auto getY() const { return yFromValue(getValue()); }

	template<typename STREAM>
	void print(STREAM& os) const {
		os << "{x" << getX().getValue() << ",y" << getY().getValue() << '}';
	}
};

struct PinGIDTag { static const std::uint64_t DEFAULT_VALUE = 0x8000800080008000; };
class PinGID : public util::ID<std::uint64_t, PinGIDTag>, public util::print_printable {
public:
	using ID::IDType;

	PinGID(BlockID b, BlockPinID bp) : ID(makeValueFromBlockAndPin(b,bp)) { }
	using ID::ID;
private:
	static IDType makeValueFromBlockAndPin(BlockID b, BlockPinID bp)      { return (util::no_sign_ext_cast<IDType>(b.getValue()) << 16) | util::no_sign_ext_cast<IDType>(bp.getValue()); }
	static std::pair<BlockID, BlockPinID> blockAndPinFromValue(IDType id) { return {blockFromValue(id), blockPinFromValue(id)}; }
	static BlockID blockFromValue(IDType id)                              { return util::make_id<BlockID   >(util::no_sign_ext_cast<BlockID   ::IDType>(id >> 16)); }
	static BlockPinID blockPinFromValue(IDType id)                        { return util::make_id<BlockPinID>(util::no_sign_ext_cast<BlockPinID::IDType>(id & 0x0000FFFF)); }

public:
	auto getBlock()    const { return blockFromValue(getValue()); }
	auto getBlockPin() const { return blockPinFromValue(getValue()); }

	template<typename STREAM>
	void print(STREAM& os) const {
		os << "{x" << getBlock().getX().getValue() << ",y" << getBlock().getY().getValue() << ",p" << getBlockPin().getValue() << '}';
	}
};



struct RouteElementIDTag { static const std::uint64_t DEFAULT_VALUE = 0x8000800080008000; };
class RouteElementID : public util::ID<std::uint64_t, RouteElementIDTag>, public util::print_printable {
public:
	using ID::IDType;
	using REIndex = std::int16_t;

	using ID::ID;
	explicit RouteElementID(PinGID p) : ID(makeValueFromPin(p)) { }
	RouteElementID(XID x, YID y, REIndex i) : ID(makeValueFromXYIndex(x,y,i)) { }

	template<typename STREAM>
	void print(STREAM& os) const {
		if (isValuePin(getValue())) {
			pinGIDFromValue_unchecked(getValue()).print(os);
		} else {
			os << "{x" << getX().getValue() << ",y" << getY().getValue() << ",i" << getIndex() << '}';
		}
	}

	XID getX() const { return xFromValue(getValue()); }
	YID getY() const { return yFromValue(getValue()); }
	REIndex getIndex() const { return indexFromValue(getValue()); }

	bool isPin() const { return isValuePin(getValue()); }
	PinGID asPin() const { return pinGIDFromValue_unchecked(getValue()); }

private:
	static IDType makeValueFromPin(PinGID p) { return p.getValue() | ID::JUST_HIGH_BIT; }
	static PinGID pinGIDFromValue_unchecked(IDType v) { return util::make_id<PinGID>(v & ~ID::JUST_HIGH_BIT); }
	static IDType makeValueFromXYIndex(XID x, YID y, REIndex i) {
		return (util::no_sign_ext_cast<IDType>(x.getValue()) << 32)
			| (util::no_sign_ext_cast<IDType>(y.getValue()) << 16)
			| (util::no_sign_ext_cast<IDType>(i))
		;
	}
	static XID xFromValue(IDType v) { return util::make_id<XID>(util::no_sign_ext_cast<XID::IDType>((v & 0xFFFF00000000) >> 32)); }
	static YID yFromValue(IDType v) { return util::make_id<YID>(util::no_sign_ext_cast<YID::IDType>((v & 0x0000FFFF0000) >> 16)); }
	static REIndex indexFromValue(IDType v) { return util::no_sign_ext_cast<REIndex>(v & 0x00000000FFFF); }
	static bool isValuePin(IDType v) { return (v & ID::JUST_HIGH_BIT) != 0; }

	template<typename>
	friend class Device;
};

enum class BlockSide {
	OTHER, LEFT, RIGHT, TOP, BOTTOM,
};

inline BlockSide oppositeSide(BlockSide bs) {
	switch (bs) {
		default:
			return BlockSide::OTHER;
		case BlockSide::LEFT:
			return BlockSide::RIGHT;
		case BlockSide::RIGHT:
			return BlockSide::LEFT;
		case BlockSide::TOP:
			return BlockSide::BOTTOM;
		case BlockSide::BOTTOM:
			return BlockSide::TOP;
	}
}

inline std::ostream& operator<<(std::ostream& os, BlockSide bs) {
	switch (bs) {
		case BlockSide::OTHER:
			os << "OTHER";
		break;
		case BlockSide::LEFT:
			os << "LEFT";
		break;
		case BlockSide::RIGHT:
			os << "RIGHT";
		break;
		case BlockSide::TOP:
			os << "TOP";
		break;
		case BlockSide::BOTTOM:
			os << "BOTTOM";
		break;
	}
	return os;
}

enum class Direction {
	OTHER, HORIZONTAL, VERTICAL,
};

inline std::ostream& operator<<(std::ostream& os, Direction dir) {
	switch (dir) {
		case Direction::OTHER:
			os << "OTHER";
		break;
		case Direction::HORIZONTAL:
			os << "HORIZONTAL";
		break;
		case Direction::VERTICAL:
			os << "VERTICAL";
		break;
	}
	return os;
}

struct DeviceInfo {
	geom::BoundBox<int> bounds{};
	int track_width = -1;
	int pins_per_block_side = -1;
	int num_blocks_adjacent_to_channel = -1;
};

template<
	  typename CONNECTOR
>
class Device {
public:
	Device(
		const DeviceInfo& dev_info,
		CONNECTOR& connector
	)
		: dev_info(dev_info)
		, connector(connector)
	{ }

	const auto& info() const {
		return dev_info;
	}

	auto fanout(RouteElementID src) const {
		return util::make_generator<typename CONNECTOR::Index>(
			connector.fanout_begin(src),
			[=](auto&& index) { return connector.is_end_index(src, index); },
			[=](auto&& index) { return connector.next_fanout(src, index); },
			[=](auto&& index) { return connector.re_from_index(src, index); }
		);
	}

	auto fanout(BlockID block) const {
		return util::make_generator<typename CONNECTOR::BlockFanoutIndex>(
			connector.block_fanout_begin(block),
			[=](auto&& index) { return connector.block_fanout_index_is_end(block, index); },
			[=](auto&& index) { return connector.next_block_fanout(block, index); },
			[=](auto&& index) { return connector.re_from_block_fanout_index(block, index); }
		);
	}

	auto blocks() const {
		return util::make_generator<typename CONNECTOR::BlockIndex>(
			connector.blocks_begin(),
			[=](auto&& index) { return connector.block_index_is_end(index); },
			[=](auto&& index) { return connector.next_block(index); },
			[=](auto&& index) { return connector.block_from_block_index(index); }
		);
	}

	BlockSide get_block_pin_side(PinGID pin) const {
		return connector.get_block_pin_side(pin);
	}

	Direction channel_direction_on_this_side(PinGID pin) const {
		return connector.channel_direction_on_this_side(pin);
	}

	Direction wire_direction(RouteElementID reid) const {
		return connector.wire_direction(reid);
	}

	auto index_in_channel(RouteElementID reid) const {
		return connector.index_in_channel(reid);
	}

private:

	DeviceInfo dev_info;

	CONNECTOR connector;
};

} // end namespace device

namespace std {
	template<typename> struct hash;
	template<> struct hash<device::BlockID> : util::MyHash<device::BlockID>::type { };
	template<> struct hash<device::PinGID> : util::MyHash<device::PinGID>::type { };
	template<> struct hash<device::RouteElementID> : util::MyHash<device::RouteElementID>::type { };
}

#endif // DEVICE__DEVICE_H
