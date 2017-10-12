#include "flows.hpp"

#include <algo/routing.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <util/logging.hpp>

#include <algorithm>

#include <boost/range/irange.hpp>

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
	template<typename RouteTheseSourcesFirst = std::vector<device::PinGID>>
	auto flow_main(const util::Netlist<device::PinGID>& pin_to_pin_netlist, RouteTheseSourcesFirst route_these_sources_first = {}, bool present_graphics = true) const {
		const auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
			str << "RouteAsIs Flow ( track_width = " << this->dev.info().track_width << " )";
		});

		std::unordered_set<device::PinGID> in_route_these_first;
		std::vector<device::PinGID> net_order;
		for (const auto& pin : route_these_sources_first) {
			in_route_these_first.insert(pin);
			net_order.emplace_back(pin);
		}

		std::copy_if(
			begin(pin_to_pin_netlist.roots()),
			end(pin_to_pin_netlist.roots()),
			std::back_inserter(net_order),
			[&](const device::PinGID& pin) {
				return in_route_these_first.find(pin) == end(in_route_these_first);
			}
		);

		const auto result = algo::route_all<false>(pin_to_pin_netlist, net_order, dev);
		const auto num_REs = std::count_if(begin(result.netlist().all_ids()), end(result.netlist().all_ids()), [](const auto& reid) {
			return !reid.isPin();
		});

		dout(DL::INFO) << "routing attempt finished. Used " << num_REs << " routing resources.\n";

		for (const auto& source : result.unroutedPins().all_ids()) {
			for (const auto& sink : result.unroutedPins().fanout(source)) {
				dout(DL::INFO) << "failed to route " << source << " -> " << sink << '\n';
			}
		}

		if (present_graphics) {
			const auto gfx_state_keeper_final_routes = graphics::get().fpga().pushState(&dev, result.netlist());
			graphics::get().waitForPress();
		}

		return result;
	}
};

template<typename Device>
class RouteWithRetryFlow : public FlowBase<Device> {
	using FlowBase<Device>::FlowBase;
	using FlowBase<Device>::dev;
public:
	bool flow_main(const util::Netlist<device::PinGID>& pin_to_pin_netlist) const {
		const auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
			str << "RouteWithRetry Flow";
		});

		std::unordered_set<device::PinGID> in_route_these_sources_first;
		std::list<device::PinGID> route_these_sources_first;

		while (true) {
			const auto result = RouteAsIsFlow<Device>(dev).flow_main(pin_to_pin_netlist, route_these_sources_first, false);

			bool added_something = false;
			for (const auto& source : result.unroutedPins().all_ids()) {
				const bool already_there = in_route_these_sources_first.find(source) != end(in_route_these_sources_first);
				const bool has_fanout = !util::empty(result.unroutedPins().fanout(source));
				if (!already_there && has_fanout) {
					dout(DL::INFO) << "new failure on : " << source << '\n';
					route_these_sources_first.push_back(source);
					in_route_these_sources_first.insert(source);
					added_something = true;
				}
			}


			if (result.unroutedPins().empty()) {
				const auto gfx_state_keeper_final_routes = graphics::get().fpga().pushState(&dev, result.netlist());
				graphics::get().waitForPress();
				return true;
			} else if (!added_something) {
				dout(DL::INFO) << "Failed to route the same nets. Giving up.\n";
				const auto gfx_state_keeper_final_routes = graphics::get().fpga().pushState(&dev, result.netlist());
				graphics::get().waitForPress();
				return false;
			}
		}
	}
};

