#ifndef DEVICE__DEVICE_H
#define DEVICE__DEVICE_H

#include <util/id.hpp>
#include <util/generator.hpp>
#include <util/print_printable.hpp>

#include <cstdint>
#include <iosfwd>

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
	static IDType makeValueFromXY(XID x, YID y) { return (static_cast<IDType>(x.getValue()) << 16) | static_cast<IDType>(y.getValue()); }
	static XYIDPair xyFromValue(IDType id)      { return {xFromValue(id), yFromValue(id)}; }
	static XID xFromValue(IDType id)            { return util::make_id<XID>(static_cast<XID::IDType>(id >> 16)); }
	static YID yFromValue(IDType id)            { return util::make_id<YID>(static_cast<YID::IDType>(id & 0x0000FFFF)); }

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
	static IDType makeValueFromBlockAndPin(BlockID b, BlockPinID bp)      { return (static_cast<IDType>(b.getValue()) << 16) | static_cast<IDType>(bp.getValue()); }
	static std::pair<BlockID, BlockPinID> blockAndPinFromValue(IDType id) { return {blockFromValue(id), blockPinFromValue(id)}; }
	static BlockID blockFromValue(IDType id)                              { return util::make_id<BlockID   >(static_cast<BlockID   ::IDType>(id >> 16)); }
	static BlockPinID blockPinFromValue(IDType id)                        { return util::make_id<BlockPinID>(static_cast<BlockPinID::IDType>(id & 0x0000FFFF)); }

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

	RouteElementID(PinGID p) : ID(makeValueFromPin(p)) { }
	RouteElementID(XID x, YID y, std::int16_t i) : ID(makeValueFromXYIndex(x,y,i)) { }

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
	std::int16_t getIndex() const { return indexFromValue(getValue()); }

	bool isPin() const { return isValuePin(getValue()); }

private:
	static IDType makeValueFromPin(PinGID p) { return p.getValue() | ID::JUST_HIGH_BIT; }
	static PinGID pinGIDFromValue_unchecked(IDType v) { return util::make_id<PinGID>(v & ~ID::JUST_HIGH_BIT); }
	static IDType makeValueFromXYIndex(XID x, YID y, std::int16_t i) {
		return (static_cast<IDType>(x.getValue()) << 32)
			| (static_cast<IDType>(y.getValue()) << 16)
			| (static_cast<IDType>(i))
		;
	}
	static XID xFromValue(IDType v) { return util::make_id<XID>(static_cast<XID::IDType>((v & 0xFFFF00000000) >> 32)); }
	static YID yFromValue(IDType v) { return util::make_id<YID>(static_cast<YID::IDType>((v & 0x0000FFFF0000) >> 16)); }
	static std::int16_t indexFromValue(IDType v) { return static_cast<std::int16_t>(v & 0x00000000FFFF); }
	static bool isValuePin(IDType v) { return (v & ID::JUST_HIGH_BIT) != 0; }

	template<typename>
	friend class Device;
};

template<
	  typename CONNECTOR
>
class Device {
public:
	Device(
		int min_x,
		int max_x,
		int min_y,
		int max_y,
		int track_width,
		CONNECTOR& connector
	)
		: min_x(min_x)
		, max_x(max_x)
		, min_y(min_y)
		, max_y(max_y)
		, track_width(track_width)
		, connector(connector)
	{ }

	auto fanout(RouteElementID src) {
		return util::make_generator<Index>(
			connector.fanout_begin(src),
			[=](auto&& index) { return connector.is_end_index(src, index); },
			[=](auto&& index) { return connector.next_fanout(src, index); },
			[=](auto&& index) { return connector.re_from_index(src, index); }
		);
	}

private:
	using Index = typename CONNECTOR::Index;

	int min_x;
	int max_x;
	int min_y;
	int max_y;
	int track_width;

	CONNECTOR connector;
};

} // end namespace device

namespace std {
	template<typename> struct hash;
	template<> struct hash<device::BlockID> : util::MyHash<device::BlockID>::type { };
	template<> struct hash<device::PinGID> : util::MyHash<device::PinGID>::type { };
}

#endif // DEVICE__DEVICE_H
