#ifndef GRAPHICS__GRAPHICS_WRAPPER_FPGA_H
#define GRAPHICS__GRAPHICS_WRAPPER_FPGA_H

#include <graphics/fpga_graphics_data.hpp>
#include <graphics/graphics_wrapper.hpp>

namespace graphics {

class FPGAGraphics : public Graphics {
public:
	// this is here (as opposed to being in the Impl) so that it always exists
	// and can be written to.
	FPGAGraphicsData fpga_graphics_data;

	FPGAGraphics()
		: fpga_graphics_data()
	{ }

	void drawAll() override {
		fpga_graphics_data.drawAll();
	}

	FPGAGraphicsData& fpga() { return fpga_graphics_data; }
};

/**
 * Singleton getter for graphics
 */
inline FPGAGraphics& get() {
	static FPGAGraphics singleton;
	return singleton;
}

} // end namespace graphics

#endif /* GRAPHICS__GRAPHICS_WRAPPER_FPGA_H */
