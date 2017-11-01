#ifndef GRAPHICS__FPGA_GRAPHICS_DATA_H
#define GRAPHICS__FPGA_GRAPHICS_DATA_H

#include <device/connectors.hpp>
#include <device/device.hpp>
#include <device/placement_device.hpp>
#include <graphics/graphics_types.hpp>
#include <util/netlist.hpp>

#include <mutex>
#include <vector>

namespace graphics {

class FPGAGraphicsDataStateScope;

namespace detail {

	class FPGAGraphicsDataState_Routing {
		using Netlist = util::Netlist<device::RouteElementID>;
	public:
		FPGAGraphicsDataState_Routing()
			: device(nullptr)
			, paths()
			, netlist()
			, extra_colours_to_draw()
		{ }

		template<typename Device>
		FPGAGraphicsDataState_Routing(
			Device const* device,
			const std::vector<std::vector<device::RouteElementID>>& paths,
			const Netlist& netlist,
			std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw
		)
			: device(device)
			, paths(paths)
			, netlist(netlist)
			, extra_colours_to_draw(std::move(extra_colours_to_draw))
		{ }

		FPGAGraphicsDataState_Routing(const FPGAGraphicsDataState_Routing&) = default;
		FPGAGraphicsDataState_Routing(FPGAGraphicsDataState_Routing&&) = default;

		FPGAGraphicsDataState_Routing& operator=(const FPGAGraphicsDataState_Routing&) = default;
		FPGAGraphicsDataState_Routing& operator=(FPGAGraphicsDataState_Routing&&) = default;

		      auto& getPaths()       { return paths; }
		const auto& getPaths() const { return paths; }

		      auto& getNetlist()       { return netlist; }
		const auto& getNetlist() const { return netlist; }

		      auto& getDevice()       { return device; }
		const auto& getDevice() const { return device; }

		      auto& getExtraColours()       { return extra_colours_to_draw; }
		const auto& getExtraColours() const { return extra_colours_to_draw; }

	private:
		template <typename... Ts> using variant_with_nullptr = boost::variant<std::nullptr_t, Ts...>;
		util::substitute_into<variant_with_nullptr, util::add_pointer_to_const_t, ALL_DEVICES_COMMA_SEP> device;
		std::vector<std::vector<device::RouteElementID>> paths;
		Netlist netlist;
		std::unordered_map<device::RouteElementID, graphics::t_color> extra_colours_to_draw;
	};

	struct FPGAGraphicsDataState_Placement {
		FPGAGraphicsDataState_Placement()
			: FPGAGraphicsDataState_Placement(
				{},
				{},
				{},
				{}
			)
		{ }

		FPGAGraphicsDataState_Placement(
			const std::vector<std::vector<device::AtomID>>& net_members,
			const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
			const std::unordered_map<device::AtomID, geom::Point<double>>& nonmoveable_block_locations,
			const std::unordered_map<device::AtomID, geom::Point<double>>& moveable_block_locations
		)
			: net_members(net_members)
			, fixed_block_locations(fixed_block_locations)
			, nonmoveable_block_locations(nonmoveable_block_locations)
			, moveable_block_locations(moveable_block_locations)
		{ }

		const auto& netMembers() const { return net_members; }
		      auto& netMembers()       { return net_members; }

		const auto& fixedBlockLocations() const { return fixed_block_locations; }
		      auto& fixedBlockLocations()       { return fixed_block_locations; }

		const auto& nonmoveableBlockLocations() const { return nonmoveable_block_locations; }
		      auto& nonmoveableBlockLocations()       { return nonmoveable_block_locations; }

		const auto& moveableBlockLocations() const { return moveable_block_locations; }
		      auto& moveableBlockLocations()       { return moveable_block_locations; }

	private:
		std::vector<std::vector<device::AtomID>> net_members;
		std::unordered_map<device::AtomID, device::BlockID> fixed_block_locations;
		std::unordered_map<device::AtomID, geom::Point<double>> nonmoveable_block_locations;
		std::unordered_map<device::AtomID, geom::Point<double>> moveable_block_locations;
	};

}

struct FPGAGraphicsDataState : public detail::FPGAGraphicsDataState_Routing, public detail::FPGAGraphicsDataState_Placement {
	using Netlist = util::Netlist<device::RouteElementID>;

