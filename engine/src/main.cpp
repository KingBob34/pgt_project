#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "loader.h"
#include "runner.h"
#include "frontends/terminal.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: logic_demo <scene-file.json>\n";
        return 1;
    }
    try
    {
        const Scene scene = loadScene(argv[1]);
        const std::vector<std::string> problems = findProblems(scene);
        if (!problems.empty())
        {
            std::cerr << "This scene cannot be played yet:\n";
            for (const std::string& problem : problems)
            {
                std::cerr << "  - " << problem << "\n";
            }
            return 1;
        }
        Runner runner(scene);
        runInTerminal(runner);
    } catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}