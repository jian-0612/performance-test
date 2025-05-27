#include "../include/http_request.h"
#include <curl/curl.h>
#include <iostream>
#include <unordered_map>

using namespace Imgix;
constexpr int PollWaitTime_ms = 100;

HttpRequest::HttpRequest() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpRequest::~HttpRequest() {
    curl_global_cleanup();
}

bool HttpRequest::download_to_file(const UrlEntry& url_entry, const std::string& outputPath) const {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    FILE* file = fopen(outputPath.c_str(), "wb");
    if (!file) {
        curl_easy_cleanup(curl);
        return false;
    }

    // Set custom headers
    struct curl_slist* headers = nullptr;
    if (!url_entry.header.empty()) {
        headers = curl_slist_append(headers, url_entry.header.data());
    }
    curl_easy_setopt(curl, CURLOPT_URL, url_entry.url.c_str());
    if (headers != nullptr) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(file);
    if (headers != nullptr) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    return (res == CURLE_OK);
}

bool HttpRequest::download_multiple_to_files(const std::vector<std::pair<UrlEntry, std::string>>& urlFilePairs) const {
    CURLM* multiHandle = curl_multi_init();
    if (!multiHandle) {
        return false;
    }
    std::vector<DownloadContext> contexts;
    for (const auto& [url_entry, path] : urlFilePairs) {
        FILE* file = fopen(path.c_str(), "wb");
        if (!file) {
            continue;
        }
        CURL* curl = curl_easy_init();
        if (!curl) {
            fclose(file);
            continue;
        }
        // Set custom headers
        struct curl_slist* headers = nullptr;
        if (!url_entry.header.empty()) {
            headers = curl_slist_append(headers, url_entry.header.data());
        }
        curl_easy_setopt(curl, CURLOPT_URL, url_entry.url.c_str());
        if (headers != nullptr) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_multi_add_handle(multiHandle, curl);
        contexts.push_back({ curl, headers, file });
    }
    int stillRunning = 0;
    CURLMcode mc;
    do {
        mc = curl_multi_perform(multiHandle, &stillRunning);
        if (mc != CURLM_OK) {
            break;
        }
        curl_multi_poll(multiHandle, nullptr, 0, PollWaitTime_ms, nullptr);
    } while (stillRunning > 0);
    // Clean up
    for (auto& ctx : contexts) {
        if (ctx.headers != nullptr) {
            curl_slist_free_all(ctx.headers);
        }
        curl_multi_remove_handle(multiHandle, ctx.curl);
        curl_easy_cleanup(ctx.curl);
        fclose(ctx.file);
    }
    curl_multi_cleanup(multiHandle);
    return (mc == CURLM_OK);
}