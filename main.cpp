#include "cuda_base.cuh"
#include "singleton.h"
#include "read_config.h"
#include "http_request.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <curl/curl.h>

using namespace Imgix;
using namespace ImgixCuda;

bool doWork(const UrlEntry& url, const std::string& path, const HttpRequest& req) {
    std::cout << url.url << " ## " << path << std::endl;
    if (req.download_to_file(url, path)) {
        std::cout << "download_to_file succeed!" << std::endl;
        return true;
    } else {
        std::cout << "download_to_file failed!" << std::endl;
        return false;
    }
}

template <typename T>
std::vector<T> whenAll(std::vector<std::future<T>>& futures) {
    std::vector<T> results;
    for (auto& fut : futures) {
        results.push_back(fut.get()); // blocks until each is done
    }
    return results;
}

int main(int argc, char* argv[]) {
    // std::cout << SingletonCuda::GetInstance().GetCudaDeviceInfo() << std::endl;
    // gpu_stats s = SingletonCuda::GetInstance().GetStats();
    // std::cout << "Temp:"<< s.temperature << ", Core:" << s.utilization_gpu << ", Mem:" << s.utilization_mem << std::endl;

    // for(int i=0; i < 2; ++i) {
    //     std::string str = Singleton::GetInstance().GetStatsInfo();
    //     std::cout << str;
    //     std::this_thread::sleep_for(std::chrono::seconds(2));
    // }

    ReadConfig read_config;
    std::vector<UrlEntry> vec = read_config.read_config_file("/home/mmajian/code/performance-test/video.toml");
    std::vector<std::pair<UrlEntry, std::string>> url_file_pairs;
    int fileNo = 0;
    for (size_t i = 0; i < vec.size(); ++i) {
        std::string file_path = "./output/" + vec[i].url;
        vec[i].url = "http://localhost:8001/http://localhost:8889/" + vec[i].url;
        url_file_pairs.push_back(std::make_pair(vec[i], file_path));
    }

    HttpRequest req;
    // std::cout << url_file_pairs[0].first.url << std::endl << url_file_pairs[0].second << std::endl;
    // if (req.download_to_file(url_file_pairs[0].first, url_file_pairs[0].second)) {
    //     std::cout << "download_to_file succeed!" << std::endl;
    // } else {
    //     std::cout << "download_to_file failed!" << std::endl;
    // }
    Singleton& single = Singleton::GetInstance();
    single.ResetStats();
    auto start = std::chrono::high_resolution_clock::now();
    // if (req.download_multiple_to_files(url_file_pairs)) {
    //     std::cout << "download_to_file succeed!" << std::endl;
    // } else {
    //     std::cout << "download_to_file failed!" << std::endl;
    // }

    // std::vector<std::future<bool>> futures;
    // for (const auto& [a, b] : url_file_pairs) {
    //     futures.emplace_back(std::async(std::launch::async, doWork, a, b, req));
    // }
    // auto results = whenAll(futures);

    std::vector<std::thread> threads;
    for (size_t i = 0; i < url_file_pairs.size(); ++i) {
        threads.emplace_back(doWork, url_file_pairs[i].first, url_file_pairs[i].second, req);
    }
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken: " << duration_ms.count() << " ms" << std::endl;
    std::cout << single.GetStatsInfo();
    return 0;
}