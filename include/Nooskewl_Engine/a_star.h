#ifndef A_STAR_H
#define A_STAR_H

#include "Nooskewl_Engine/main.h"
#include "Nooskewl_Engine/basic_types.h"

namespace Nooskewl_Engine {

class Map;

class A_Star {
public:
	struct Node {
		Node *parent;
		Point<int> position;
		int cost_from_start;
		int cost_to_goal;
		int total_cost;
	};

	A_Star(Map *map);
	~A_Star();

	std::list<Node *> find_path(Point<int> start, Point<int> goal, bool check_solids = true);

private:
	Node *find_in_list(Point<int> position, std::list<Node *> &list);
	void remove_from_list(Node *node, std::list<Node *> &list);
	void add_to_open(Node *node);
	void destroy_nodes(std::list<Node *> &list);
	void branch(Node *node, Point<int> offset, Point<int> goal, bool check_solids = true);
	int heuristic(Point<int> start, Point<int> end);

	Map *map;
	Node *final_node;

	std::list<Node *> open;
	std::list<Node *> closed;
};

} // End namespace Nooskewl_Engine

#endif // A_STAR_H
