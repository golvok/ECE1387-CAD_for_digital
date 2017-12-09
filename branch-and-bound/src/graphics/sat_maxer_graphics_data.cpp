#include "sat_maxer_graphics_data.hpp"

#include <thread>

#include <graphics/graphics.hpp>
#include <util/logging.hpp>

namespace {
	template<typename Collection>
	auto& find_or_insert(Collection& c, const Literal& lit) {
		const auto search_result = std::find_if(begin(c), end(c), [&](auto& elem) {
			return elem->lit == lit;
		});

		if (search_result == end(c)) {
			c.emplace_back(std::make_unique<SatMaxerGraphicsData::TreeNode>(lit));
			return c.back();
		} else {
			return *search_result;
		}
	}
}

SatMaxerGraphicsData::SatMaxerGraphicsData()
	: explored_tree()
	, num_levels_visible(-1)
	, width(2048)
	, level_step(10)
{ }

void SatMaxerGraphicsData::setNumLevelsVisible(int num) {
	num_levels_visible = num;
	graphics::set_visible_world(-width/2, 0, width/2, level_step*(float)num_levels_visible);
}

void SatMaxerGraphicsData::addPath(std::vector<Literal>&& path) {
	if (path.empty()) {
		return;
	} else {
		if (path.size() == 1) {
			find_or_insert(explored_tree.children, path.front());
		} else {
			auto* curr_node = &explored_tree;
			for (const auto& lit : path) {
				curr_node = find_or_insert(curr_node->children, lit).get();
			}
		}
	}
}

void SatMaxerGraphicsData::drawAll() {
	using graphics::t_point;
	using child_list = std::vector<std::unique_ptr<TreeNode>>;
	using child_list_iter = child_list::const_iterator;

	struct State {
		child_list_iter curr;
		child_list_iter end;
		float curr_width;
		t_point parent_pos;
	};

	std::vector<State> state_stack;
	state_stack.push_back(State{
		begin(explored_tree.children),
		end(explored_tree.children),
		width/2,
		t_point(0,0)
	});

	while (!state_stack.empty()) {
		auto& state = state_stack.back();
		auto& current_node = **state.curr;

		auto xoffset = (state.curr_width/2) * (current_node.lit.inverted() ? -1 : 1);
		t_point curr = state.parent_pos + t_point(xoffset, level_step);

		graphics::drawline(state.parent_pos,curr);

		// dout(DL::INFO) << "draw " << state.parent_pos << " -> " << curr << '\n';

		if (current_node.children.empty()) {
			while (!state_stack.empty()) {
				auto next_after_pop = std::next(state_stack.back().curr);
				if (next_after_pop == state_stack.back().end) {
					state_stack.pop_back();
				} else {
					state_stack.back().curr = next_after_pop;
					break;
				}
			}
		} else {
			state_stack.push_back(State{
				begin(current_node.children),
				end(current_node.children),
				state.curr_width/2,
				curr
			});
		}
	}
}
