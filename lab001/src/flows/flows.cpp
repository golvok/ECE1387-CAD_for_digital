#include "flows.hpp"

#include <algo/routing.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <util/lambda_compose.hpp>
#include <util/logging.hpp>

#include <algorithm>

#include <boost/range/irange.hpp>

namespace flows {

template<typename Device>
class FlowBase {
public:
	FlowBase(const FlowBase&) = default;
	FlowBase(FlowBase&&) = default;

	FlowBase(
		const Device& dev,
		int nThreads
	)
		: dev(dev)
		, nThreads(nThreads)
	{ }

	FlowBase withDevice(const Device& newDev) const {
		return {
			newDev,
			nThreads
		};
	}

	const Device& dev;
	const int nThreads;
};

template<typename Device>
class FanoutTestFlow : public FlowBase<Device> {
public:
	using FlowBase<Device>::FlowBase;
	using FlowBase<Device>::dev;
	using FlowBase<Device>::nThreads;
	FanoutTestFlow(const FanoutTestFlow&) = default;
	FanoutTestFlow(FanoutTestFlow&&) = default;
	FanoutTestFlow(FlowBase<Device>&& fb) : FlowBase<Device>(std::move(fb)) { }
	FanoutTestFlow(const FlowBase<Device>& fb) : FlowBase<Device>(fb) { }

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
public:
	using FlowBase<Device>::FlowBase;
	using FlowBase<Device>::dev;
	using FlowBase<Device>::nThreads;
	RouteAsIsFlow(const RouteAsIsFlow&) = default;
	RouteAsIsFlow(RouteAsIsFlow&&) = default;
	RouteAsIsFlow(FlowBase<Device>&& fb) : FlowBase<Device>(std::move(fb)) { }
	RouteAsIsFlow(const FlowBase<Device>& fb) : FlowBase<Device>(fb) { }

	template<typename RouteTheseSourcesFirst = std::vector<device::PinGID>>
	auto flow_main(const util::Netlist<device::PinGID>& pin_to_pin_netlist, const RouteTheseSourcesFirst& route_these_sources_first = {}, bool present_graphics = true) const {
		const auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
			str << "RouteAsIs Flow ( track_width = " << this->dev.info().track_width << " )";
		});

		std::unordered_set<device::PinGID> in_route_these_first;
		std::vector<device::PinGID> net_order;
		for (const auto& pin : route_these_sources_first) {
			if (in_route_these_first.find(pin) == end(in_route_these_first)) {
				in_route_these_first.insert(pin);
				net_order.emplace_back(pin);
			}
		}

		std::copy_if(
			std::begin(pin_to_pin_netlist.roots()),
			std::end(pin_to_pin_netlist.roots()),
			std::back_inserter(net_order),
			[&](const device::PinGID& pin) {
				return in_route_these_first.find(pin) == std::end(in_route_these_first);
			}
		);

