#ifndef MAP_LOGIC_H
#define MAP_LOGIC_H

#include "Nooskewl_Engine/main.h"

namespace Nooskewl_Engine {

class Map;
class Map_Entity;

class NOOSKEWL_EXPORT Map_Logic {
public:
	virtual void start(Map *map);
	virtual void end(Map *map);
	virtual void trigger(Map *map, Map_Entity *entity);
	virtual void update(Map *map);
};

} // End namespace Nooskewl_Engine

#ifdef NOOSKEWL_ENGINE_BUILD
using namespace Nooskewl_Engine;
#endif

#endif // MAP_LOGIC_H