	struct routing_state_tag {};
	struct placement_state_tag {};

	FPGAGraphicsDataState()
		: FPGAGraphicsDataState_Routing()
		, FPGAGraphicsDataState_Placement()
		, enabled(true)
	{ }

	template<typename Device>
	FPGAGraphicsDataState(
		routing_state_tag,
		Device const* device,
		const std::vector<std::vector<device::RouteElementID>>& paths,
		const Netlist& netlist,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw
	)
		: FPGAGraphicsDataState_Routing(
			device,
			paths,
			netlist,
			std::move(extra_colours_to_draw)
		)
		, FPGAGraphicsDataState_Placement()
		, enabled(true)
	{ }

	FPGAGraphicsDataState(
		placement_state_tag,
		const std::vector<std::vector<device::AtomID>>& net_members,
		const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
		const std::unordered_map<device::AtomID, geom::Point<double>>& nonmoveable_block_locations,
		const std::unordered_map<device::AtomID, geom::Point<double>>& moveable_block_locations
	)
		: FPGAGraphicsDataState_Routing()
		, FPGAGraphicsDataState_Placement(
			net_members,
			fixed_block_locations,
			nonmoveable_block_locations,
			moveable_block_locations
		)
		, enabled(true)
	{ }

	FPGAGraphicsDataState(const FPGAGraphicsDataState&) = default;
	FPGAGraphicsDataState(FPGAGraphicsDataState&&) = default;

	FPGAGraphicsDataState& operator=(const FPGAGraphicsDataState&) = default;
	FPGAGraphicsDataState& operator=(FPGAGraphicsDataState&&) = default;

	bool isEnabled() { return enabled; }

private:
	friend class FPGAGraphicsDataStateScope;

	bool enabled;
};

class FPGAGraphicsDataStateScope {
public:
	FPGAGraphicsDataStateScope(FPGAGraphicsDataState* src)
		: src(src)
	{ }

	FPGAGraphicsDataStateScope(const FPGAGraphicsDataStateScope&) = delete;
	FPGAGraphicsDataStateScope& operator=(const FPGAGraphicsDataStateScope&) = delete;

	FPGAGraphicsDataStateScope(FPGAGraphicsDataStateScope&& other)
		: src(nullptr)
	{
		std::swap(src, other.src);
	}

	FPGAGraphicsDataStateScope& operator=(FPGAGraphicsDataStateScope&& rhs) {
		std::swap(src, rhs.src);
		return *this;
	}

	~FPGAGraphicsDataStateScope() {
		if (src) {
			src->enabled = false;
		}
	}
private:
	FPGAGraphicsDataState* src;
};

class Graphics;
namespace detail{
	template<typename>
	struct pushState_instantiator;
}

class FPGAGraphicsData {
public:
	FPGAGraphicsData()
		: state_stack()
		, state_owner_count()
	{
		state_stack.emplace_back(std::make_unique<FPGAGraphicsDataState>());
	}

	FPGAGraphicsData(const FPGAGraphicsData&) = delete;
	FPGAGraphicsData(FPGAGraphicsData&&) = default;

	FPGAGraphicsData& operator=(const FPGAGraphicsData&) = delete;
	FPGAGraphicsData& operator=(FPGAGraphicsData&&) = default;

