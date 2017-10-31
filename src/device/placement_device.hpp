#ifndef DEVICE__PLACEMENT_DEVICE_H
#define DEVICE__PLACEMENT_DEVICE_H

#include <device/placement_ids.hpp>
#include <graphics/geometry.hpp>

namespace device {

struct PlacementDevice {
	using Bounds = geom::BoundBox<int>;

	PlacementDevice(Bounds device_bounds)
		: device_bounds(device_bounds)
	{ }

	auto& info()       { return *this; }
	auto& info() const { return *this; }
	auto& bounds()       { return device_bounds; }
	auto& bounds() const { return device_bounds; }

private:
	Bounds device_bounds;
};

} // end namespace device

#endif // DEVICE__PLACEMENT_DEVICE_H
