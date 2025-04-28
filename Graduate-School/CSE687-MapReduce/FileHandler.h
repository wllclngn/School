#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "ERROR_Handler.h"
#include "Logger.h"

/*
// CALLS FOR IF DYNAMIC VALIDATE DIRECTORY IS USED
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif
*/

namespace fs = std::filesystem;

class FileHandler {
public:
    static bool read_file(const std::string &filename, std::vector<std::string> &lines) {
        std::ifstream file(filename);
        if (!file) {
            ErrorHandler::reportError("Could not open file " + filename + " for reading.");
            return false;
        }
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        return true;
    }

    static bool validate_directory(std::string &folder_path, bool create_if_missing = true) {
        Logger &logger = Logger::getInstance();
        std::vector<std::string> directory_history;
        logger.log("Starting directory validation process.");
    
        // Check if the directory exists
        if (fs::exists(folder_path)) {
            if (fs::is_directory(folder_path)) {
                logger.log("Validated directory: " + folder_path);
                std::cout << "LOG: The directory " << folder_path << " already exists. Proceeding..." << std::endl;
                return true; // Directory exists and is valid
            } else {
                std::cerr << "WARNING: A file with the same name already exists, but it is not a directory." << std::endl;
                logger.log("ERROR: Path exists but is not a directory: " + folder_path);
                return false; // Path exists but is not a directory
            }
        }
    
        // Directory does not exist
        if (create_if_missing) {
            try {
                if (fs::create_directory(folder_path)) {
                    std::cout << "LOG: Directory " << folder_path << " created successfully. Proceeding..." << std::endl;
                    logger.log("Directory created successfully: " + folder_path);
                    return true; // Directory created successfully
                } else {
                    std::cerr << "WARNING: Failed to create the directory." << std::endl;
                    logger.log("ERROR: Failed to create directory: " + folder_path);
                    return false; // Failed to create directory
                }
            } catch (const fs::filesystem_error &e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
                logger.log("Filesystem error: " + std::string(e.what()));
                return false; // Filesystem error occurred
            }
        } else {
            std::cout << "ERROR: Directory " << folder_path << " does not exist and creation is disabled." << std::endl;
            logger.log("Directory does not exist and creation is disabled: " + folder_path);
            return false; // Directory does not exist and creation is not allowed
        }
    }

        /*

        // VALIDATE_DIRECTORY FOR WINDOWS AND UNIX-LIKE DYNAMIC DIRECTORY INPUTS, ETC.
            static bool validate_directory(std::string &folder_path) {
        Logger &logger = Logger::getInstance();
        std::vector<std::string> directory_history;
        logger.log("Starting directory validation process.");

#ifdef _WIN32
        // Windows-specific input handling
        while (true) {
            if (fs::exists(folder_path) && fs::is_directory(folder_path)) {
                logger.log("Validated directory: " + folder_path);
                return true; // Valid directory
            }

            // Log the error and prompt the user
            ErrorHandler::reportError("Directory " + folder_path + " does not exist or is not valid.");
            logger.log("Invalid directory: " + folder_path);

            std::cout << "Please enter a valid directory path (or type 'EXIT' to quit): ";
            char ch;
            folder_path.clear();

            while (true) {
                ch = _getch();
                if (ch == '\r') { // Enter key
                    std::cout << "\n";
                    break;
                } else if (ch == '\b' && !folder_path.empty()) { // Backspace
                    folder_path.pop_back();
                    std::cout << "\b \b";
                } else if (isprint(ch)) { // Printable character
                    folder_path += ch;
                    std::cout << ch;
                }
            }

            if (folder_path == "EXIT") {
                logger.log("User exited the directory validation process.");
                std::cout << "Program terminated by user.\n";
                return false;
            }

            directory_history.push_back(folder_path);
            logger.log("User provided new directory path: " + folder_path);
        }
#else
        // Unix-like systems with readline support
        rl_attempted_completion_function = [](const char *text, int start, int end) -> char ** {
            if (start != 0) return nullptr;

            std::vector<std::string> matches;
            try {
                for (const auto &entry : fs::directory_iterator(fs::current_path())) {
                    std::string entry_name = entry.path().filename().string();
                    if (entry_name.rfind(text, 0) == 0) { // Starts with "text"
                        matches.push_back(entry.path().string());
                    }
                }
            } catch (const fs::filesystem_error &e) {
                std::cerr << "[ERROR] Filesystem error: " << e.what() << "\n";
            }

            char **completion_list = new char *[matches.size() + 1];
            for (size_t i = 0; i < matches.size(); ++i) {
                completion_list[i] = strdup(matches[i].c_str());
            }
            completion_list[matches.size()] = nullptr;
            return completion_list;
        };

        while (true) {
            if (fs::exists(folder_path) && fs::is_directory(folder_path)) {
                logger.log("Validated directory: " + folder_path);
                return true; // Valid directory
            }

            ErrorHandler::reportError("Directory " + folder_path + " does not exist or is not valid.");
            logger.log("Invalid directory: " + folder_path);

            char *input = readline("Please enter a valid directory path (or type 'EXIT' to quit): ");
            if (!input) {
                std::cerr << "Error reading input.\n";
                return false;
            }

            folder_path = input;
            free(input);

            if (folder_path == "EXIT") {
                logger.log("User exited the directory validation process.");
                std::cout << "Program terminated by user.\n";
                return false;
            }

            directory_history.push_back(folder_path);
            logger.log("User provided new directory path: " + folder_path);
        }
#endif
    }
        */

