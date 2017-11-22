#ifndef DATASTRUCTURES__CNF_EVALUATION_HPP
#define DATASTRUCTURES__CNF_EVALUATION_HPP

#include <datastructures/cnf_expression.hpp>
#include <datastructures/literal_id.hpp>

#include <vector>

template<
	template <typename...> class LiteralMap
>
struct CNFEvaluation {
	template<typename... Args> using DisjunctionMap = std::vector<Args...>;
	using DisjunctionID = CNFExpression::DisjunctionID;

	struct Counts {
		int false_count = 0;
		int undecidable_count = 0;
	};

	struct DisjunctionStatus {
		bool decidable() const { return num_unknown <= 0 || num_true > 0; }
		bool valueIfDecidable() const { return num_true > 0; }

		void add(bool value)    { countFor(value) += 1; num_unknown -= 1; }
		void remove(bool value) { countFor(value) -= 1; num_unknown += 1; }

		auto id() const { return m_id; }

		DisjunctionStatus(DisjunctionID id, int num_terms)
			: m_id(id), num_true(0), num_false(0), num_unknown(num_terms)
		{ }
	private:
		int& countFor(bool value) { return value ? num_true : num_false; }

		DisjunctionID m_id;
		int num_true;
		int num_false;
		int num_unknown;
	};

	using DisjunctionsWithLiterals = LiteralMap<std::vector<DisjunctionStatus*>>;

	CNFEvaluation(const CNFExpression& expr)
		: expression(expr)
		, disjunction_status(create_all_unkown_disjunction_status(expr.all_disjunctions()))
		, disjunctions_with_literal(find_disjunctions_with_literals(expr.all_disjunctions(), disjunction_status, expr.all_literals()))
		, current_result{0, (int)expr.all_disjunctions().size()}
	{ }

	const Counts& addSetting(const LiteralID& lit_id, bool value) {
		for (auto* disj_status_ptr : disjunctions_with_literal.at(lit_id.getValue())) {
			const auto& disj = expression.all_disjunctions().at(disj_status_ptr->id());
			const auto& lit = *std::find_if(begin(disj), end(disj), [&](auto& lit) { return lit.id() == lit_id; });
			const bool value_in_disj = lit.inverted() ? !value : value;

			auto& status = *disj_status_ptr;
			const bool decidable_before = status.decidable();
			status.add(value_in_disj);
			if (!decidable_before && status.decidable()) {
				current_result.undecidable_count -= 1;
				if (!status.valueIfDecidable()) {
					current_result.false_count += 1; // now is false, so increment
				}
			}
		}
		return current_result;
	}

	const Counts& removeSetting(const LiteralID& lit_id, bool value) {
		for (auto* disj_status_ptr : disjunctions_with_literal.at(lit_id.getValue())) {
			const auto& disj = expression.all_disjunctions().at(disj_status_ptr->id());
			const auto& lit = *std::find_if(begin(disj), end(disj), [&](auto& lit) { return lit.id() == lit_id; });
			const bool value_in_disj = lit.inverted() ? !value : value;

			auto& status = *disj_status_ptr;
			const bool decidable_before = status.decidable();
			const bool value_before_if_decidable = status.valueIfDecidable();
			status.remove(value_in_disj);
			if (decidable_before && !status.decidable()) {
				current_result.undecidable_count += 1;
				if (!value_before_if_decidable) {
					current_result.false_count -= 1; // it was false, so decrement
				}
			}
		}
		return current_result;
	}

	const Counts& getCounts() const { return current_result; }

private:
	template<typename DisjunctionList, typename DisjunctionStatusList, typename LiteralList>
	static auto find_disjunctions_with_literals(const DisjunctionList& disj_list, DisjunctionStatusList& disj_status_list, const LiteralList&) {
		DisjunctionsWithLiterals result(disj_list.size());
		for (int idisj = 0; idisj < (int)disj_list.size(); ++idisj) {
			for (const auto& lit : disj_list.at(idisj)) {
				result[lit.id().getValue()].emplace_back(&disj_status_list.at(idisj));
			}
		}
		return result;
	}

	template<typename DisjunctionList>
	static auto create_all_unkown_disjunction_status(const DisjunctionList& disj_list) {
		DisjunctionMap<DisjunctionStatus> result;
		for (int idisj = 0; idisj < (int)disj_list.size(); ++idisj) {
			result.emplace_back(DisjunctionStatus(idisj, (int)disj_list.at(idisj).size()));
		}
		return result;
	}

	CNFExpression expression;
	DisjunctionMap<DisjunctionStatus> disjunction_status;
	DisjunctionsWithLiterals disjunctions_with_literal;
	Counts current_result;
};

#endif /* DATASTRUCTURES__CNF_EVALUATION_HPP */