		const auto result = algo::route_all<false>(pin_to_pin_netlist, net_order, dev, nThreads);
		const auto num_REs = std::count_if(std::begin(result.netlist().all_ids()), std::end(result.netlist().all_ids()), [](const auto& reid) {
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
public:
	using FlowBase<Device>::FlowBase;
	using FlowBase<Device>::dev;
	using FlowBase<Device>::nThreads;
	RouteWithRetryFlow(const RouteWithRetryFlow&) = default;
	RouteWithRetryFlow(RouteWithRetryFlow&&) = default;
	RouteWithRetryFlow(FlowBase<Device>&& fb) : FlowBase<Device>(std::move(fb)) { }
	RouteWithRetryFlow(const FlowBase<Device>& fb) : FlowBase<Device>(fb) { }

	template<typename PinOrder>
	bool flow_main(
		const util::Netlist<device::PinGID>& pin_to_pin_netlist,
		const PinOrder& base_pin_order
	) const {
		const auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
			str << "RouteWithRetry Flow";
		});

		std::unordered_set<device::PinGID> in_route_these_sources_first;
		std::list<device::PinGID> route_these_sources_first;

		while (true) {
			std::vector<device::PinGID> source_order;
			std::copy(begin(route_these_sources_first), end(route_these_sources_first), std::back_inserter(source_order));
			for (const auto& source_and_sink : base_pin_order) {
				const auto& source = source_and_sink.first;
				if (in_route_these_sources_first.find(source) == end(in_route_these_sources_first)) {
					source_order.push_back(source);
				}
			}
			const auto result = RouteAsIsFlow<Device>(*this).flow_main(pin_to_pin_netlist, source_order, false);

			bool added_something = false;
			for (const auto& source : result.unroutedPins().all_ids()) {
				const bool already_there = in_route_these_sources_first.find(source) != std::end(in_route_these_sources_first);
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
public:
	using FlowBase<Device>::FlowBase;
	using FlowBase<Device>::dev;
	using FlowBase<Device>::nThreads;
	TrackWidthExplorationFlow(const TrackWidthExplorationFlow&) = default;
	TrackWidthExplorationFlow(TrackWidthExplorationFlow&&) = default;
	TrackWidthExplorationFlow(FlowBase<Device>&& fb) : FlowBase<Device>(std::move(fb)) { }
	TrackWidthExplorationFlow(const FlowBase<Device>& fb) : FlowBase<Device>(fb) { }

	void flow_main(
		const util::Netlist<device::PinGID>& pin_to_pin_netlist,
		const std::vector<std::pair<device::PinGID, device::PinGID>>& base_pin_order
	) const {
		const auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
			str << "TrackWidthExploration Flow";
		});

		std::unordered_map<int, bool> attempt_statuses;
		auto track_width_range = boost::irange(1, dev.info().track_width+1); // +1 as last argument is a past-end
		const auto SENTINEL = -1; // something note in the above range, specifically less than everything
		using std::begin; using std::end;
		std::binary_search(std::begin(track_width_range), std::end(track_width_range), SENTINEL, [&](const auto& rhs, const auto& lhs) {
			auto dev_info_copy = this->dev.info();
			dev_info_copy.track_width = std::max(rhs,lhs); // get the non-sentinel

			const auto route_success = [&](){
				const auto attempt_find_result = attempt_statuses.find(dev_info_copy.track_width);
				if (attempt_find_result == std::end(attempt_statuses)) {
					const auto indent_scope = dout(DL::INFO).indentWithTitle([&](auto&& str) {
						str << "Trying track width of " << dev_info_copy.track_width;
					});

					auto indent = dout(DL::INFO).indentWithTitle("Creating New Device");
					const Device modified_dev(dev_info_copy);
					dout(DL::INFO) << "done creating new device\n";
					indent.endIndent();

					const auto route_success = RouteWithRetryFlow<Device>(this->withDevice(modified_dev)).flow_main(pin_to_pin_netlist, base_pin_order);
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
		auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
			str << "Initial Device Creation";
		});

		const auto& dtype = dev_desc.type();

		if (dtype == device::DeviceType::Wilton) {
			return device::Device<device::WiltonConnector>(dev_desc);

		} else if (dtype == device::DeviceType::FullyConnected) {
			return device::Device<device::FullyConnectedConnector>(dev_desc);

		// } else if (dtype == device::DeviceType::Wilton_Cached) {
		// 	return device::Device<device::FanoutCachingConnector<device::WiltonConnector>>(dev_desc);

		// } else if (dtype == device::DeviceType::FullyConnected_Cached) {
		// 	return device::Device<device::FanoutCachingConnector<device::FullyConnectedConnector>>(dev_desc);

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

void fanout_test(
	const device::DeviceInfo& dev_desc,
	int nThreads
) {
	auto device_variant = make_device(dev_desc);
	apply_visitor(util::compose_withbase<boost::static_visitor<void>>([&](auto&& device) {
		FanoutTestFlow<std::decay_t<decltype(device)>> flow(device, nThreads);
		flow.flow_main();
	}), device_variant);
}

void track_width_exploration(
	const device::DeviceInfo& dev_desc,
	const util::Netlist<device::PinGID>& pin_to_pin_netlist,
	const std::vector<std::pair<device::PinGID, device::PinGID>>& base_pin_order,
	int nThreads
) {
	auto device_variant = make_device(dev_desc);
	apply_visitor(util::compose_withbase<boost::static_visitor<void>>([&](auto&& device) {
		TrackWidthExplorationFlow<std::decay_t<decltype(device)>> flow(device, nThreads);
		flow.flow_main(pin_to_pin_netlist, base_pin_order);
	}), device_variant);
}

void route_as_is(
	const device::DeviceInfo& dev_desc,
	const util::Netlist<device::PinGID>& pin_to_pin_netlist,
	const std::vector<std::pair<device::PinGID, device::PinGID>>& base_pin_order,
	int nThreads
) {
	auto device_variant = make_device(dev_desc);
	apply_visitor(util::compose_withbase<boost::static_visitor<void>>([&](auto&& device) {
		RouteAsIsFlow<std::decay_t<decltype(device)>> flow(device, nThreads);
		flow.flow_main(pin_to_pin_netlist, util::xrange_forward_pe<decltype(begin(base_pin_order))>(
			begin(base_pin_order),
			end(base_pin_order),
			[](auto& source_and_sink) { return source_and_sink->first; }
		));
	}), device_variant);
}

} // end namespace flows
