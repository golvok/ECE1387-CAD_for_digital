#ifndef DATASTRUCTURES__CNF_EXPRESSION_HPP
#define DATASTRUCTURES__CNF_EXPRESSION_HPP

#include <datastructures/literal_id.hpp>

#include <algorithm>
#include <unordered_set>
#include <vector>

enum class VariableOrder {
	FILE,
	GROUPED_BY_DISJUNCTION,
	MOST_COMMON_FIRST,
};

struct CNFExpression {
	CNFExpression(const std::vector<VariableOrder>& ordering, const std::vector<std::vector<int>>& data);
	CNFExpression(const CNFExpression&) = default;
	CNFExpression(CNFExpression&&) = default;
	CNFExpression& operator=(const CNFExpression&) = default;
	CNFExpression& operator=(CNFExpression&&) = default;

	auto& all_literals() const { return m_all_literals; }
	auto& all_literals()       { return m_all_literals; }

	int max_literal() const;

	auto& all_disjunctions() const { return m_disjunctions; }
	auto& all_disjunctions()       { return m_disjunctions; }

	using DisjunctionID = int;
private:
	std::vector<std::vector<Literal>> m_disjunctions;
	std::vector<LiteralID> m_all_literals;
};

#endif /* DATASTRUCTURES__CNF_EXPRESSION_HPP */
