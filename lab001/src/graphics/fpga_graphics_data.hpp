#ifndef GRAPHICS__FPGA_GRAPHICS_DATA_H
#define GRAPHICS__FPGA_GRAPHICS_DATA_H

#include <device/connectors.hpp>
#include <device/device.hpp>
#include <graphics/graphics_types.hpp>

#include <mutex>
#include <vector>

namespace graphics {

class FPGAGraphicsData;

class FPGAGraphicsDataState {
public:
	FPGAGraphicsDataState()
		: enabled(true)
		, fc_dev(nullptr)
		, paths()
		, extra_colours_to_draw()
	{ }

	FPGAGraphicsDataState(
		device::Device<device::FullyConnectedConnector> const* fc_dev,
		const std::vector<std::vector<device::RouteElementID>>& paths = {},
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw = {}
	)
		: enabled(true)
		, fc_dev(fc_dev)
		, paths(paths)
		, extra_colours_to_draw(std::move(extra_colours_to_draw))
	{ }

	FPGAGraphicsDataState(const FPGAGraphicsDataState&) = default;
	FPGAGraphicsDataState(FPGAGraphicsDataState&&) = default;

	FPGAGraphicsDataState& operator=(const FPGAGraphicsDataState&) = default;
	FPGAGraphicsDataState& operator=(FPGAGraphicsDataState&&) = default;

	      auto& getPaths()       { return paths; }
	const auto& getPaths() const { return paths; }

	      auto& getFCDev()       { return fc_dev; }
	const auto& getFCDev() const { return fc_dev; }

	      auto& getExtraColours()       { return extra_colours_to_draw; }
	const auto& getExtraColours() const { return extra_colours_to_draw; }

	bool isEnabled() { return enabled; }

private:
	friend class FPGAGraphicsDataStateScope;

	bool enabled;
	device::Device<device::FullyConnectedConnector> const* fc_dev;
	std::vector<std::vector<device::RouteElementID>> paths;
	std::unordered_map<device::RouteElementID, graphics::t_color> extra_colours_to_draw;
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

	FPGAGraphicsDataStateScope pushState(
		device::Device<device::FullyConnectedConnector> const* fc_dev = nullptr,
		const std::vector<std::vector<device::RouteElementID>>& paths = {},
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw = {}
	);

	FPGAGraphicsDataStateScope pushState(
		device::Device<device::FullyConnectedConnector> const* fc_dev,
		std::unordered_map<device::RouteElementID, graphics::t_color>&& extra_colours_to_draw
	);

private:
	friend class Graphics;
	void drawAll();

	struct StateLock {
		int owner_count = 0;
		void lock() { owner_count += 1; }
		void unlock() { owner_count -= 1; }
	};

	std::pair<FPGAGraphicsDataState&, std::unique_lock<StateLock>> dataAndLock() {
		if (!state_owner_count.owner_count == 0) {
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
