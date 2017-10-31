#include "fpga_graphics_data.hpp"

#include <graphics/graphics.hpp>
#include <util/graph_algorithms.hpp>
#include <util/lambda_compose.hpp>
#include <util/logging.hpp>

namespace {

template<typename T>
bool is_contained_in_a_path(device::RouteElementID reid, const T& paths) {
	for (const auto& path : paths) {
		if (std::find(begin(path), end(path), reid) != end(path)) {
			return true;
		}
	}
	return false;
}

template<typename X, typename Y>
auto bounds_for_xy(X x, Y y) {
	auto x1 = static_cast<float>(x)*1.0f;
	auto y1 = static_cast<float>(y)*1.0f;
	return graphics::t_bound_box(x1, y1, x1 + 1.0f, y1 + 1.0f);
}

auto block_bounds_within(graphics::t_bound_box grid_square_bounds) {
	return graphics::t_bound_box(
		grid_square_bounds.bottom_left() + graphics::t_point(grid_square_bounds.get_width()/2, grid_square_bounds.get_height()/2),
		grid_square_bounds.top_right()
	);
}

template<typename X, typename Y, typename Dir>
auto channel_bounds_for_block_at(X x, Y y, Dir dir) -> graphics::t_bound_box {
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
}

template<typename Dir>
auto channel_location(Dir dir) {
	switch (dir) {
		case decltype(dir)::HORIZONTAL:
			return device::BlockSide::BOTTOM;
		case decltype(dir)::VERTICAL:
			return device::BlockSide::LEFT;
		default:
			return device::BlockSide::OTHER;
	}
}

template<typename Device>
std::pair<graphics::t_point, graphics::t_point> calculateWireLocation(const device::RouteElementID& reid, Device&& device) {
	const auto xy_loc = geom::make_point(
		reid.getX().getValue(),
		reid.getY().getValue()
	);

	const auto grid_square_bounds = bounds_for_xy(xy_loc.x(), xy_loc.y());
	const auto block_bounds = block_bounds_within(grid_square_bounds);

	if (reid.isPin()) {
		const auto as_pin = reid.asPin();
		const auto pin_side = device.get_block_pin_side(as_pin);
		const auto channel_bounds = channel_bounds_for_block_at(xy_loc.x(), xy_loc.y(), pin_side);

		const auto block_side = block_bounds.get_side(pin_side);
		const auto channel_side = channel_bounds.get_side(pin_side);

		const auto block_point = interpolate(block_side.first, 2.0f/3.0f, block_side.second);
		const auto channel_point = interpolate(channel_side.first, 2.0f/3.0f, channel_side.second);

		const auto p1 = block_point;
		const auto p2 = interpolate(block_point, 0.9f, channel_point);

		return {p1, p2};
	} else { // if (device.is_channel(reid)) {
		const auto wire_dir = device.wire_direction(reid);
		const auto channel_bounds = channel_bounds_for_block_at(xy_loc.x(), xy_loc.y(), channel_location(wire_dir));

		const auto sides = [&]() -> std::pair<std::pair<graphics::t_point, graphics::t_point>, std::pair<graphics::t_point, graphics::t_point>> {
			using device::BlockSide;
			switch (wire_dir) {
				case decltype(wire_dir)::HORIZONTAL:
					return {channel_bounds.get_side(BlockSide::LEFT), channel_bounds.get_side(BlockSide::RIGHT)};
				case decltype(wire_dir)::VERTICAL:
				default:
					return {channel_bounds.get_side(BlockSide::TOP), channel_bounds.get_side(BlockSide::BOTTOM)};
			}
		}();

		const auto channel_pos = ((float)device.index_in_channel(reid) + 0.5f) / (float)device.info().track_width;
		const auto p1 = interpolate(sides.first.first, channel_pos, sides.first.second);
		const auto p2 = interpolate(sides.second.second, channel_pos, sides.second.first);
		return {p1, p2};
	}
}

template<typename Device>
auto drawBlocks(Device&& device) {
	std::unordered_map<decltype(*begin(device.blocks())), graphics::t_bound_box> block_locations;
	for (const auto& block : device.blocks()) {
		const auto xy_loc = geom::make_point(
			block.getX().getValue(),
			block.getY().getValue()
		);

		const auto grid_square_bounds = bounds_for_xy(xy_loc.x(), xy_loc.y());
		const auto block_bounds = block_bounds_within(grid_square_bounds);
		block_locations.emplace(block, block_bounds);

		graphics::drawrect(block_bounds);
		graphics::settextrotation(0);
		graphics::setfontsize(8);
		graphics::drawtext_in(block_bounds, util::stringify_through_stream(geom::make_point(block.getX().getValue(), block.getY().getValue())));
	}

	return block_locations;
}

template<typename Device>
void drawDevice(Device&& device, const graphics::FPGAGraphicsDataState& data) {

	graphics::setlinestyle(graphics::SOLID);
	graphics::setcolor(0,0,0);

	std::vector<device::RouteElementID> reIDs_to_draw;
	std::unordered_set<device::RouteElementID> already_drawn;
	std::unordered_map<device::RouteElementID, graphics::t_color> colour_overrides;

	drawBlocks(device);

	for (const auto& block : device.blocks()) {
		for (const auto& pin_re : device.fanout(block)) {
			reIDs_to_draw.push_back(pin_re);
		}
	}

	const auto& drawWire = [&](const auto& curr, boost::optional<graphics::t_color> colour = boost::none) {
		const auto wire_loc = calculateWireLocation(curr, device);
		const auto angle = geom::inclination(geom::make_point(wire_loc.first.x, wire_loc.first.y), geom::make_point(wire_loc.second.x, wire_loc.second.y));

		if (colour) {
			graphics::setcolor(*colour);
		} else {
			const auto colour_overrides_find_result = colour_overrides.find(curr);
			if (colour_overrides_find_result != end(colour_overrides)) {
				graphics::setcolor(colour_overrides_find_result->second);
			} else {
				const auto extracolours_find_result = data.getExtraColours().find(curr);
				if (extracolours_find_result != end(data.getExtraColours())) {
					graphics::setcolor(extracolours_find_result->second);
				} else {
					return wire_loc;
				}
			}
		}

		graphics::drawline(wire_loc.first, wire_loc.second);
		graphics::settextrotation((int)angle.degreeValue() % 180);
		graphics::drawtext_in(graphics::t_bound_box(wire_loc.first,wire_loc.second), util::stringify_through_stream(curr), 0.02f);

		graphics::setcolor(0,0,0);

		return wire_loc;
	};

	const auto& drawConnection = [&](const auto& colour, const std::pair<device::RouteElementID, std::pair<graphics::t_point, graphics::t_point>>& wire_and_loc1, const std::pair<device::RouteElementID, std::pair<graphics::t_point, graphics::t_point>>& wire_and_loc2) {
		const auto point_list = {
			std::make_pair(wire_and_loc1.second.first,  wire_and_loc2.second.first),
			std::make_pair(wire_and_loc1.second.first,  wire_and_loc2.second.second),
			std::make_pair(wire_and_loc1.second.second, wire_and_loc2.second.first),
			std::make_pair(wire_and_loc1.second.second, wire_and_loc2.second.second),
		};

		const auto point_pair = *std::min_element(begin(point_list), end(point_list), [](const auto& lhs, const auto& rhs) {
			return geom::l1_distance(geom::make_point(lhs.first.x, lhs.first.y), geom::make_point(lhs.second.x, lhs.second.y))
				< geom::l1_distance(geom::make_point(rhs.first.x, rhs.first.y), geom::make_point(rhs.second.x, rhs.second.y));
		});

		graphics::setcolor(colour);
		graphics::drawline(point_pair.first, point_pair.second);

		const float arrow_size = 0.01f;
		if (LOD_screen_area_test(graphics::t_bound_box({0,0}, arrow_size, arrow_size))) {

			const auto t_point_delta = point_pair.second - point_pair.first;
			const auto delta = geom::make_point(t_point_delta.x, t_point_delta.y);
			const auto unit_delta = unit(delta);
			const auto perp = unit(perpindictular(delta));
			const auto offset1 = ( perp - unit_delta)*arrow_size;
			const auto offset2 = (-perp - unit_delta)*arrow_size;
			graphics::t_point arrow_points[] = {
				point_pair.second,
				point_pair.second + graphics::t_point(offset1.x(), offset1.y()),
				point_pair.second + graphics::t_point(offset2.x(), offset2.y()),
			};

			using std::end;
			using std::begin;
			graphics::fillpoly(arrow_points, (int)std::distance(begin(arrow_points), end(arrow_points)));
		}
	};

	{ int net_number = 0;
	for (const auto& root_id : data.getNetlist().roots()) {
		const auto net_colour = graphics::t_color(0x00, (uint_fast8_t)(0x66 + (net_number*0x07) % (0xFF-0x66)), (uint_fast8_t)(0x66 + (net_number*0x05) % (0xCC-0x66)));
		struct IterState {
			device::RouteElementID parent = util::make_id<device::RouteElementID>();
			std::pair<graphics::t_point, graphics::t_point> parent_loc = {};
		};
		data.getNetlist().for_all_descendants(root_id, IterState(), [&](const auto& id, const auto& state) -> IterState {
			auto wire_loc = drawWire(id, net_colour);
			if (state.parent != (IterState()).parent) {
				drawConnection(net_colour, {state.parent, state.parent_loc}, {id, wire_loc});
			}
			already_drawn.insert(id);
			reIDs_to_draw.push_back(id); // make sure that other fanouts are drawn?
			return {id, wire_loc};
		});
		net_number++;
	}}

	for (const auto& path : data.getPaths()) {
		boost::optional<std::pair<device::RouteElementID, std::pair<graphics::t_point, graphics::t_point>>> prev_loc;
		const auto path_colour = graphics::t_color(0x00, 0x00, 0xFF);
		for (const auto& id : path) {
			auto wire_loc = drawWire(id, path_colour);
			if (prev_loc) {
				drawConnection(path_colour, *prev_loc, {id, wire_loc});
			}
			prev_loc = std::make_pair(id, wire_loc);
			already_drawn.insert(id);
			reIDs_to_draw.push_back(id); // make sure that other fanouts are drawn?
		}
	}


	util::GraphAlgo<device::RouteElementID>().breadthFirstVisit(device, reIDs_to_draw, [&](const auto& curr) {
		if (already_drawn.find(curr) == end(already_drawn)) {
			drawWire(curr);
		}
	});

}

template<typename Device>
void drawDevice(Device* device, const graphics::FPGAGraphicsDataState& data) {
	if (device) {
		drawDevice(*device, data);
	}
}

template<> void drawDevice(const std::nullptr_t&, const graphics::FPGAGraphicsDataState&) { }
// template<> void drawDevice(std::nullptr_t&, const graphics::FPGAGraphicsDataState&) { }

void drawPlacementData(const graphics::detail::FPGAGraphicsDataState_Placement& data) {

	graphics::setcolor(0,0,0);
	const auto block_bounds = drawBlocks(device::Device<device::WiltonConnector>(device::DeviceInfo{
		device::DeviceType::Wilton,
		geom::BoundBox<int>(0,0,40,40),
		10,
		1,
		2,
	}));

	auto location_of = [&](const auto& atom) -> graphics::t_point {
		const auto fixed_block_lookup = data.fixedBlockLocations().find(atom);
		if (fixed_block_lookup == end(data.fixedBlockLocations())) {
			const auto& raw_point = data.moveableBlockLocations().at(atom);
			return {(float)raw_point.x() + 0.75f, (float)raw_point.y() + 0.75f};
		} else {
			return block_bounds.at(fixed_block_lookup->second).get_center();
		}
	};

	for (const auto& id_and_block : data.fixedBlockLocations()) {
		const auto& id = id_and_block.first;
		const auto& block = id_and_block.second;

		graphics::setcolor(0xFF,0,0);
		graphics::setfontsize(10);
		graphics::drawtext_in(block_bounds.at(block) + graphics::t_point(0.0f, -0.1f), util::stringify_through_stream(id));
	}

	for (const auto& id_and_point : data.moveableBlockLocations()) {
		const auto& id = id_and_point.first;
		const auto point = location_of(id);

		graphics::setcolor(0x00, 0xFF, 0x00);
		graphics::fillarc(point.x, point.y, 0.1f, 0, 360);
		graphics::setcolor(0x00, 0x00, 0x00);
		graphics::drawtext(point.x, point.y, util::stringify_through_stream(id));
	}

	graphics::setcolor(0x00, 0x00, 0x00);
	for (const auto& net : data.netMembers()) {
		for (const auto& atom : net) {
			for (const auto& atom2 : net) {
				if (atom != atom2) {
					const auto p = location_of(atom);
					const auto p2 = location_of(atom2);

					drawline(p, p2);
				}
			}
		}
	}
}

} // end anon namespace

namespace graphics {

void FPGAGraphicsData::do_graphics_refresh(bool reset_view, const geom::BoundBox<float>& fpga_bb) {
	if (reset_view) {
		const float margin = 3.0f;
		graphics::set_visible_world(fpga_bb.minx()-margin, fpga_bb.miny()-margin, fpga_bb.maxx()+margin, fpga_bb.maxy()+margin);
	}
	graphics::refresh_graphics();
}

void FPGAGraphicsData::drawAll() {
	graphics::clearscreen();
	const auto& data_and_lock_copy = dataAndLock();
	const auto& data = data_and_lock_copy.first;

	boost::apply_visitor(util::compose_withbase<boost::static_visitor<void>>([&](auto&& device) {
		drawDevice(device, data);
	}), data.getDevice());

	drawPlacementData(data);
}

} // end namespace graphics
