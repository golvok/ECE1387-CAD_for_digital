#ifndef GRAPHICS__GRAPHICS_WRAPPER_SAT_MAXER_H
#define GRAPHICS__GRAPHICS_WRAPPER_SAT_MAXER_H

#include <graphics/sat_maxer_graphics_data.hpp>
#include <graphics/graphics_wrapper.hpp>

namespace graphics {

class SatMaxerGraphics : public Graphics {
public:
	SatMaxerGraphicsData graphics_data;

	SatMaxerGraphics()
		: graphics_data()
	{ }

	void drawAll() override {
		graphics_data.drawAll();
	}

	SatMaxerGraphicsData& tree() { return graphics_data; }
};

/**
 * Singleton getter for graphics
 */
inline SatMaxerGraphics& get() {
	static SatMaxerGraphics singleton;
	return singleton;
}

} // end namespace graphics

#endif /* GRAPHICS__GRAPHICS_WRAPPER_SAT_MAXER_H */
