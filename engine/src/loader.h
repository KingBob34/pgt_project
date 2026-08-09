#ifndef LOADER_H
#define LOADER_H
#include <string>
#include <vector>
#include "scene.h"

// Reads a scene file and returns it parsed, resolved and validated
Scene loadScene(const std::string& path);

// Lists everything that stops this scene from being played
// e.g. an output pin with no wire
std::vector<std::string> findProblems(const Scene& scene);

#endif //LOADER_H