    static bool write_filenames_to_file(const std::string &folder_path, const std::string &output_filename) {
        std::ofstream outfile(output_filename);
        if (!outfile) {
            ErrorHandler::reportError("Could not open " + output_filename + " for writing.");
            return false;
        }
        for (const auto &entry : fs::directory_iterator(folder_path)) {
            if (entry.is_regular_file()) {
                outfile << entry.path().filename().string() << "\n";
            }
        }
        outfile.close();
        return true;
    }

    static bool write_output(const std::string &filename, const std::map<std::string, int> &data) {
        std::ofstream file(filename);
        if (!file) {
            ErrorHandler::reportError("Could not open file " + filename + " for writing.");
            return false;
        }
        for (const auto &kv : data) {
            file << kv.first << ": " << kv.second << "\n";
        }
        file.close();
        return true;
    }

    
    static bool create_temp_log_file(const std::string &folder_path, const std::string &logFilePath)
    {

        // CREATE fileNames.txt FOR FUTURE FUNCTIONAL CALLS
        std::ofstream logFile(logFilePath);
        if (!logFile.is_open())
        {
            std::cerr << "WARNING: Could not create log file 'fileNames.txt'." << std::endl;
            return false;
        }

        std::cout << "LOGFILE PATH: " << logFilePath << std::endl;

        try
        {
            for (const auto &entry : fs::directory_iterator(folder_path))
            {
                if (entry.is_regular_file())
                {
                    const auto &filePath = entry.path();
                    if (filePath.extension() == ".txt")
                    {
                        logFile << filePath.filename().string() << std::endl;
                        std::cout << "Logged *txt file: " << filePath.filename().string() << std::endl;
                    }
                    else
                    {
                        std::cout << "Skipping file " << filePath.filename().string()
                                  << " due to its file type extension: " << filePath.extension().string() << "." << std::endl;
                    }
                }
            }
            return true;
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << "ERROR: " << e.what() << std::endl;
            return false;
        }
    }

    static bool write_summed_output(const std::string &filename, const std::map<std::string, std::vector<int>> &data) {
        std::ofstream outfile(filename);
        if (!outfile) {
            ErrorHandler::reportError("Could not open file " + filename + " for writing.");
            return false;
        }
        for (const auto &kv : data) {
            int sum = 0;
            for (int count : kv.second) {
                sum += count;
            }
            outfile << "<\"" << kv.first << "\", " << sum << ">\n";
        }
        outfile.close();
        return true;
    }

    static bool read_mapped_data(const std::string &filename, std::vector<std::pair<std::string, int>> &mapped_data) {
        std::ifstream infile(filename);
        if (!infile) {
            ErrorHandler::reportError("Could not open file " + filename + " for reading.");
            return false;
        }
        std::string line;
        while (std::getline(infile, line)) {
            std::string word;
            int count;
            size_t start = line.find('<');
            size_t comma = line.find(',', start);
            size_t end = line.find('>', comma);
            if (start != std::string::npos && comma != std::string::npos && end != std::string::npos) {
                word = line.substr(start + 1, comma - start - 1);
                std::string count_str = line.substr(comma + 1, end - comma - 1);
                std::stringstream ss(count_str);
                ss >> count;
                if (!word.empty()) {
                    mapped_data.emplace_back(word, count);
                }
            }
        }
        infile.close();
        return true;
    }

    static bool extract_values_from_temp_input(std::vector<std::string> &lines, const std::string &tempFolder) {
        std::ifstream infile(tempFolder);
        if (!infile) {
            ErrorHandler::reportError("Could not open file " + tempFolder + " for reading.");
            return false;
        }
        std::string kv_line;
        while (std::getline(infile, kv_line)) {
            size_t quote_pos = kv_line.find("\", \"");
            if (quote_pos != std::string::npos) {
                std::string value = kv_line.substr(quote_pos + 4);
                if (!value.empty() && value.back() == '>')
                    value.pop_back();
                if (!value.empty() && value.back() == '"')
                    value.pop_back();
                lines.push_back(value);
            }
        }
        infile.close();
        return true;
    }
};