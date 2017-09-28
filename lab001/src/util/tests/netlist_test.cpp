#include "../netlist.hpp"

using namespace util;

template<bool IS_TREE>
void shorting_trees_test() {
	Netlist<int, IS_TREE> nlist;

	nlist.addConnection(0,1);
	nlist.addConnection(1,0);

	try {
		nlist.addConnection(2,1);
		if (IS_TREE) {
			throw std::runtime_error("expected exception");
		}
	} catch (std::invalid_argument& e) {
		if (!IS_TREE) {
			throw e;
		}
	}
}

template<bool IS_TREE>
void root_existance() {
	Netlist<int, IS_TREE> nlist;

	nlist.addConnection(1, 11);
	nlist.addConnection(2, 22);
	nlist.addConnection(3, 33);

	const auto& roots = nlist.roots();

	if (std::distance(begin(roots), end(roots)) != 3) {
		throw std::runtime_error("wrong number of roots");
	}
}

template<bool IS_TREE>
void root_replacement() {
	Netlist<int, IS_TREE> nlist;

	nlist.addConnection(2, 3);
	nlist.addConnection(1, 2);

	const auto& roots = nlist.roots();

	if (std::distance(begin(roots), end(roots)) != 1) {
		throw std::runtime_error("wrong number of roots");
	}

	if (*begin(roots) != 1) {
		throw std::runtime_error("root is wrong one");
	}
}

int main() {
	shorting_trees_test<true>();
	shorting_trees_test<false>();

	root_existance<true>();
	root_existance<false>();

	root_replacement<true>();
	root_replacement<false>();
}
