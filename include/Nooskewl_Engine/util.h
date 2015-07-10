#ifndef UTIL_H
#define UTIL_H

#include "Nooskewl_Engine/main.h"
#include "Nooskewl_Engine/error.h"

namespace Nooskewl_Engine {

class NOOSKEWL_EXPORT List_Directory {
public:
	List_Directory(std::string glob);
	~List_Directory();

	std::string next();

private:
#ifdef _MSC_VER
	bool got_first;
	bool done;
	HANDLE handle;
	WIN32_FIND_DATA ffd;
#endif
};

NOOSKEWL_EXPORT int SDL_fgetc(SDL_RWops *file);
NOOSKEWL_EXPORT char *SDL_fgets(SDL_RWops *file, char * const buf, size_t max);
NOOSKEWL_EXPORT int SDL_fputs(const char *string, SDL_RWops *file);

NOOSKEWL_EXPORT SDL_RWops *open_file(std::string filename) throw (Error);
NOOSKEWL_EXPORT std::string itos(int i);
NOOSKEWL_EXPORT bool check_args(int argc, char **argv, std::string arg);

} // End namespace Nooskewl_Engine

#ifdef NOOSKEWL_ENGINE_BUILD
using namespace Nooskewl_Engine;
#endif

#endif // UTIL_H