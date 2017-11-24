#include "sat_maxer_graphics_data.hpp"

#include <thread>

#include <graphics/graphics.hpp>
#include <util/logging.hpp>

SatMaxerGraphicsData::SatMaxerGraphicsData()
	: paths_to_draw()
	, num_levels_visible(-1)
	, width(2048)
	, level_step(10)
{ }

void SatMaxerGraphicsData::setNumLevelsVisible(int num) {
	num_levels_visible = num;
	graphics::set_visible_world(-width/2, 0, width/2, level_step*(float)num_levels_visible);
}

void SatMaxerGraphicsData::addPath(std::vector<Literal>&& path) {
	auto* path_copy = new std::vector<Literal>(std::move(path));
	while (not paths_to_draw.push(path_copy)) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1ns);
	}
}

void SatMaxerGraphicsData::drawAll() {
	if (num_levels_visible < 0) {
		setNumLevelsVisible(200);
	}

	using graphics::t_point;

	while (true) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1ns);

		paths_to_draw.consume_all([&](auto& path_ptr) {
			auto& path = *path_ptr;

			float curr_width = width;

			t_point previous(0,0);
			for (const auto& lit : path) {
				curr_width /= 2;

				auto xoffset = (curr_width/2) * (lit.inverted() ? -1 : 1);
				t_point curr = previous + t_point(xoffset, level_step);

				graphics::drawline(previous,curr);

				// dout(DL::INFO) << "draw " << previous << " -> " << curr << '\n';

				previous = curr;
			}

			delete path_ptr;
		});
	}
}
