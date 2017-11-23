#ifndef GRAPHICS__SAT_MAXER_GRAPHICS_DATA_H
#define GRAPHICS__SAT_MAXER_GRAPHICS_DATA_H

#include <datastructures/literal_id.hpp>

#include <vector>

#include <boost/lockfree/queue.hpp>

struct SatMaxerGraphicsData {
	SatMaxerGraphicsData();

	void addPath(std::vector<Literal>&& path);

	void drawAll();
private:
	boost::lockfree::queue<std::vector<Literal>*> paths_to_draw;
};

#endif /* GRAPHICS__SAT_MAXER_GRAPHICS_DATA_H */
