#include "fpga_graphics_data.hpp"

#include <graphics/graphics.hpp>
#include <util/logging.hpp>

namespace graphics {

void FPGAGraphicsData::setFCDev(device::Device<device::FullyConnectedConnector> const* fc_dev) {
	this->fc_dev = fc_dev;
	graphics::set_visible_world(-10,-10,20,20);
}

void FPGAGraphicsData::drawAll() {
	graphics::clearscreen();

	if (!fc_dev) return;

	dout(DL::INFO) << "drawing fc device\n";
	graphics::setcolor(0,0,0);

	const auto bounds_for_xy = [&](auto x, auto y) {
		auto x1 = static_cast<float>(x)*1.0f;
		auto y1 = static_cast<float>(y)*1.0f;
		return graphics::t_bound_box(x1, y1, x1 + 1.0f, y1 + 1.0f);
	};

	const auto block_bounds_within = [&](graphics::t_bound_box grid_square_bounds) {
		return graphics::t_bound_box(
			grid_square_bounds.bottom_left() + graphics::t_point(grid_square_bounds.get_width()/2, grid_square_bounds.get_height()/2),
			grid_square_bounds.top_right()
		);
	};

	const auto channel_bounds_for_block_at = [&](auto x, auto y, auto dir) -> graphics::t_bound_box {
		const auto bound_of_me = bounds_for_xy(x, y);
		const auto bound_of_the_right = bounds_for_xy(x+1, y);
		const auto bound_of_the_above = bounds_for_xy(x, y+1);
		const auto block_bounds = block_bounds_within(bound_of_me);
		const auto block_bounds_of_the_right = block_bounds_within(bound_of_the_right);
		const auto block_bounds_of_the_above = block_bounds_within(bound_of_the_above);
		switch (dir) {
			case decltype(dir)::LEFT: {
				return {bound_of_me.top_left(), block_bounds.bottom_left()};
			} break;
			case decltype(dir)::RIGHT: {
				return {block_bounds.top_right(), block_bounds_of_the_right.bottom_left()};
			} break;
			case decltype(dir)::TOP: {
				return {block_bounds.top_right(), block_bounds_of_the_above.bottom_left()};
			} break;
			case decltype(dir)::BOTTOM: {
				return {bound_of_me.bottom_right(), block_bounds.bottom_left()};
			} break;
			default:
				dout(DL::WARN) << "not sure how calculate bounds of " << dir << " channel\n";
				return block_bounds;
			break;
		}
	};

	const auto channel_location = [](auto dir) {
		switch (dir) {
			case decltype(dir)::HORIZONTAL:
				return device::BlockSide::BOTTOM;
			case decltype(dir)::VERTICAL:
				return device::BlockSide::LEFT;
			default:
				return device::BlockSide::OTHER;
		}
	};

	std::list<device::RouteElementID> reIDs_to_draw;
	for (const auto& block : fc_dev->blocks()) {
		const auto xy_loc = geom::make_point(
			block.getX().getValue(),
			block.getY().getValue()
		);

		const auto grid_square_bounds = bounds_for_xy(xy_loc.x(), xy_loc.y());
		const auto block_bounds = block_bounds_within(grid_square_bounds);

		graphics::drawrect(block_bounds);
		graphics::settextrotation(0);
		graphics::drawtext_in(block_bounds, util::stringify_through_stream(geom::make_point(block.getX().getValue(), block.getY().getValue())));

		for (const auto& pin_re : fc_dev->fanout(block)) {
			reIDs_to_draw.push_back(pin_re);
		}
	}

	std::unordered_set<device::RouteElementID> reIDs_already_seen;
	while (!reIDs_to_draw.empty()) {
		const auto curr = reIDs_to_draw.front();
		reIDs_to_draw.pop_front();

		if (reIDs_already_seen.insert(curr).second == false) {
			continue;
		}

		for (const auto& fanout : fc_dev->fanout(curr)) {
			if (reIDs_already_seen.find(fanout) == end(reIDs_already_seen)) {
				reIDs_already_seen.insert(curr);
				reIDs_to_draw.push_back(fanout);
			}
		}

		const auto xy_loc = geom::make_point(
			curr.getX().getValue(),
			curr.getY().getValue()
		);

		const auto grid_square_bounds = bounds_for_xy(xy_loc.x(), xy_loc.y());
		const auto block_bounds = block_bounds_within(grid_square_bounds);

		if (curr.isPin()) {
			const auto as_pin = curr.asPin();
			const auto pin_side = fc_dev->get_block_pin_side(as_pin);
			const auto channel_bounds = channel_bounds_for_block_at(xy_loc.x(), xy_loc.y(), pin_side);

			const auto block_side = block_bounds.get_side(pin_side);
			const auto channel_side = channel_bounds.get_side(pin_side);

			const auto block_point = interpolate(block_side.first, 2.0f/3.0f, block_side.second);
			const auto channel_point = interpolate(channel_side.first, 2.0f/3.0f, channel_side.second);

			const auto p1 = block_point;
			const auto p2 = interpolate(block_point, 0.9f, channel_point);
			const auto angle = inclination(geom::make_point(p1.x, p1.y), geom::make_point(p2.x, p2.y));

			graphics::drawline(p1, p2);
			graphics::settextrotation((int)angle.degreeValue() % 180);
			graphics::drawtext_in(graphics::t_bound_box(p1,p2), util::stringify_through_stream(curr), 0.02f);
		} else { // if (fc_dev->is_channel(curr)) {
			const auto wire_dir = fc_dev->wire_direction(curr);
			const auto channel_bounds = channel_bounds_for_block_at(xy_loc.x(), xy_loc.y(), channel_location(wire_dir));

			const auto sides = [&]() -> std::pair<std::pair<t_point, t_point>, std::pair<t_point, t_point>> {
				using device::BlockSide;
				switch (wire_dir) {
					case decltype(wire_dir)::HORIZONTAL:
						return {channel_bounds.get_side(BlockSide::LEFT), channel_bounds.get_side(BlockSide::RIGHT)};
					case decltype(wire_dir)::VERTICAL:
					default:
						return {channel_bounds.get_side(BlockSide::BOTTOM), channel_bounds.get_side(BlockSide::TOP)};
				}
			}();

			const auto channel_pos = ((float)fc_dev->index_in_channel(curr) + 0.5f) / (float)fc_dev->info().track_width;
			const auto p1 = interpolate(sides.first.first, channel_pos, sides.first.second);
			const auto p2 = interpolate(sides.second.second, channel_pos, sides.second.first);
			const auto angle = inclination(geom::make_point(p1.x, p1.y), geom::make_point(p2.x, p2.y));

			graphics::drawline(p1, p2);
			graphics::settextrotation((int)angle.degreeValue() % 180);
			graphics::drawtext_in(graphics::t_bound_box(p1,p2), util::stringify_through_stream(curr), 0.02f);
		}
	}

	dout(DL::INFO) << "done drawing fg device\n";

}

} // end namespace graphics
