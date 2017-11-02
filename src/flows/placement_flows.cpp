#include "placement_flows.hpp"

#include <algo/analytic_placement.hpp>
#include <flows/flows_common.hpp>
#include <graphics/graphics_wrapper.hpp>

#include <array>

namespace flows {
namespace placement {

namespace {

template<typename Points, typename BlockLocations>
Points convert_to_points(const BlockLocations& bl) {
	Points result;
	std::transform(begin(bl), end(bl), std::inserter(result, end(result)),
		[](auto&& id_and_block) {
			return std::make_pair(
				id_and_block.first,
				geom::make_point<double>(
					(double)id_and_block.second.getX().getValue(),
					(double)id_and_block.second.getY().getValue()
				)
			);
		}
	);
	return result;
}

template<typename MovableAtomLocations, typename FixedBlockLocations, typename NetMembers>
double compute_hpbb_wirelength(
	MovableAtomLocations&& moveable_atom_locations,
	FixedBlockLocations&& fixed_block_locations,
	NetMembers&& net_members
) {
	auto location_of = [&](const auto& atom) -> boost::optional<geom::Point<double>> {
		const auto movable_result = moveable_atom_locations.find(atom);
		if (movable_result != end(moveable_atom_locations)) {
			return movable_result->second;
		} else {
			const auto fixed_block_lookup = fixed_block_locations.find(atom);
			if (fixed_block_lookup != end(fixed_block_locations)) {
				return fixed_block_lookup->second.template asPoint<geom::Point<double>>();
			} else {
				return boost::none;
			}
		}
	};

	double total = 0;

	for (const auto& net : net_members) {
		boost::optional<geom::BoundBox<double>> net_bounds;
		for (const auto& atom : net) {
			const auto& loc = location_of(atom);
			if (loc) {
				if (net_bounds) {
					net_bounds->maxx() = std::max(net_bounds->maxx(), loc->x());
					net_bounds->minx() = std::min(net_bounds->minx(), loc->x());
					net_bounds->maxy() = std::max(net_bounds->maxy(), loc->y());
					net_bounds->miny() = std::min(net_bounds->miny(), loc->y());
				} else {
					net_bounds = geom::BoundBox<double>(*loc, *loc);
				}
			}
		}

		if (net_bounds) {
			total += net_bounds->get_width() + net_bounds->get_height();
		}
	}

	return total;
}

// forward declations
template<typename Device, typename FixedBlockLocations>
struct LegalizationFlow;


template<typename Self, typename Device, typename FixedBlockLocations, typename AnchorLocations = std::unordered_map<device::AtomID, geom::Point<double>>>
struct APLFlowBase : public FlowBase<APLFlowBase<Self, Device, FixedBlockLocations, AnchorLocations>, Device, FixedBlockLocations> {
	using FlowBaseBaseClass = FlowBase<APLFlowBase, Device, FixedBlockLocations>;
	DECLARE_USING_FLOWBASE_MEMBERS_JUST_MEMBERS(FlowBaseBaseClass)


	template<typename ArgSelf>
	APLFlowBase(const APLFlowBase<ArgSelf, Device, FixedBlockLocations, AnchorLocations>& src)
		: FlowBaseBaseClass(src)
		, anchor_locations(src.anchor_locations)
	{ }

	template<typename ArgSelf>
	APLFlowBase(APLFlowBase<ArgSelf, Device, FixedBlockLocations, AnchorLocations>&& src)
		: FlowBaseBaseClass(src)
		, anchor_locations(src.anchor_locations)
	{ }

	APLFlowBase(
		const FlowBaseBaseClass& fb,
		const AnchorLocations& anchor_locations
	)
		: FlowBaseBaseClass(fb)
		, anchor_locations(anchor_locations)
	{ }

	APLFlowBase(
		const Device& dev,
		const FixedBlockLocations& fixed_block_locations,
		const AnchorLocations& anchor_locations,
		int nThreads
	)
		: FlowBaseBaseClass(
			dev,
			fixed_block_locations,
			nThreads
		)
		, anchor_locations(anchor_locations)
	{ }

