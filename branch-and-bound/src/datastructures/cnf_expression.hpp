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

private:
	std::vector<std::vector<Literal>> m_disjunctions;
	std::unordered_set<LiteralID> m_all_literals;
};

#endif /* DATASTRUCTURES__CNF_EXPRESSION_HPP */
