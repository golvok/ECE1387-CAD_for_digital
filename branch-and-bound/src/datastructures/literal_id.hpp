#ifndef DATASTRUTURES__LITERAL_ID_HPP
#define DATASTRUTURES__LITERAL_ID_HPP

#include <util/id.hpp>
#include <util/bit_tools.hpp>
#include <util/print_printable.hpp>

struct LiteralIDTag { static const std::uint16_t DEFAULT_VALUE = 0xFFFF; };
using LiteralID = util::ID<std::int16_t, LiteralIDTag>;

inline bool operator<(const LiteralID& lhs, const LiteralID& rhs) {
	return lhs.getValue() < rhs.getValue();
}

struct LiteralTag { static const std::uint16_t DEFAULT_VALUE = 0xFFFF; };
struct Literal : public util::ID <std::uint16_t, LiteralTag>, public util::print_printable {
	template<typename T>
	explicit Literal(T&& t) : util::ID <std::uint16_t, LiteralTag>(std::forward<T>(t)) { } // ECF compiler seems to need this???
	explicit Literal() : util::ID <std::uint16_t, LiteralTag>() { } // ECF compiler seems to need this???
	using ID::IDType;

	Literal(bool inverted, LiteralID id)
		: ID(makeValue(inverted, id))
	{ }

	LiteralID id() const { return util::make_id<LiteralID>(IDType(~JUST_HIGH_BIT & getValue())); }
	bool inverted() const { return JUST_HIGH_BIT & getValue(); }

	template<typename STREAM>
	void print(STREAM& os) const {
		os << (inverted() ? '!' :  '+') << id().getValue();
	}

private:
	using ID::ID;
	static IDType makeValue(bool inverted, LiteralID id) { return IDType(id.getValue() | ((IDType)inverted << (BIT_SIZE-1))); }
};

namespace std {
	template<typename> struct hash;
	template<> struct hash<Literal> : util::MyHash<Literal>::type { };
	template<> struct hash<LiteralID> : util::MyHash<LiteralID>::type { };
}

#endif /* DATASTRUTURES__LITERAL_ID_HPP */