template<typename Device>
class TrackWidthExplorationFlow : public FlowBase<Device> {
	using FlowBase<Device>::FlowBase;
	using FlowBase<Device>::dev;
public:
	void flow_main(const util::Netlist<device::PinGID>& pin_to_pin_netlist) const {
		const auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
			str << "TrackWidthExploration Flow";
		});

		std::unordered_map<int, bool> attempt_statuses;
		auto track_width_range = boost::irange(1, dev.info().track_width+1); // +1 as last argument is a past-end
		const auto SENTINEL = -1; // something note in the above range, specifically less than everything
		using std::begin; using std::end;
		std::binary_search(begin(track_width_range), end(track_width_range), SENTINEL, [&](const auto& rhs, const auto& lhs) {
			auto dev_info_copy = this->dev.info();
			dev_info_copy.track_width = std::max(rhs,lhs); // get the non-sentinel

			const auto route_success = [&](){
				const auto attempt_find_result = attempt_statuses.find(dev_info_copy.track_width);
				if (attempt_find_result == end(attempt_statuses)) {
					const auto indent_scope = dout(DL::INFO).indentWithTitle([&](auto&& str) {
						str << "Trying track width of " << dev_info_copy.track_width;
					});

					const Device modified_dev(dev_info_copy);
					const auto route_success = RouteWithRetryFlow<Device>(modified_dev).flow_main(pin_to_pin_netlist);
					// const auto route_success = RouteAsIsFlow<Device>(modified_dev).flow_main(pin_to_pin_netlist).unroutedPins().empty();

					if (route_success) {
						dout(DL::INFO) << "Circuit successfully routed with track width of " << modified_dev.info().track_width << '\n';
					} else {
						dout(DL::INFO) << "Circuit FAILED to route with track width of " << modified_dev.info().track_width << '\n';
					}
					return route_success;
				} else {
					const auto route_success = attempt_find_result->second;

					if (route_success) {
						dout(DL::INFO) << "Circuit already successfully routed with track width of " << attempt_find_result->first << '\n';
					} else {
						dout(DL::INFO) << "Circuit ALREADY FAILED to route with track width of " << attempt_find_result->first << '\n';
					}
					return route_success;
				}
			}();

			attempt_statuses[dev_info_copy.track_width] = route_success;

			if (rhs == SENTINEL) {
				return route_success;
			} else {
				return !route_success;
			}
		});
	}
};

namespace {
	using DeviceVariant = boost::variant<ALL_DEVICES_COMMA_SEP>;

	DeviceVariant make_device(const device::DeviceInfo& dev_desc) {
		const auto& dtype = dev_desc.type();

		if (dtype == device::DeviceType::Wilton) {
			return device::Device<device::WiltonConnector>(dev_desc);

		} else if (dtype == device::DeviceType::FullyConnected) {
			return device::Device<device::FullyConnectedConnector>(dev_desc);

		} else if (dtype == device::DeviceType::Wilton_Cached) {
			return device::Device<device::FanoutCachingConnector<device::WiltonConnector>>(dev_desc);

		} else if (dtype == device::DeviceType::FullyConnected_Cached) {
			return device::Device<device::FanoutCachingConnector<device::FullyConnectedConnector>>(dev_desc);

		} else if (dtype == device::DeviceType::Wilton_PreCached) {
			return device::Device<device::FanoutPreCachingConnector<device::WiltonConnector>>(dev_desc);

		} else if (dtype == device::DeviceType::FullyConnected_PreCached) {
			return device::Device<device::FanoutPreCachingConnector<device::FullyConnectedConnector>>(dev_desc);

		} else {
			util::print_and_throw<std::runtime_error>([&](auto&& str) {
				str << "don't understand device type " << dtype.getValue();
			});
			throw "impossible";
		}
	}
}

void fanout_test(const device::DeviceInfo& dev_desc) {
	auto device_variant = make_device(dev_desc);
	apply_visitor([](auto&& device) {
		FanoutTestFlow<std::remove_reference_t<decltype(device)>> flow(device);
		flow.flow_main();
	}, device_variant);
}

void track_width_exploration(const device::DeviceInfo& dev_desc, const util::Netlist<device::PinGID>& pin_to_pin_netlist) {
	auto device_variant = make_device(dev_desc);
	apply_visitor([&](auto& device) {
		TrackWidthExplorationFlow<std::remove_reference_t<decltype(device)>> flow(device);
		flow.flow_main(pin_to_pin_netlist);
	}, device_variant);
}

void route_as_is(const device::DeviceInfo& dev_desc, const util::Netlist<device::PinGID>& pin_to_pin_netlist) {
	auto device_variant = make_device(dev_desc);
	apply_visitor([&](auto& device) {
		RouteAsIsFlow<std::remove_reference_t<decltype(device)>> flow(device);
		flow.flow_main(pin_to_pin_netlist);
	}, device_variant);
}

} // end namespace flows
