#ifndef GRAPHICS__SAT_MAXER_GRAPHICS_DATA_H
#define GRAPHICS__SAT_MAXER_GRAPHICS_DATA_H

#include <datastructures/literal_id.hpp>

#include <vector>

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/policies.hpp>

struct SatMaxerGraphicsData {
	SatMaxerGraphicsData();

	void setNumLevelsVisible(int num);

	void addPath(std::vector<Literal>&& path);

	void drawAll();

	struct TreeNode {
		TreeNode(Literal lit) : lit(lit), children() { }
		TreeNode() : lit(), children() { }

		Literal lit;
		std::vector<std::unique_ptr<TreeNode>> children;
	};
private:
	TreeNode explored_tree;
	int num_levels_visible;
	float width;
	float level_step;
};

#endif /* GRAPHICS__SAT_MAXER_GRAPHICS_DATA_H */
