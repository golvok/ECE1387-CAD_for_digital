#include "cnf_expression.hpp"

#include <util/logging.hpp>
#include <util/utils.hpp>

#include <unordered_map>

template<typename T>
void f(const std::vector<std::vector<int>>& data, T m_disjunctions, T m_all_literals) {
	std::unordered_set<LiteralID> liteals_seen;
	for (const auto& disjunction : data) {
		if (disjunction.empty()) { continue; }
		m_disjunctions.emplace_back();
		std::transform(begin(disjunction), end(disjunction), std::back_inserter(m_disjunctions.back()), [&](const auto& datum) {
			auto literal_id = util::make_id<LiteralID>(static_cast<LiteralID::IDType>(abs(datum)));
			if (liteals_seen.insert(literal_id).second) {
				m_all_literals.push_back(literal_id);
			}
			return Literal(datum < 0, literal_id);
		});
	}
}


CNFExpression::CNFExpression(VariableOrder ordering, const std::vector<std::vector<int>>& data)
	: m_disjunctions()
	, m_all_literals()
{
	for (const auto& disjunction : data) {
		if (disjunction.empty()) { continue; }
		m_disjunctions.emplace_back();
		std::transform(begin(disjunction), end(disjunction), std::back_inserter(m_disjunctions.back()), [&](const auto& datum) {
			auto literal_id = util::make_id<LiteralID>(static_cast<LiteralID::IDType>(abs(datum)));
			return Literal(datum < 0, literal_id);
		});
	}

	std::unordered_map<Literal, int> literal_counts;
	for (const auto& disjunction : all_disjunctions()) {
		for (const auto& lit : disjunction) {
			literal_counts[lit] += 1;
		}
	}

	std::unordered_set<LiteralID> literals_ids_seen;
	for (const auto& disjunction : all_disjunctions()) {
		for (const auto& lit : disjunction) {
			if (literals_ids_seen.insert(lit.id()).second) {
				m_all_literals.push_back(lit.id());
			}
		}
	}

	switch (ordering) {
		case VariableOrder::FILE: {
			// leave as-is
		break; } case VariableOrder::MOST_COMMON_FIRST: {
			std::sort(begin(m_all_literals), end(m_all_literals), [&](auto& lhs, auto& rhs) {
				return std::max(literal_counts.at(Literal(true,lhs)), literal_counts.at(Literal(false,lhs)))
					> std::max(literal_counts.at(Literal(true,rhs)), literal_counts.at(Literal(false,rhs)));
			});
		break; }
	}
}
