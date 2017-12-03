#include "cnf_expression.hpp"

#include <util/logging.hpp>
#include <util/utils.hpp>

#include <unordered_map>

std::vector<std::pair<VariableOrder, std::vector<std::string>>> variableOrderStrings {
	{ VariableOrder::FILE,                        {"FILE","F"} },
	{ VariableOrder::GROUPED_BY_DISJUNCTION,      {"GROUPED_BY_DISJUNCTION","GBD"} },
	{ VariableOrder::MOST_COMMON_FIRST,           {"MOST_COMMON_FIRST","MCF"} },
};

std::vector<VariableOrder> variableOrders {
	VariableOrder::FILE,
	VariableOrder::GROUPED_BY_DISJUNCTION,
	VariableOrder::MOST_COMMON_FIRST,
};

const std::vector<VariableOrder>& allVariableOrders() {
	return variableOrders;
}

const std::vector<std::string>& stringsFor(VariableOrder vo) {
	for (const auto& enum_and_string_list : variableOrderStrings) {
		if (enum_and_string_list.first == vo) {
			return enum_and_string_list.second;
		}
	}

	util::print_and_throw<std::runtime_error>([&](auto&& str) {
		str << "no strings configured for VariableOrder " << (int)vo;
	});
}

VariableOrder variableOrderFromString(const std::string& s) {
	for (const auto& enum_and_string_list : variableOrderStrings) {
		for (const auto& str : enum_and_string_list.second) {
			if (str == s) {
				return enum_and_string_list.first;
			}
		}
	}

	util::print_and_throw<std::invalid_argument>([&](auto&& str) {
		str << "can't convert `" << s << "` to a VariableOrder";
	});
}


CNFExpression::CNFExpression(const std::vector<VariableOrder>& ordering, const std::vector<std::vector<int>>& data)
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
	std::unordered_map<Literal, int> literal_file_pos;
	std::unordered_map<LiteralID, int> literal_id_file_pos;
	std::unordered_map<LiteralID, int> literal_id_first_disjunction;

	int disjunction_pos = 0;
	int curr_file_pos = 0;
	for (const auto& disjunction : all_disjunctions()) {
		for (const auto& lit : disjunction) {
			literal_counts[lit] += 1;
			literal_counts[~lit] += 0; // ensure it exists
			literal_id_file_pos.emplace(lit.id(), curr_file_pos);
			literal_file_pos.emplace(lit, curr_file_pos);
			literal_id_first_disjunction.emplace(lit.id(), disjunction_pos);
			curr_file_pos += 1;
		}
		disjunction_pos += 1;
	}

	std::unordered_map<int, std::vector<Literal>> count_to_literal;
	for (const auto& lit_and_count : literal_counts) {
		count_to_literal[lit_and_count.second].push_back(lit_and_count.first);
	}
	{auto indent = dout(DL::DATA_READ1).indentWithTitle("Occurance Counts");
		util::print_assoc_container(dout(DL::DATA_READ1), count_to_literal, "\n", "", "", [&](auto& str, auto& lit_list) { util::print_container(str, lit_list); });
		dout(DL::DATA_READ1) << '\n';
	}

	for (const auto& lit_id_and_count : literal_id_file_pos) {
		m_all_literals.push_back(lit_id_and_count.first);
	}

	auto by_literal_id_file_pos = [&](auto& lhs, auto& rhs) {
		return literal_id_file_pos.at(lhs) < literal_id_file_pos.at(rhs);
	};

	auto by_literal_count = [&](auto& lhs, auto& rhs) {
		return ( literal_counts.at(Literal(true,lhs)) + literal_counts.at(Literal(false,lhs)) )
		     > ( literal_counts.at(Literal(true,rhs)) + literal_counts.at(Literal(false,rhs)) );
	};

	auto by_disjunction = [&](auto& lhs, auto& rhs) {
		return literal_id_first_disjunction.at(lhs) < literal_id_first_disjunction.at(rhs);
	};

	for (auto it = rbegin(ordering); it != rend(ordering); ++it) {
		std::stable_sort(begin(m_all_literals), end(m_all_literals), [&](auto& lhs, auto& rhs) {
			switch (*it) {
				case VariableOrder::FILE: {
					return by_literal_id_file_pos(lhs, rhs);
				break; } case VariableOrder::MOST_COMMON_FIRST: {
					return by_literal_count(lhs, rhs);
				break; } case VariableOrder::GROUPED_BY_DISJUNCTION: {
					return by_disjunction(lhs, rhs);
				break; } default: {
					util::print_and_throw<std::runtime_error>([&](auto& str) {
						str << "unhandled vaiable ordering type: " << (int)*it;
					}); throw "impossible";
				}
			}
		});
	}
}

int CNFExpression::max_literal() const {
	int result = 0;
	for (const auto& disj : all_disjunctions()) {
		for (const auto& lit : disj) {
			result = std::max(result, (int)lit.id().getValue());
		}
	}
	return result;
}
