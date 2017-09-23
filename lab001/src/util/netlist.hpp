
#include <util/id.hpp>
#include <util/print_printable.hpp>

#include <cstdint>
#include <iosfwd>
#include <unordered_map>
#include <unordered_set>

namespace util {

struct XIDTag { static const std::uint32_t DEFAULT_VALUE = 0x10000000; };
struct YIDTag { static const std::uint32_t DEFAULT_VALUE = 0x10000000; };
using XID = ID<std::int32_t, XIDTag>;
using YID = ID<std::int32_t, YIDTag>;

using XYIDPair = std::pair<XID,YID>;

struct BlockIDTag { static const std::uint64_t DEFAULT_VALUE = 0x1000000010000000; };
class BlockID : public ID <std::uint64_t, BlockIDTag>, public print_printable {
private:
	using ID::IDType;
	static IDType makeValueFromXY(XID x, YID y) { return (static_cast<IDType>(x.getValue()) << 32) | static_cast<IDType>(y.getValue()); }
	static XYIDPair xyFromValue(IDType id)      { return {xFromValue(id), yFromValue(id)}; }
	static XID xFromValue(IDType id)            { return make_id<XID>(static_cast<XID::IDType>(id >> 32)); }
	static YID yFromValue(IDType id)            { return make_id<YID>(static_cast<YID::IDType>(id & 0x0000FFFF)); }
public:
	BlockID(XID x, YID y) : ID(makeValueFromXY(x,y)) { }
	BlockID() : ID() { }

	void print(std::ostream& os) const {
		os << "{x" << xFromValue(getValue()).getValue() << ",y" << yFromValue(getValue()).getValue() << '}';
	}
};

template<typename NODE_ID>
class Netlist {
public:
	Netlist()
		: connections()
	{ }

	void add_connection(const NODE_ID& source, const NODE_ID& sink) {
		connections[source].emplace(sink);
	}

	auto begin() const { return std::begin(connections); }
	auto end()   const { return   std::end(connections); }
private:
	std::unordered_map<NODE_ID,std::unordered_set<NODE_ID>> connections;
};

} // end namespace util

namespace std {
	template<>
	struct hash<util::BlockID> : util::MyHash<util::BlockID>::type { };
}
