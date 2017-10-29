#ifndef DEVICE__COMMON_IDS_H
#define DEVICE__COMMON_IDS_H

#include <util/bit_tools.hpp>
#include <util/id.hpp>
#include <util/print_printable.hpp>

#include <cstdint>
#include <iostream>

namespace device {

struct XIDTag { static const std::uint16_t DEFAULT_VALUE = 0x8000; };
struct YIDTag { static const std::uint16_t DEFAULT_VALUE = 0x8000; };
struct BlockPinIDTag { static const std::uint16_t DEFAULT_VALUE = 0x8000; };
using XID = util::ID<std::int16_t, XIDTag>;
using YID = util::ID<std::int16_t, YIDTag>;
using BlockPinID = util::ID<std::int16_t, BlockPinIDTag>;

using XYIDPair = std::pair<XID,YID>;

struct AtomIDTag { static const std::uint32_t DEFAULT_VALUE = 0x80008000; };
using AtomID = util::ID<std::uint32_t, AtomIDTag>;

struct XValTag { constexpr static const double DEFAULT_VALUE = -123.45; };
struct YValTag { constexpr static const double DEFAULT_VALUE = -123.45; };
using XVal = util::ID<double, XValTag>;
using YVal = util::ID<double, YValTag>;

inline std::ostream& operator<<(std::ostream& os, const AtomID& id) {
	os << "Atom:" << id.getValue();
	return os;
}

struct BlockIDTag { static const std::uint32_t DEFAULT_VALUE = 0x80008000; };
class BlockID : public util::ID <std::uint32_t, BlockIDTag>, public util::print_printable {
	using ID::ID;
public:
	template<typename T>
	explicit BlockID(T&& t) : util::ID <std::uint32_t, BlockIDTag>(std::forward<T>(t)) { } // ECF compiler seems to need this???
	explicit BlockID() : util::ID <std::uint32_t, BlockIDTag>() { } // ECF compiler seems to need this???
	using ID::IDType;
private:
	static IDType makeValueFromXY(XID x, YID y) { return (util::no_sign_ext_cast<IDType>(x.getValue()) << 16) | util::no_sign_ext_cast<IDType>(y.getValue()); }
	static XYIDPair xyFromValue(IDType id)      { return {xFromValue(id), yFromValue(id)}; }
	static XID xFromValue(IDType id)            { return util::make_id<XID>(util::no_sign_ext_cast<XID::IDType>(id >> 16)); }
	static YID yFromValue(IDType id)            { return util::make_id<YID>(util::no_sign_ext_cast<YID::IDType>(id & 0x0000FFFF)); }

public:
	BlockID(XID x, YID y) : ID(makeValueFromXY(x,y)) { }

	auto getX() const { return xFromValue(getValue()); }
	auto getY() const { return yFromValue(getValue()); }

	template<typename STREAM>
	void print(STREAM& os) const {
		os << "{x" << getX().getValue() << ",y" << getY().getValue() << '}';
	}
};

}

namespace std {
	template<typename> struct hash;
	template<> struct hash<device::BlockID> : util::MyHash<device::BlockID>::type { };
	template<> struct hash<device::AtomID> : util::MyHash<device::AtomID>::type { };
}

#endif // DEVICE__COMMON_IDS_H