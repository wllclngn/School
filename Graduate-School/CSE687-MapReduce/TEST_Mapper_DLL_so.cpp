#include "Mapper_DLL_so.h"
#include <iostream>
#include <vector>
#include <string>

int main()
{
    Mapper mapper;

    std::vector<std::string> lines = {
        "This is a test line.",
        "Another line, with more words!",
        "Yet another line to map and clean."
    };

    // Ensure cross-platform path handling
    #ifdef _WIN32
    std::string tempFolderPath = ".\\temp";
    #else
    std::string tempFolderPath = "./temp";
    #endif

    mapper.map_words(lines, tempFolderPath);

    std::cout << "Mapping completed. Check the temp folder for output." << std::endl;

    return 0;
}