	Self withDevice(const Device& newDev) const {
		return {
			newDev,
			fixed_block_locations,
			anchor_locations,
			nThreads
		};
	}

	Self withFixedBlockLocations(const FixedBlockLocations& newFBL) const {
		return {
			dev,
			newFBL,
			anchor_locations,
			nThreads
		};
	}

	Self withAnchorLocations(const AnchorLocations& newALoc) const {
		return {
			dev,
			fixed_block_locations,
			newALoc,
			nThreads
		};
	}

	const AnchorLocations& anchor_locations;
};

#define DECLARE_USING_APLFLOWBASE_MEMBERS(...) \
	using __VA_ARGS__::APLFlowBase; \
	using __VA_ARGS__::anchor_locations; \



template<typename Device, typename FixedBlockLocations>
struct SimpleCliqueSolveFlow : public APLFlowBase<SimpleCliqueSolveFlow<Device, FixedBlockLocations>, Device, FixedBlockLocations> {
	DECLARE_USING_FLOWBASE_MEMBERS(SimpleCliqueSolveFlow, APLFlowBase<SimpleCliqueSolveFlow, Device, FixedBlockLocations>)
	DECLARE_USING_APLFLOWBASE_MEMBERS(APLFlowBase<SimpleCliqueSolveFlow, Device, FixedBlockLocations>)

	SimpleCliqueSolveFlow(const SimpleCliqueSolveFlow&) = default;
	SimpleCliqueSolveFlow(SimpleCliqueSolveFlow&&) = default;

	struct SimpleCliqueWeighter {
		template<typename T>
		auto operator() (const device::AtomID& a1, const device::AtomID& a2, const T& net_size) {
			(void)a1; (void)a2;
			return 2.0/(double)net_size;
		}
	};

	template<typename Weighter = SimpleCliqueWeighter>
	auto flow_main(
		const std::vector<std::vector<device::AtomID>>& net_members,
		Weighter weighter = Weighter(),
		bool display_result = true
	) const {
		const auto indent = dout(DL::INFO).indentWithTitle("Simple Clique Solve Flow");

		const auto& result = apl::exact_solution(
			net_members,
			anchor_locations,
			dev,
			weighter
		);

		dout(DL::INFO) << "HPBB WireLength = " << compute_hpbb_wirelength(
			result,
			fixed_block_locations,
			net_members
		) << '\n';

		if (dout(DL::APL_D2).enabled()) {
			{const auto indent = dout(DL::APL_D1).indentWithTitle("Solution");
				util::print_assoc_container(result, dout(DL::APL_D1));
				dout(DL::APL_D1) << '\n';
			}
		}

		if (display_result) {
			const auto graphics_keeper = graphics::get().fpga().pushPlacingState(
				net_members,
				fixed_block_locations,
				anchor_locations,
				result,
				false
			);
			graphics::get().waitForPress();
		}

		return result;
	}
};

template<typename Device, typename FixedBlockLocations>
struct CliqueAndSpreadFLow : public APLFlowBase<CliqueAndSpreadFLow<Device, FixedBlockLocations>, Device, FixedBlockLocations> {
	DECLARE_USING_FLOWBASE_MEMBERS(CliqueAndSpreadFLow, APLFlowBase<CliqueAndSpreadFLow, Device, FixedBlockLocations>)
	DECLARE_USING_APLFLOWBASE_MEMBERS(APLFlowBase<CliqueAndSpreadFLow, Device, FixedBlockLocations>)

	CliqueAndSpreadFLow(const CliqueAndSpreadFLow&) = default;
	CliqueAndSpreadFLow(CliqueAndSpreadFLow&&) = default;

