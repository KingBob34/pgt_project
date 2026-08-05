#ifndef TERMINAL_H
#define TERMINAL_H
#include <string>
#include <vector>
#include <optional>
#include "runner.h"

// Show one block of story prose
void showText(const std::string& text);

// Show the options the player can pick from
void showOptions(const std::vector<DisplayOption>& options);

// Print one prompt and read one whole line
std::optional<std::string> readLine(const std::string& prompt);

// Print a message (command replies, errors, etc.)
void showMessage(const std::string& text);

// Play a scene in the terminal until it ends or input runs out (EOF)
void runInTerminal(Runner& runner);

#endif //TERMINAL_H
