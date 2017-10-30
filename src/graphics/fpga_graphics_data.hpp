#ifndef GRAPHICS__FPGA_GRAPHICS_DATA_H
#define GRAPHICS__FPGA_GRAPHICS_DATA_H

#include <device/connectors.hpp>
#include <device/device.hpp>
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

}

class FPGAGraphicsDataState : public detail::FPGAGraphicsDataState_Routing {
	using Netlist = util::Netlist<device::RouteElementID>;
public:
	FPGAGraphicsDataState()
		: FPGAGraphicsDataState_Routing()
		, enabled(true)
	{ }

	template<typename Device>
	FPGAGraphicsDataState(
		Device const* device,
		const std::vector<std::vector<device::RouteElementID>>& paths = {},
		const Netlist& netlist = {},
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw = {}
	)
		: FPGAGraphicsDataState_Routing(
			device,
			paths,
			netlist,
			std::move(extra_colours_to_draw)
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
	FPGAGraphicsDataStateScope pushState(
		Device const* device,
		const std::vector<std::vector<device::RouteElementID>>& paths,
		const util::Netlist<device::RouteElementID>& netlist,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw,
		bool reset_view = false
	) {
		return pushState_base(device, paths, netlist, std::move(extra_colours_to_draw), reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushState(
		Device const* device,
		bool reset_view = false
	) {
		return pushState_base(device, {}, {}, {}, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushState(
		Device const* device,
		const std::vector<std::vector<device::RouteElementID>>& paths,
		bool reset_view = false
	) {
		return pushState_base(device, paths, {}, {}, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushState(
		Device const* device,
		const std::vector<std::vector<device::RouteElementID>>& paths,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw,
		bool reset_view = false
	) {
		return pushState_base(device, paths, {}, extra_colours_to_draw, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushState(
		Device const* device,
		const util::Netlist<device::RouteElementID>& netlist,
		bool reset_view = false
	) {
		return pushState_base(device, {}, netlist, {}, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushState(
		Device const* device,
		const util::Netlist<device::RouteElementID>& netlist,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw,
		bool reset_view = false
	) {
		return pushState_base(device, {}, netlist, extra_colours_to_draw, reset_view);
	}

	template<typename Device>
	FPGAGraphicsDataStateScope pushState(
		Device const* device,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw,
		bool reset_view = false
	) {
		return pushState_base(device, {}, {}, std::move(extra_colours_to_draw), reset_view);
	}

private:
	friend class Graphics;
	template<typename> friend class detail::pushState_instantiator;

	template<typename Device>
	FPGAGraphicsDataStateScope pushState_base(
		Device const* device = nullptr,
		const std::vector<std::vector<device::RouteElementID>>& paths = {},
		const util::Netlist<device::RouteElementID>& netlist = {},
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw = {},
		bool reset_view = false
	) {
		state_stack.push_back(std::make_unique<FPGAGraphicsDataState>(device, paths, netlist, std::move(extra_colours_to_draw)));
		do_graphics_refresh(reset_view, device->info().bounds);
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
