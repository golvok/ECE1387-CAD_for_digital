#include "flows.hpp"

#include <algo/routing.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <util/logging.hpp>

namespace flows {

template<typename Device>
class FlowBase {
public:
	FlowBase(const Device& dev)
		: dev(dev)
	{ }

	const Device& dev;
};

template<typename Device>
class FanoutTestFlow : public FlowBase<Device> {
	using FlowBase<Device>::FlowBase;
	using FlowBase<Device>::dev;
public:
	void flow_main() const {
		bool do_view_reset = true;
		for (int i = 0; i < dev.info().track_width*2; ++i) {
			device::RouteElementID test_re(
				util::make_id<device::XID>((int16_t)2),
				util::make_id<device::YID>((int16_t)4),
				(int16_t)i
			);

			std::unordered_map<device::RouteElementID, graphics::t_color> colours;
			colours[test_re] = graphics::t_color(0,0xFF,0);

			dout(DL::INFO) << test_re << '\n';
			for (const auto& fanout : dev.fanout(test_re)) {
				colours[fanout] = graphics::t_color(0xFF,0,0);
				dout(DL::INFO) << '\t' << fanout << '\n';
			}

			const auto gfx_state_keeper = graphics::get().fpga().pushState(&dev, std::move(colours), do_view_reset);
			do_view_reset = false;
			graphics::get().waitForPress();
		}
	}
};

template<typename Device>
class RouteAsIsFlow : public FlowBase<Device> {
	using FlowBase<Device>::FlowBase;
	using FlowBase<Device>::dev;
public:
	void flow_main(const util::Netlist<device::PinGID>& pin_to_pin_netlist) const {
		const auto result = algo::route_all(pin_to_pin_netlist, pin_to_pin_netlist.roots(), dev);
		for (const auto& source : result.unroutedPins().all_ids()) {
			for (const auto& sink : result.unroutedPins().fanout(source)) {
				dout(DL::WARN) << "failed to route " << source << " -> " << sink << '\n';
			}
		}

		const auto gfx_state_keeper_final_routes = graphics::get().fpga().pushState(&dev, result.netlist());
		graphics::get().waitForPress();
	}
};

namespace {
	using DeviceVariant = boost::variant<ALL_DEVICES_COMMA_SEP>;

	DeviceVariant make_device(const device::DeviceInfo& dev_desc) {
		device::WiltonConnector connector(dev_desc);
		return device::Device<device::WiltonConnector>(dev_desc, connector);
	}
}

void fanout_test(const device::DeviceInfo& dev_desc) {
	auto device_variant = make_device(dev_desc);
	apply_visitor([](auto&& device) {
		FanoutTestFlow<std::remove_reference_t<decltype(device)>> flow(device);
		flow.flow_main();
	}, device_variant);
}

void track_width_exploration(const device::DeviceInfo& dev_desc) {
	(void)dev_desc;
}

void route_as_is(const device::DeviceInfo& dev_desc, const util::Netlist<device::PinGID>& pin_to_pin_netlist) {
	auto device_variant = make_device(dev_desc);
	apply_visitor([&](auto& device) {
		RouteAsIsFlow<std::remove_reference_t<decltype(device)>> flow(device);
		flow.flow_main(pin_to_pin_netlist);
	}, device_variant);
}

} // end namespace flows