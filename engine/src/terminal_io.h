#ifndef TERMINAL_IO_H
#define TERMINAL_IO_H
#include <string>
#include <vector>

// One option as shown to the player: a label, and whether it can be picked
struct DisplayOption
{
    std::string label;   // empty when locked
    bool available = true;
};

// Show a passage's text, then its options
void showPassage(const std::string& text, const std::vector<DisplayOption>& options);

// Print one prompt, read a whole line of raw input, return it
std::string readLine(const std::string& prompt);

// Print a message to the player (command replies, error hints)
void showMessage(const std::string& text);

#endif //TERMINAL_IO_H
