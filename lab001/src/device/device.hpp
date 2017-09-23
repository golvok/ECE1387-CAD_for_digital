
#include <util/id.hpp>
#include <util/print_printable.hpp>

#include <cstdint>
#include <iosfwd>
#include <unordered_map>
#include <unordered_set>

namespace util {

struct XIDTag { static const std::int16_t DEFAULT_VALUE = 0x1000; };
struct YIDTag { static const std::int16_t DEFAULT_VALUE = 0x1000; };
struct BlockPinIDTag { static const std::int16_t DEFAULT_VALUE = 0x1000; };
using XID = ID<std::int16_t, XIDTag>;
using YID = ID<std::int16_t, YIDTag>;
using BlockPinID = ID<std::int16_t, BlockPinIDTag>;

using XYIDPair = std::pair<XID,YID>;

struct BlockIDTag { static const std::uint32_t DEFAULT_VALUE = 0x10001000; };
class BlockID : public ID <std::uint32_t, BlockIDTag>, public print_printable {
public:
	using ID::IDType;
private:
	static IDType makeValueFromXY(XID x, YID y) { return (static_cast<IDType>(x.getValue()) << 16) | static_cast<IDType>(y.getValue()); }
	static XYIDPair xyFromValue(IDType id)      { return {xFromValue(id), yFromValue(id)}; }
	static XID xFromValue(IDType id)            { return make_id<XID>(static_cast<XID::IDType>(id >> 16)); }
	static YID yFromValue(IDType id)            { return make_id<YID>(static_cast<YID::IDType>(id & 0x0000FFFF)); }

public:
	BlockID(XID x, YID y) : ID(makeValueFromXY(x,y)) { }
	using ID::ID;

	auto getX() const { return xFromValue(getValue()); }
	auto getY() const { return yFromValue(getValue()); }

	void print(std::ostream& os) const {
		os << "{x" << getX().getValue() << ",y" << getY().getValue() << '}';
	}
};

struct PinGIDTag { static const std::uint64_t DEFAULT_VALUE = 0x1000000010001000; };
class PinGID : public ID<std::uint64_t, PinGIDTag>, public print_printable {
public:
	using ID::IDType;

	PinGID(BlockID b, BlockPinID bp) : ID(makeValueFromBlockAndPin(b,bp)) { }
	using ID::ID;
private:
	static IDType makeValueFromBlockAndPin(BlockID b, BlockPinID bp)      { return (static_cast<IDType>(b.getValue()) << 16) | static_cast<IDType>(bp.getValue()); }
	static std::pair<BlockID, BlockPinID> blockAndPinFromValue(IDType id) { return {blockFromValue(id), blockPinFromValue(id)}; }
	static BlockID blockFromValue(IDType id)                              { return make_id<BlockID   >(static_cast<BlockID   ::IDType>(id >> 16)); }
	static BlockPinID blockPinFromValue(IDType id)                        { return make_id<BlockPinID>(static_cast<BlockPinID::IDType>(id & 0x0000FFFF)); }
public:

	auto getBlock()    const { return blockFromValue(getValue()); }
	auto getBlockPin() const { return blockPinFromValue(getValue()); }

	void print(std::ostream& os) const {
		os << "{x" << getBlock().getX().getValue() << ",y" << getBlock().getY().getValue() << ",p" << getBlockPin().getValue() << '}';
	}
};

} // end namespace util

namespace std {
	template<> struct hash<util::BlockID> : util::MyHash<util::BlockID>::type { };
	template<> struct hash<util::PinGID> : util::MyHash<util::PinGID>::type { };
}
