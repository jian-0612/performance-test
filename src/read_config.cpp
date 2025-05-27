#include "../include/read_config.h"
#include <iostream>
#include <fstream>

using namespace Imgix;

void UrlEntry::print() const {
    std::cout << "URL: " << url << std::endl;
    std::cout << "Header: " << header << std::endl;
}

std::string ReadConfig::trim(const std::string& s){
    auto start = s.find_first_not_of(" \t\n\r");
    auto end = s.find_last_not_of(" \t\n\r");
    return (start == std::string::npos || end == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string ReadConfig::parse_url(const std::string& line) {
    if (line.size() >= 4 &&
        line.substr(0, 2) == "[\"" &&
        line.substr(line.size() - 2) == "\"]") {
        return line.substr(2, line.size() - 4);
    }
    return "";
}

std::string ReadConfig::parse_headers(const std::string& line) {
    std::string key = "", value = "";
    if (line.size() > 11 && 
        line.substr(0, 11) == "headers = {" && 
        line.substr(line.size() - 1) == "}") {
        size_t pos1 = line.find('\'');
        size_t pos2 = line.find('\'', pos1 + 1);
        size_t pos3 = line.find('\'', pos2 + 1);
        size_t pos4 = line.find('\'', pos3 + 1);
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            key = line.substr(pos1 + 1, pos2 - pos1 - 1);
        }
        if ( pos3 != std::string::npos && pos4 != std::string::npos) {
            value = line.substr(pos3 + 1, pos4 - pos3 - 1);
        }
    }
    return key.empty() && value.empty() ? "" : key + ":" + value;
}

std::vector<UrlEntry> ReadConfig::read_config_file(const std::string& filePath) {
    std::vector<UrlEntry> res;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file:" << filePath << std::endl;
        return res;
    }
    std::string line;
    std::string url;
    while (std::getline(file, line)) {
        // Process each line here
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        url = parse_url(line);
        if (url == "") {
            continue;
        }
        std::string h = "";
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            h = parse_headers(line);
            break;
        }
        res.push_back(UrlEntry(url, h));
    }
    file.close();
    return res;
}