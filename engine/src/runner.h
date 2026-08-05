#ifndef RUNNER_H
#define RUNNER_H
#include <string>
#include <vector>
#include "game_state.h"
#include "scene.h"
#include "node_result.h"

// What the runner needs from the frontend before it can carry on
struct Step
{
    enum class Kind
    {
        ShowText,   // show text, then call advance() again
        AskChoice,   // show options, then call choose()
        Finished   // an end node was reached
    };
    Kind kind = Kind::Finished;
    std::string text;   // ShowText
    std::vector<DisplayOption> options;   // AskChoice
    std::string outcome;   // Finished
};

// One playthrough in progress: which scene, which node, and the current state
struct Runner
{
    explicit Runner(const Scene& scene);

    // run nodes until there is something for the player to see or do
    [[nodiscard]] Step advance();

    // answers the AskChoice that advance() returned last
    void choose(int index);
    const Scene& scene;
    GameState state;

    // output pins of the available options, in the order they were shown
    std::vector<std::string> offeredPins;
};

#endif //RUNNER_H
