#include "maze_router.hpp"

namespace algo {

template
std::vector<device::RouteElementID> maze_route<device::RouteElementID, const device::RouteElementID&, const device::RouteElementID&, const device::Device<device::FullyConnectedConnector>&>(
	const device::RouteElementID& source, const device::RouteElementID& dest, const device::Device<device::FullyConnectedConnector>& fanout_gen
);

} // end namespace algo
