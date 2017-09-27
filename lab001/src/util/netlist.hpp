#ifndef UTIL__NETLIST_H
#define UTIL__NETLIST_H

#include <unordered_map>
#include <unordered_set>

namespace util {

template<typename NODE_ID>
class Netlist {
public:
	Netlist()
		: connections()
	{ }

	void add_connection(const NODE_ID& source, const NODE_ID& sink) {
		connections[source].emplace(sink);
	}

	auto begin() const { return std::begin(connections); }
	auto end()   const { return   std::end(connections); }
private:
	std::unordered_map<NODE_ID,std::unordered_set<NODE_ID>> connections;
};

} // end namespace util

#endif // UTIL__NETLIST_H
