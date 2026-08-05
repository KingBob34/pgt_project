#ifndef LOADER_H
#define LOADER_H
#include <string>
#include "story.h"

// Load a story from a JSON file.
Story loadStory(const std::string& path);

#endif //LOADER_H
