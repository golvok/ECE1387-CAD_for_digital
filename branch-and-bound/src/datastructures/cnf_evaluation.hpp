#ifndef DATASTRUCTURES__CNF_EVALUATION_HPP
#define DATASTRUCTURES__CNF_EVALUATION_HPP

#include <datastructures/cnf_expression.hpp>
#include <datastructures/literal_id.hpp>

#include <util/logging.hpp> /// remove me!

#include <vector>

template<typename T>
auto eval_disjunctions(const CNFExpression& expression, const T& literal_settings) {
	// count all disjunctions that have all terms set, and are false.
	struct Result {
		int falseCount() const { return false_count; }
		int undecidableCount() const { return undecidable_count; }

		int true_count = 0;
		int false_count = 0;
		int undecidable_count = 0;
	} result;

	std::vector<Literal> single_unsets;
	std::vector<std::pair<Literal, Literal>> double_unsets;

	for (const auto& disjunction : expression.all_disjunctions()) {
		bool disjunction_value = false;
		int unset_count = 0;
		Literal last_unset(true, util::make_id<LiteralID>((short)-1)); // only to be used if unset_count > 0
		Literal prev_last_unset(true, util::make_id<LiteralID>((short)-1)); // only to be used if unset_count > 0
		for (const auto& literal : disjunction) {
			const auto& lookup = literal_settings.at(literal.id().getValue());
			if (lookup.unset()) {
				unset_count += 1;
				prev_last_unset = last_unset;
				last_unset = literal;
			} else {
				bool literal_value = literal.inverted() ? !lookup.valueIfSet() : lookup.valueIfSet();
				disjunction_value = disjunction_value || literal_value;
			}
		}
		if (disjunction_value) {
			result.true_count += 1;
		} else {
			if (unset_count == 0) {
				result.false_count += 1;
			} else {
				if (unset_count == 1) {
					single_unsets.push_back(last_unset);
				} else if (unset_count == 2) {
					if (prev_last_unset.id().getValue() < last_unset.id().getValue()) {
						double_unsets.emplace_back(prev_last_unset, last_unset);
					} else {
						double_unsets.emplace_back(last_unset, prev_last_unset);
					}
				}

				result.undecidable_count += 1;
			}
		}
	}

	for (const auto& lit_id : expression.all_literals()) {
		int pos_count = 0;
		int neg_count = 0;
		for (const auto& lit : single_unsets) {
			if (lit_id == lit.id()) {
				if (lit.inverted()) {
					neg_count += 1;
				} else {
					pos_count += 1;
				}
			}
		}

		result.false_count += std::min(pos_count, neg_count);
	}

	auto count_index = [&](auto&& lit_pair) {
		if (lit_pair.first.inverted()) {
			if (lit_pair.second.inverted()) {
				return 0;
			} else {
				return 1;
			}
		} else {
			if (lit_pair.second.inverted()) {
				return 2;
			} else {
				return 3;
			}
		}
	};

	if (double_unsets.size() >= 4) {
		for (auto lit_id_it1 = begin(expression.all_literals()); lit_id_it1 != end(expression.all_literals()); ++lit_id_it1) {
			for (auto lit_id_it2 = std::next(lit_id_it1); lit_id_it2 != end(expression.all_literals()); ++lit_id_it2) {
				std::array<int,4> counts;
				std::fill(begin(counts), end(counts), 0);
				for (const auto& lit_pair : double_unsets) {
					if (*lit_id_it1 == lit_pair.first.id() && *lit_id_it2 == lit_pair.second.id()) {
						counts[count_index(lit_pair)] += 1;
					} else if (*lit_id_it1 == lit_pair.second.id() && *lit_id_it2 == lit_pair.first.id()) {
						counts[count_index(std::make_pair(lit_pair.second, lit_pair.first))] += 1;
					}
				}

				auto false_count_incr = *std::min_element(begin(counts), end(counts));
				if (false_count_incr != 0) {
					dout(DL::INFO) << "found one!";
				}
				result.false_count += false_count_incr;
			}
		}
	}

	return result;
}

template<
	template <typename...> class LiteralMap
>
struct CNFEvaluation {
	template<typename... Args> using DisjunctionMap = std::vector<Args...>;
	using DisjunctionID = CNFExpression::DisjunctionID;

	struct Counts {
		int falseCount() const { return false_count; }
		int undecidableCount() const { return undecidable_count; }

		int false_count = 0;
		int undecidable_count = 0;
	};

	enum class LiteralSetting : char {
		FARSE, TRUTH, UNSET,
	};

	struct DisjunctionStatus {
		bool decidable() const { return num_unknown <= 0 || num_true > 0; }
		int numUnknown() const { return num_unknown; }
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

	struct LiteralStatus {
		LiteralStatus()
			: setting(LiteralSetting::UNSET)
			, extra_false_settings(0)
		{ }

		bool unset() const { return setting == LiteralSetting::UNSET; }
		bool valueIfSet() const { return setting == LiteralSetting::TRUTH; }

		void set(bool value) { if (value) { setting = LiteralSetting::TRUTH; } else { setting = LiteralSetting::FARSE; } }
		void setAsUnset() { setting = LiteralSetting::UNSET; }

	private:
		LiteralSetting setting;
		int extra_false_settings;
	};

	using DisjunctionsWithLiterals = LiteralMap<std::vector<DisjunctionStatus*>>;

	CNFEvaluation(const CNFExpression& expr)
		: expression(expr)
		, disjunction_status(create_all_unkown_disjunction_status(expr.all_disjunctions()))
		, disjunctions_with_literal(find_disjunctions_with_literals(expr.all_disjunctions(), disjunction_status, expr.all_literals()))
		, literal_status((int)expr.all_literals().size() + 1)
		, current_result{0, (int)expr.all_disjunctions().size()}
	{ }

	const Counts& addSetting(const LiteralID& lit_id, bool value) {
		literal_status.at(lit_id.getValue()).set(value);

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
			} else if (not status.decidable() and status.numUnknown() == 1) {

			}
		}
		return current_result;
	}

	const Counts& removeSetting(const LiteralID& lit_id, bool value) {
		literal_status.at(lit_id.getValue()).setAsUnset();

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
	// auto getCounts() const { return eval_disjunctions(expression, literal_status); }

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
	LiteralMap<LiteralStatus> literal_status;

	Counts current_result;
};

#endif /* DATASTRUCTURES__CNF_EVALUATION_HPP */
