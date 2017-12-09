#ifndef GRAHPCIS__GRAPHICS_WRAPPER_H
#define GRAHPCIS__GRAPHICS_WRAPPER_H

#include <memory>

namespace graphics {

/**
 * The main API for making windows and graphics elements.
 */
class Graphics {
	bool enabled;
	// PIMPL to keep windowing & drawing dependencies contained
	class Impl;
	std::unique_ptr<Impl> impl;

public:
	Graphics();
	virtual ~Graphics();

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
	 * cause the screen to redraw
	 */
	void refresh();

	/**
	 * Exit the graphics. .join() will return soon after
	 */
	void close();

	/**
	 * Wait for all threads to exit and windows to close
	 */
	void join();

	void startThreadsAndOpenWindow();

	virtual void drawAll() { }
private:
};

} // end namespace graphics

#endif // GRAHPCIS__GRAPHICS_WRAPPER_H
