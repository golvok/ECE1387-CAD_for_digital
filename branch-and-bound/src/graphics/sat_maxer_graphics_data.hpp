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
private:
	static const int QUEUE_SIZE = 65534; // this appears to be the maximum size allowed
	boost::lockfree::queue<std::vector<Literal>*, boost::lockfree::capacity<QUEUE_SIZE>> paths_to_draw;
	int num_levels_visible;
	float width;
	float level_step;
};

#endif /* GRAPHICS__SAT_MAXER_GRAPHICS_DATA_H */
