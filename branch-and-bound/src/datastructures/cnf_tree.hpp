#ifndef DATASTRUCTURES__CNF_TREE_HPP
#define DATASTRUCTURES__CNF_TREE_HPP

#include <util/generator.hpp>

struct CNFTree {
	struct Vertex {
		std::vector<LiteralID>::const_iterator lit_pos;
		bool inverted;

		Literal literal() const {
			return Literal(inverted, *lit_pos);
		}

		// bool operator==(const Vertex& rhs) const {
		// 	return std::forward_as_tuple(lit_pos, inverted, id)
		// 		== std::forward_as_tuple(rhs.lit_pos, rhs.inverted, rhs.id);
		// }
	};


	template<typename LiteralOrder>
	CNFTree(const LiteralOrder& lo)
		: m_literal_order(begin(lo), end(lo))
	{ }

	auto fanout(const Vertex& source) const {
		auto src_lit_pos = source.lit_pos;
		return util::make_generator<int> (
			isValid(source) ? 0 : 2,
			[](const int& index) { return index == 2; },
			[](const int& index) { return index + 1; },
			[src_lit_pos](const int& index) { return Vertex{ std::next(src_lit_pos), index == 0 ? false : true }; }
		);
	}

	bool hasNextSibling(const Vertex& v) const {
		return v.inverted == false;
	}

	Vertex nextSibling(Vertex&& v) const {
		v.inverted = true;
		return v;
	}

	bool hasFanout(const Vertex& v) const {
		return std::next(v.lit_pos) != end(m_literal_order);
	}

	auto roots() const {
		return std::array<Vertex, 2>{{
			{begin(m_literal_order), false},
			{begin(m_literal_order), true},
		}};
	}

	bool isValid(const Vertex& v) const {
		return v.lit_pos != end(m_literal_order);
	}
private:
	std::vector<LiteralID> m_literal_order;
};

namespace std {
	template<> struct hash<CNFTree::Vertex> {
		auto operator()(const CNFTree::Vertex& v) const {
			return std::hash<std::decay_t<decltype(*v.lit_pos)>>()(*v.lit_pos)
				| std::hash<std::decay_t<decltype(v.inverted)>>()(v.inverted)
				// | std::hash<std::decay_t<decltype(v.id)>>()(v.id)
			;
		}
	};
}

#endif /* DATASTRUCTURES__CNF_TREE_HPP */
