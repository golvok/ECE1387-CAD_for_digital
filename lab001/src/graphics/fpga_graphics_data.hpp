#ifndef GRAPHICS__FPGA_GRAPHICS_DATA_H
#define GRAPHICS__FPGA_GRAPHICS_DATA_H

#include <device/connectors.hpp>
#include <device/device.hpp>

namespace graphics {

class FPGAGraphicsData {
public:
	FPGAGraphicsData()
		: fc_dev(nullptr)
	{ }

	void setFCDev(device::Device<device::FullyConnectedConnector>* fc_dev);

	void drawAll();

private:
	device::Device<device::FullyConnectedConnector>* fc_dev;
};

} // end namespace graphics

#endif // GRAPHICS__FPGA_GRAPHICS_DATA_H
