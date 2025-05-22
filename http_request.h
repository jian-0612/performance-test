#pragma once
#include "read_config.h"
#include <curl/curl.h>

namespace Imgix {
    struct DownloadContext {
        CURL* curl;
        curl_slist* headers;
        FILE* file;

        DownloadContext() : curl{nullptr}, headers{nullptr}, file{nullptr}
        { }

        DownloadContext(CURL* c, curl_slist* h, FILE* f) : curl(c), headers(h), file(f)
        { }
    };

    class HttpRequest {
    public:
        HttpRequest();
        ~HttpRequest();
        bool download_to_file(const UrlEntry& url_entry, const std::string& outputPath) const;
        bool download_multiple_to_files(const std::vector<std::pair<UrlEntry, std::string>>& urlFilePairs) const;
    };
}