	template<typename ShouldStop>
	auto flow_main(
		const std::vector<std::vector<device::AtomID>>& net_members,
		ShouldStop&& should_stop,
		bool display_result = true
	) const {
		const auto indent = dout(DL::INFO).indentWithTitle("Clique And Spread Flow");
		const auto graphics_keeper = graphics::get().fpga().pushPlacingState(
			{},
			fixed_block_locations,
			{},
			{},
			true
		);
		using AtomID = device::AtomID;
		using BlockID = device::BlockID;

		std::unordered_map<AtomID, int> anchor_generation;
		auto current_anchor_locations = anchor_locations; // copy
		auto current_net_members = net_members; // copy
		auto current_bb = geom::BoundBox<double>(dev.info().bounds());
		current_bb.min_point() -= geom::make_point(0.5, 0.5);
		current_bb.max_point() += geom::make_point(0.5, 0.5);

		util::IDGenerator<AtomID> atom_id_gen(100000); // TODO have the file parser return something better

		for (int num_spreadings = 0; /* always */; ++num_spreadings) {
			if (should_stop(num_spreadings)) {
				break;
			}

			const auto moveable_atom_locations = SimpleCliqueSolveFlow<Device, FixedBlockLocations>(*this)
				.withAnchorLocations(current_anchor_locations)
				.flow_main(
					current_net_members,
					[&] (const device::AtomID& a1, const device::AtomID& a2, const auto& net_size) {
						boost::optional<AtomID> anchor;
						if (current_anchor_locations.find(a1) != end(current_anchor_locations) && this->fixed_block_locations.find(a1) == end(this->fixed_block_locations)) {
							anchor = a1;
						} else if (current_anchor_locations.find(a2) != end(current_anchor_locations) && this->fixed_block_locations.find(a2) == end(this->fixed_block_locations)) {
							anchor = a2;
						}

						if (anchor) {
							const auto& agen = anchor_generation.at(anchor.get());
							return 10.0 * std::pow(1.1, 3*agen) * std::pow(2, agen - num_spreadings);
						} else {
							return 2.0/(double)net_size;
						}
					},
					false
				);

			const auto anchor_infos = compute_anchors(
				moveable_atom_locations,
				num_spreadings,
				current_bb,
				atom_id_gen
			);

			auto legalization = LegalizationFlow<Device, FixedBlockLocations>(*this).flow_main(
				moveable_atom_locations
			);

			std::unordered_map<BlockID, int> block_usage;
			for (const auto& block_and_atom : legalization) {
				block_usage[block_and_atom.first] += 1;
			}

			int overused_count = 0;
			for (const auto& block_and_atom : legalization) {
				if (block_usage.at(block_and_atom.first) > 1) {
					overused_count += 1;
				}
			}

			{const auto indent = dout(DL::INFO).indentWithTitle("Legalization Result");
				util::print_assoc_container(legalization, dout(DL::APL_D1), "", "", "\n");
				{const auto indent = dout(DL::APL_D2).indentWithTitle("Block Overusage");
					for (const auto& block_and_usage : block_usage) {
						if (block_and_usage.second > 1) {
							dout(DL::APL_D2) << block_and_usage.first << " -> " << block_and_usage.second << '\n';
						}
					}
				}
				std::unordered_map<AtomID, geom::Point<double>> legalization_atom_locations;
				std::transform(begin(legalization), end(legalization), std::inserter(legalization_atom_locations, end(legalization_atom_locations)), [&](const auto& block_and_atom) {
					return std::make_pair(block_and_atom.second, block_and_atom.first.template asPoint<geom::Point<double>>());
				});
				dout(DL::INFO) << "Legalized HPBB WireLength = " << compute_hpbb_wirelength(legalization_atom_locations, fixed_block_locations, net_members) << '\n';
				dout(DL::INFO) << "Atom-based overuse: " << overused_count << '/' << moveable_atom_locations.size() << " (" << 100.0*(double)overused_count/(double)moveable_atom_locations.size() << "%)\n";

			}

			auto assignments = assign_to_anchors(
				anchor_infos,
				moveable_atom_locations
			);

			{const auto indent = dout(DL::APL_D3).indentWithTitle("New Assignments");
			for (const auto new_net : assignments.new_nets) {
				dout(DL::APL_D3) << new_net[0] << " -> " << new_net[1] << '\n';
			}}

			{ const auto indent = dout(DL::APL_D1).indentWithTitle("New Anchors");
			for (const auto anchor_info : anchor_infos) {
				dout(DL::APL_D1) << anchor_info.id << " @ " << anchor_info.anchor_location << " <- " << anchor_info.filter << '\n';
				current_anchor_locations.emplace(
					anchor_info.id,
					anchor_info.anchor_location
				);
				anchor_generation.emplace(
					anchor_info.id,
					num_spreadings
				);
			}}

			std::move(begin(assignments.new_nets), end(assignments.new_nets), std::back_inserter(current_net_members));

			if (display_result) {
				const auto graphics_keeper = graphics::get().fpga().pushPlacingState(
					current_net_members,
					fixed_block_locations,
					current_anchor_locations,
					moveable_atom_locations,
					false
				);
				graphics::get().waitForPress();
			}
		}
	}

