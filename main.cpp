#include "cuda_base.cuh"
#include <iostream>
#include <curl/curl.h>

using namespace ImgixCuda;

size_t writeCallback(void *contents, size_t size, size_t nmemb, std::string *output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

int main(int argc, char* argv[]) {
  std::cout << "Hello, World!" << std::endl;
  std::cout << SingletonCuda::GetInstance().GetCudaDeviceInfo() << std::endl;
  gpu_stats s = SingletonCuda::GetInstance().GetStats();
  std::cout << "Temp:"<< s.temperature << ", Core:" << s.utilization_gpu << ", Mem:" << s.utilization_mem << std::endl;

  CURL *curl;
  CURLcode res;
  std::string response;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      res = curl_easy_perform(curl);

      if(res == CURLE_OK) {
          std::cout << "Response:\n" << response << std::endl;
      } else {
          std::cerr << "Error: " << curl_easy_strerror(res) << std::endl;
      }

      curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
  return 0;
}