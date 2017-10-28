#ifndef DEVICE__COMMON_IDS_H
#define DEVICE__COMMON_IDS_H

#include <util/id.hpp>

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

}

namespace std {
	template<typename> struct hash;
	template<> struct hash<device::AtomID> : util::MyHash<device::AtomID>::type { };
}

#endif // DEVICE__COMMON_IDS_H