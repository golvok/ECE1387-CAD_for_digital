#include "fpga_graphics_data.hpp"

#include <graphics/graphics.hpp>
#include <util/logging.hpp>

namespace graphics {

void FPGAGraphicsData::setFCDev(device::Device<device::FullyConnectedConnector>* fc_dev) {
	this->fc_dev = fc_dev;
	graphics::set_visible_world(-10,-10,20,20);
}

void FPGAGraphicsData::drawAll() {
	graphics::clearscreen();

	if (!fc_dev) return;

	dout(DL::INFO) << "drawing fc device\n";
}

} // end namespace graphics
