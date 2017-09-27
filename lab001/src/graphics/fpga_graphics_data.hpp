#ifndef GRAPHICS__FPGA_GRAPHICS_DATA_H
#define GRAPHICS__FPGA_GRAPHICS_DATA_H

#include <device/connectors.hpp>
#include <device/device.hpp>

#include <vector>

namespace graphics {

class FPGAGraphicsData {
public:
	FPGAGraphicsData()
		: fc_dev(nullptr)
		, paths()
	{ }

	FPGAGraphicsData(const FPGAGraphicsData&) = delete;
	FPGAGraphicsData(FPGAGraphicsData&&) = default;

	FPGAGraphicsData& operator=(const FPGAGraphicsData&) = delete;
	FPGAGraphicsData& operator=(FPGAGraphicsData&&) = default;

	void setFCDev(device::Device<device::FullyConnectedConnector> const* fc_dev);

	void clearPaths() { paths.clear(); }
	void addPath(const std::vector<device::RouteElementID>& path) { paths.push_back(path); }
	auto& getPaths() const { return paths; }

	void drawAll();

private:
	device::Device<device::FullyConnectedConnector> const* fc_dev;

	std::vector<std::vector<device::RouteElementID>> paths;
};

} // end namespace graphics

#endif // GRAPHICS__FPGA_GRAPHICS_DATA_H
