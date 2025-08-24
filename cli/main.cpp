#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cxxopts.hpp> // Include cxxopts header

#include "../common/FileConverter.hpp"

// Helper function to split strings
std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char* argv[]) {
    cxxopts::Options options("file_converter", "A tool to convert file encodings in specified directories.");

    options.add_options()
        ("d,dirs", "Comma-separated list of directories to process", cxxopts::value<std::string>())
        ("e,exts", "Comma-separated list of file extensions to convert", cxxopts::value<std::string>())
        ("t,target", "Target encoding for conversion (e.g., UTF-8)", cxxopts::value<std::string>())
        ("b,backup", "Create backup files before conversion", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print usage");

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        if (!result.count("dirs") || !result.count("exts") || !result.count("target")) {
            std::cerr << "Error: Missing required arguments. Please use --help for usage." << std::endl;
            return 1;
        }

        // Parse command line arguments and split strings
        std::vector<std::string> target_dirs = splitString(result["dirs"].as<std::string>(), ',');
        std::vector<std::string> file_exts = splitString(result["exts"].as<std::string>(), ',');
        std::string target_encoding = result["target"].as<std::string>();
        bool backup_enabled = result["backup"].as<bool>();

        // Call FileConverter class to process files
        FileConverter::processDirectory(target_dirs, file_exts, target_encoding, backup_enabled);

    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
