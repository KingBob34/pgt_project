#include <iostream>
#include <stdexcept>
#include "loader.h"
#include "engine.h"
#include "story.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: logic_demo <story-file.json>\n";
        return 1;
    }
    try
    {
        Story story = loadStory(argv[1]);
        runStory(story);
    } catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
