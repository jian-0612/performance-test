#pragma once
#include <string>
#include <utility>
#include <vector>

namespace Imgix {
    class UrlEntry {
    public:
        std::string url;
        std::string header;

        UrlEntry(const std::string& u, const std::string& h)
            : url(u), header(h) {}
        void print() const;
    };

    class ReadConfig {
    public:
        static std::string trim(const std::string& s);
        static std::string parse_url(const std::string& line);
        static std::string parse_headers(const std::string& line);
        std::vector<UrlEntry> read_config_file(const std::string& filePath);
    };
}