	template<typename MovableAtomLocations, typename Bounds>
	static boost::optional<geom::Point<double>> compute_centroid_of_atoms_in(
		const MovableAtomLocations& moveable_atom_locations,
		const Bounds& bounds
	) {
		std::vector<decltype(begin(moveable_atom_locations))> x_order;
		for (auto it = begin(moveable_atom_locations); it != end(moveable_atom_locations); ++it) {
			if (bounds.intersects(it->second)) {
				x_order.push_back(it);
			}
		}
		std::vector<decltype(begin(moveable_atom_locations))> y_order(x_order);

		std::sort(begin(x_order), end(x_order), [&](auto& lhs, auto& rhs) {
			return lhs->second.x() < rhs->second.x();
		});
		std::sort(begin(y_order), end(y_order), [&](auto& lhs, auto& rhs) {
			return lhs->second.y() < rhs->second.y();
		});

		if (x_order.size() == 0) {
			return boost::none;
		} else if (x_order.size() % 2 == 0) {
			return geom::BoundBox<double>(
				x_order[x_order.size()/2 - 1]->second.x(),
				y_order[y_order.size()/2 - 1]->second.y(),
				x_order[x_order.size()/2 ]->second.x(),
				y_order[y_order.size()/2 ]->second.y()
			).get_center();
		} else {
			return geom::Point<double>(
				x_order[x_order.size()/2]->second.x(),
				y_order[y_order.size()/2]->second.y()
			);
		}
	}

	template<typename MovableAtomLocations, typename Bounds, typename AtomIDGen>
	auto compute_anchors(
		MovableAtomLocations&& moveable_atom_locations,
		int num_spreadings,
		Bounds&& current_bb,
		AtomIDGen&& atom_id_gen
	) const {
		using AtomID = device::AtomID;

		int current_num_divisions = 1;
		for (int i = 0; i <= num_spreadings; ++i) {
			current_num_divisions *= 2;
			if (current_num_divisions > 2 * (dev.info().bounds().get_width() + 1)) {
				break;
			}
		}
		const bool snap_to_grid = current_num_divisions == dev.info().bounds().get_width();

		struct Division {
			geom::BoundBox<double> bb;
			int num_subdiv;
			geom::Point<int> index;
		};

		std::vector<Division> divisions{
			Division{current_bb, current_num_divisions, {0,0}},
		};

		while (true) {
			std::vector<Division> next_divisions;
			bool did_a_division = false;
			for (const auto& div : divisions) {
				if (div.num_subdiv == 1) {
					next_divisions.emplace_back(div);
				} else {
					did_a_division = true;
					// TODO if snapping, adjust proportions
					auto centroid = compute_centroid_of_atoms_in(
						moveable_atom_locations,
						div.bb
					);
					if (centroid) {
						auto snapped_centroid = *centroid;
						// if (snap_to_grid) {
						// 	snapped_centroid = geom::Point<int>(snapped_centroid);
						// }
						(void) snap_to_grid;
						struct Subdiv{
							geom::BoundBox<double> bb;
							geom::Point<int> index_offset;
							bool round_up;
						};
						for (const auto& subdiv : {
							Subdiv{ {snapped_centroid, div.bb.max_point()     }, {1,1} , true  },
							Subdiv{ {snapped_centroid, div.bb.minxmaxy_point()}, {0,1} , true },
							Subdiv{ {snapped_centroid, div.bb.min_point()     }, {0,0} , false  },
							Subdiv{ {snapped_centroid, div.bb.maxxminy_point()}, {1,0} , false },
						}) {
							int num_subdiv_adjustment = (div.num_subdiv % 2 != 0 && subdiv.round_up) ? 1 : 0;
							next_divisions.emplace_back(Division{
								subdiv.bb,
								div.num_subdiv/2 + num_subdiv_adjustment,
								div.index*2 + subdiv.index_offset
							});
						}
					}
				}
			}
			
			if (!did_a_division) {
				break;
			} else {
				divisions = std::move(next_divisions);
			}
		}

		struct AnchorInfo {
			geom::BoundBox<double> filter;
			AtomID id;
			geom::Point<double> anchor_location;
		};

		std::vector<AnchorInfo> result;
		for (const auto& div : divisions) {
			result.emplace_back(AnchorInfo{
				div.bb,
				atom_id_gen.gen_id(),
				geom::Point<double>(current_bb.min_point()) + geom::make_point(
					(double)current_bb.get_width()  * ((double)div.index.x() + 0.5)/(double)current_num_divisions,
					(double)current_bb.get_height() * ((double)div.index.y() + 0.5)/(double)current_num_divisions
				)
			});
		}
		return result;
	}

