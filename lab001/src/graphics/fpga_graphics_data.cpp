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
	graphics::setcolor(0,0,0);

	const auto bounds_for_xy = [](auto x, auto y) {
		auto x1 = x*1.0f;
		auto y1 = y*1.0f;
		return graphics::t_bound_box(x1, y1, x1 + 1.0f, y1 + 1.0f);
	};

	for (const auto& block : fc_dev->blocks()) {
		const auto grid_square_bounds = bounds_for_xy(
			block.getX().getValue(),
			block.getY().getValue()
		);

		const auto block_bounds = graphics::t_bound_box(
			grid_square_bounds.bottom_left() + graphics::t_point(grid_square_bounds.get_width()/2, grid_square_bounds.get_height()/2),
			grid_square_bounds.top_right()
		);

		graphics::drawrect(block_bounds);
		graphics::drawtext_in(block_bounds, util::stringify_through_stream(geom::make_point(block.getX().getValue(), block.getY().getValue())));

		for (const auto& pin_re : fc_dev->fanout(block)) {
			if (pin_re.isPin()) {
				const auto as_pin = pin_re.asPin();
				const auto pin_side = fc_dev->get_block_pin_side(as_pin);
				switch (pin_side) {
					case decltype(pin_side)::LEFT: {
						const auto point_on_block = interpolate(block_bounds.bottom_left(), 1.0f/3.0f, block_bounds.top_left());
						graphics::drawline(
							point_on_block,
							interpolate(point_on_block, 0.9f, graphics::t_point(grid_square_bounds.left(), point_on_block.y))
						);
					} break;
					case decltype(pin_side)::RIGHT: {
						const auto point_on_block = interpolate(block_bounds.bottom_right(), 2.0f/3.0f, block_bounds.top_right());
						graphics::drawline(
							point_on_block,
							interpolate(point_on_block, 0.9f, graphics::t_point(grid_square_bounds.right() + 0.5f, point_on_block.y))
						);
					} break;
					case decltype(pin_side)::TOP: {
						const auto point_on_block = interpolate(block_bounds.top_left(), 1.0f/3.0f, block_bounds.top_right());
						graphics::drawline(
							point_on_block,
							interpolate(point_on_block, 0.9f, graphics::t_point(point_on_block.x, grid_square_bounds.top() + 0.5f))
						);
					} break;
					case decltype(pin_side)::BOTTOM: {
						const auto point_on_block = interpolate(block_bounds.bottom_left(), 2.0f/3.0f, block_bounds.bottom_right());
						graphics::drawline(
							point_on_block,
							interpolate(point_on_block, 0.9f, graphics::t_point(point_on_block.x, grid_square_bounds.bottom()))
						);
					} break;
					default:
					break;
				}
			}
		}
	}
}

} // end namespace graphics
