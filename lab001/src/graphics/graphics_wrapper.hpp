#ifndef GRAHPCIS__GRAPHICS_WRAPPER_H
#define GRAHPCIS__GRAPHICS_WRAPPER_H

#include <graphics/fpga_graphics_data.hpp>

#include <memory>

namespace graphics {

class Graphics;
class TrainsArea;

/**
 * Singleton getter for graphics
 */
Graphics& get();

/**
 * The main API for making windows and graphics elements.
 */
class Graphics {
	bool enabled;
	// PIMPL to keep windowing & drawing dependencies contained
	class Impl;
	std::unique_ptr<Impl> impl;

	// this is here (as opposed to being in the Impl) so that it always exists
	// and can be written to.
	FPGAGraphicsData fpga_graphics_data;
public:
	Graphics();

	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;

	void enable() { enabled = true; }
	void disable() { enabled = false; }

	/**
	 * Blocks until the ever-present proceed button is pressed.
	 * Returns immediately if no graphics, or if the window with the continue
	 * button has been closed.
	 */
	void waitForPress();

	/**
	 * Exit the graphics. .join() will return soon after
	 */
	void close();

	/**
	 * Wait for all threads to exit and windows to close
	 */
	void join();

	/**
	 * Get the data interface for the FPGA Area.
	 * The intended way of making data availible to the FPGA Area graphics.
	 */
	FPGAGraphicsData& fpga() { return fpga_graphics_data; }
	void startThreadsAndOpenWindow();
private:
};

} // end namespace graphics

#endif // GRAHPCIS__GRAPHICS_WRAPPER_H
