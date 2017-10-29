#ifndef DEVICE__PLACEMENT_DEVICE_H
#define DEVICE__PLACEMENT_DEVICE_H

#include <device/placement_ids.hpp>
#include <graphics/geometry.hpp>

namespace device {

struct PlacementDevice {
	using Bounds = geom::BoundBox<int>;

	PlacementDevice(Bounds bounds)
		: bounds(bounds)
	{ }

private:
	Bounds bounds;
};

} // end namespace device

#endif // DEVICE__PLACEMENT_DEVICE_H
