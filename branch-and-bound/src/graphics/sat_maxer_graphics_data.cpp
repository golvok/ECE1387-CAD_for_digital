#include "sat_maxer_graphics_data.hpp"

#include <graphics/graphics.hpp>
#include <util/logging.hpp>

SatMaxerGraphicsData::SatMaxerGraphicsData()
	: paths_to_draw(500)
{ }

void SatMaxerGraphicsData::addPath(std::vector<Literal>&& path) {
	paths_to_draw.push(new std::vector<Literal>(std::move(path)));
}

void SatMaxerGraphicsData::drawAll() {
	using graphics::t_point;

	const float WIDTH = 1024;
	const float LEVEL_STEP = 5;
	const float NUM_LEVELS = 152;
	graphics::set_visible_world(-WIDTH, 0, WIDTH, LEVEL_STEP*NUM_LEVELS);

	while (true) {
	paths_to_draw.consume_all([&](auto& path_ptr) {
		auto& path = *path_ptr;

		float curr_width = WIDTH;

		t_point previous(0,0);
		for (const auto& lit : path) {
			curr_width /= 2;

			auto xoffset = (curr_width/2) * (lit.inverted() ? -1 : 1);
			t_point curr = previous + t_point(xoffset, LEVEL_STEP);

			graphics::drawline(previous,curr);

			// dout(DL::INFO) << "draw " << previous << " -> " << curr << '\n';

			previous = curr;
		}

		delete path_ptr;
	});
}
}
