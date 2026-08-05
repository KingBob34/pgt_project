#include "terminal_io.h"
#include <iostream>

void showPassage(const std::string& text, const std::vector<DisplayOption>& options)
{
    std::cout << "\n" << text << "\n\n";
    int number = 1;
    for (const DisplayOption& option : options)
    {
        if (option.available)
        {
            std::cout << "  " << number << ") " << option.label << "\n";
        } else
        {
            std::cout << "  " << number << ") [LOCKED]\n";
        }
        number ++;
    }
}

std::string readLine(const std::string& prompt)
{
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void showMessage(const std::string& text)
{
    std::cout << text << "\n";
}
