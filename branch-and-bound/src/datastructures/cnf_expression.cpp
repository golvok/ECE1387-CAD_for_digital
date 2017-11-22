#include "cnf_expression.hpp"

#include <util/logging.hpp>

CNFExpression::CNFExpression(const std::vector<std::vector<int>>& data)
	: m_disjunctions()
	, m_all_literals()
{
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