	template<typename AnchorInfos, typename AtomRange>
	static auto assign_to_anchors(
		AnchorInfos&& anchor_info,
		AtomRange&& moveable_atom_locations
	) {
		using AtomID = device::AtomID;

		auto anchor_for = [&](const auto& point) {
			return std::find_if(begin(anchor_info), end(anchor_info), [&](const auto& elem) {
				return elem.filter.intersects(point);
			})->id;
		};

		struct Result {
			std::vector<std::vector<AtomID>> new_nets{};
		} result;

		for (const auto& atom_and_loc : moveable_atom_locations) {
			const auto& anchor_id = anchor_for(atom_and_loc.second);

			std::vector<AtomID> list = {atom_and_loc.first, anchor_id};
			result.new_nets.emplace_back(std::move(list));
		}

		return result;
	}
};

template<typename P>
auto adjacent_points(P point) {
	return util::make_generator<int>(
		0,
		[=](auto&& index) { return index == 8; },
		[=](auto&& index) { return index + 1; },
		[=](auto&& index) {
			switch (index) {
				case 0: return point + std::make_pair( 1, 1);
				case 1: return point + std::make_pair( 1, 0);
				case 2: return point + std::make_pair( 1,-1);
				case 3: return point + std::make_pair( 0,-1);
				case 4: return point + std::make_pair(-1,-1);
				case 5: return point + std::make_pair(-1, 0);
				case 6: return point + std::make_pair(-1, 1);
				case 7: return point + std::make_pair( 0, 1);
				default: throw "unexpected";
			}
		}
	);
}

template<typename Device, typename FixedBlockLocations>
struct LegalizationFlow : public FlowBase<LegalizationFlow<Device, FixedBlockLocations>, Device, FixedBlockLocations> {
	DECLARE_USING_FLOWBASE_MEMBERS(LegalizationFlow, FlowBase<LegalizationFlow, Device, FixedBlockLocations>)

	LegalizationFlow(const LegalizationFlow&) = default;
	LegalizationFlow(LegalizationFlow&&) = default;

