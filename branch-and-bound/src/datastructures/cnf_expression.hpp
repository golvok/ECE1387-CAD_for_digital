#ifndef DATASTRUCTURES__CNF_EXPRESSION_HPP
#define DATASTRUCTURES__CNF_EXPRESSION_HPP

#include <datastructures/literal_id.hpp>

#include <algorithm>
#include <unordered_set>
#include <vector>

struct CNFExpression {
	CNFExpression(const std::vector<std::vector<int>>& data);
	CNFExpression(const CNFExpression&) = default;
	CNFExpression(CNFExpression&&) = default;
	CNFExpression& operator=(const CNFExpression&) = default;
	CNFExpression& operator=(CNFExpression&&) = default;

	auto& all_literals() const { return m_all_literals; }
	auto& all_literals()       { return m_all_literals; }

	auto& all_disjunctions() const { return m_disjunctions; }
	auto& all_disjunctions()       { return m_disjunctions; }

	using DisjunctionID = int;
private:
	std::vector<std::vector<Literal>> m_disjunctions;
	std::unordered_set<LiteralID> m_all_literals;
};

template<typename T>
auto eval_disjunctions(const CNFExpression& expression, const T& literal_settings) {
	// count all disjunctions that have all terms set, and are false.
	struct Result {
		int true_count = 0;
		int false_count = 0;
		int undecidable_count = 0;
	} result;
	for (const auto& disjunction : expression.all_disjunctions()) {
		bool disjunction_value = false;
		bool disjunction_decidable = true;
		for (const auto& literal : disjunction) {
			const auto& lookup = literal_settings.find(literal.id());
			if (lookup == end(literal_settings)) {
				disjunction_decidable = false;
			} else {
				bool literal_value = literal.inverted() ? !lookup->second : lookup->second;
				disjunction_value = disjunction_value || literal_value;
			}
		}
		if (disjunction_value) {
			result.true_count += 1;
		} else {
			if (disjunction_decidable) {
				result.false_count += 1;
			} else {
				result.undecidable_count += 1;
			}
		}
	}
	return result;
}

#endif /* DATASTRUCTURES__CNF_EXPRESSION_HPP */