	template<typename Device>
	FPGAGraphicsDataStateScope pushRoutingState(
		Device const* device,
		const std::vector<std::vector<device::RouteElementID>>& paths,
		const util::Netlist<device::RouteElementID>& netlist,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw,
		bool reset_view = false
	) {
		return pushRoutingState_base(device, paths, netlist, std::move(extra_colours_to_draw), reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushRoutingState(
		Device const* device,
		bool reset_view = false
	) {
		return pushRoutingState_base(device, {}, {}, {}, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushRoutingState(
		Device const* device,
		const std::vector<std::vector<device::RouteElementID>>& paths,
		bool reset_view = false
	) {
		return pushRoutingState_base(device, paths, {}, {}, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushRoutingState(
		Device const* device,
		const std::vector<std::vector<device::RouteElementID>>& paths,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw,
		bool reset_view = false
	) {
		return pushRoutingState_base(device, paths, {}, extra_colours_to_draw, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushRoutingState(
		Device const* device,
		const util::Netlist<device::RouteElementID>& netlist,
		bool reset_view = false
	) {
		return pushRoutingState_base(device, {}, netlist, {}, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushRoutingState(
		Device const* device,
		const util::Netlist<device::RouteElementID>& netlist,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw,
		bool reset_view = false
	) {
		return pushRoutingState_base(device, {}, netlist, extra_colours_to_draw, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushRoutingState(
		Device const* device,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw,
		bool reset_view = false
	) {
		return pushRoutingState_base(device, {}, {}, std::move(extra_colours_to_draw), reset_view);
	}

	FPGAGraphicsDataStateScope pushPlacingState(
		const std::vector<std::vector<device::AtomID>>& net_members,
		const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations,
		const std::unordered_map<device::AtomID, geom::Point<double>>& nonmoveable_block_locations,
		const std::unordered_map<device::AtomID, geom::Point<double>>& moveable_block_locations,
		bool reset_view = false
	) {
		return pushPlacingState_base(net_members, fixed_block_locations, nonmoveable_block_locations, moveable_block_locations, reset_view);
	}

private:
	friend class Graphics;
	template<typename> friend class detail::pushState_instantiator;

	template<typename Device>
	FPGAGraphicsDataStateScope pushRoutingState_base(
		Device const* device = nullptr,
		const std::vector<std::vector<device::RouteElementID>>& paths = {},
		const util::Netlist<device::RouteElementID>& netlist = {},
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw = {},
		bool reset_view = false
	) {
		state_stack.push_back(std::make_unique<FPGAGraphicsDataState>(FPGAGraphicsDataState::routing_state_tag{}, device, paths, netlist, std::move(extra_colours_to_draw)));
		do_graphics_refresh(reset_view, device->info().bounds);
		return FPGAGraphicsDataStateScope(state_stack.back().get());
	}

	FPGAGraphicsDataStateScope pushPlacingState_base(
		const std::vector<std::vector<device::AtomID>>& net_members = {},
		const std::unordered_map<device::AtomID, device::BlockID>& fixed_block_locations = {},
		const std::unordered_map<device::AtomID, geom::Point<double>>& nonmoveable_block_locations = {},
		const std::unordered_map<device::AtomID, geom::Point<double>>& moveable_block_locations = {},
		bool reset_view = false
	) {
		state_stack.push_back(std::make_unique<FPGAGraphicsDataState>(
			FPGAGraphicsDataState::placement_state_tag{},
			net_members,
			fixed_block_locations,
			nonmoveable_block_locations,
			moveable_block_locations
		));
		do_graphics_refresh(reset_view, geom::BoundBox<double>(0,0,1,1));
		return FPGAGraphicsDataStateScope(state_stack.back().get());
	}

	void do_graphics_refresh(bool reset_view, const geom::BoundBox<float>& fpga_bb);

	void drawAll();

	struct StateLock {
		int owner_count = 0;
		void lock() { owner_count += 1; }
		void unlock() { owner_count -= 1; }
	};

	std::pair<FPGAGraphicsDataState&, std::unique_lock<StateLock>> dataAndLock() {
		if (state_owner_count.owner_count == 0) {
			while (state_stack.size() > 1 && !state_stack.back()->isEnabled()) {
				state_stack.pop_back();
			}
		}
		return {*state_stack.back(), std::unique_lock<StateLock>(state_owner_count)};
	}

	std::vector<std::unique_ptr<FPGAGraphicsDataState>> state_stack;
	StateLock state_owner_count;
};

} // end namespace graphics

#endif // GRAPHICS__FPGA_GRAPHICS_DATA_H