	template<typename AtomRange>
	auto flow_main(
		AtomRange&& moveable_atom_locations
	) const {
		const auto& indent = dout(DL::INFO).indentWithTitle("Legalization Flow");
		using device::AtomID;
		using device::BlockID;
		using BlockPoint = geom::Point<std::int16_t>;

		std::unordered_multimap<BlockID, AtomID> snappings;
		std::unordered_set<AtomID> already_snapped;

		auto add_snapping = [&](auto&& block, auto&& atom) {
			snappings.emplace(block, atom);
			already_snapped.emplace(atom);
		};

		auto valid_and_empty = [&](const auto& block_point) {
			return snappings.find(BlockID::fromPoint(block_point)) == end(snappings)
				&& this->dev.info().bounds().intersects(block_point);
		};

		auto metric = [&](const auto& atom, const auto& test_point) {
			return distanceSquared(moveable_atom_locations.at(atom), test_point);
		};

		for (const auto& block_and_loc : fixed_block_locations) {
			add_snapping(block_and_loc.second, block_and_loc.first);
		}

		std::unordered_map<BlockID, std::map<double, AtomID>> atom_distance_to_closest_nonfixed_block;

		for (const auto& atom_and_loc : moveable_atom_locations) {
			auto bp = geom::make_point(
				std::lround(atom_and_loc.second.x()),
				std::lround(atom_and_loc.second.y())
			);

			const auto dist = metric(atom_and_loc.first, geom::Point<double>(bp));

			atom_distance_to_closest_nonfixed_block[BlockID::fromPoint(BlockPoint(bp))][dist] = atom_and_loc.first;
		}

		for (const auto& block_and_dist_to_id : atom_distance_to_closest_nonfixed_block) {
			if (valid_and_empty(block_and_dist_to_id.first.asPoint<BlockPoint>())) {
				add_snapping(block_and_dist_to_id.first, begin(block_and_dist_to_id.second)->second);
			}
		}

		auto best_block_for = [&](const auto& closest_block, const auto& atom) {
			const auto points_temp = adjacent_points(closest_block.template asPoint<BlockPoint>());
			std::vector<BlockPoint> points;
			for (const auto& p : points_temp) {
				points.push_back(BlockPoint(p));
			}

			boost::optional<BlockPoint> best_point;
			{
				double best_point_metric = std::numeric_limits<double>::max();

				for (const auto& test_point : points) {
					if (valid_and_empty(test_point)) {
						auto test_point_metric = metric(atom, test_point);
						if (!best_point || test_point_metric > best_point_metric) {
							best_point_metric = test_point_metric;
							best_point = test_point;
						}
					}
				}
			}

			if (best_point) {
				return BlockID::fromPoint(*best_point);
			} else {
				return closest_block; // overuse
			}
		};

		for (const auto& block_and_dist_to_id : atom_distance_to_closest_nonfixed_block) {
			for (const auto& dist_and_atom : block_and_dist_to_id.second) {
				if (already_snapped.find(dist_and_atom.second) == end(already_snapped)) { // TODO do real tracking of alroady assigned atoms
					add_snapping(best_block_for(block_and_dist_to_id.first, dist_and_atom.second), dist_and_atom.second);
				}
			}
		}

		return snappings;
	}

};

} // end anon namespace

device::PlacementDevice make_default_device_description(const std::vector<std::vector<device::AtomID>>& net_members) {
	std::unordered_set<device::AtomID> unique_atoms;
	for (const auto& net : net_members) {
		for (const auto& atom : net) {
			unique_atoms.insert(atom);
		}
	}
	const auto width = static_cast<int>(std::lround(std::ceil(std::sqrt(unique_atoms.size()))));
	return device::PlacementDevice(device::PlacementDevice::Bounds(0, 0, width-1, width-1));
}

void simple_clique_solve(
	const std::vector<std::vector<device::AtomID>>& net_members,
	const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
	const device::PlacementDevice& device
) {
	SimpleCliqueSolveFlow<std::decay_t<decltype(device)>, std::decay_t<decltype(fixed_block_locations)>>(
		device,
		fixed_block_locations,
		convert_to_points<std::unordered_map<device::AtomID, geom::Point<double>>>(fixed_block_locations),
		1
	).flow_main(
		net_members
	);
}

void clique_and_spread(
	const std::vector<std::vector<device::AtomID>>& net_members,
	const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
	const device::PlacementDevice& device,
	int max_spreadings
) {
	CliqueAndSpreadFLow<std::decay_t<decltype(device)>, std::decay_t<decltype(fixed_block_locations)>>(
		device,
		fixed_block_locations,
		convert_to_points<std::unordered_map<device::AtomID, geom::Point<double>>>(fixed_block_locations),
		1
	).flow_main(
		net_members,
		[&](auto& num_spreadings) {
			return num_spreadings > max_spreadings;
		}
	);
}


}
}
