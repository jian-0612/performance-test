#include "performance_test.h"

using namespace Imgix;

int main(int argc, char* argv[]) {
    std::string config_path;
    if (argc > 1) {
        config_path = argv[0];
    } else {
        config_path = "./config.json";
    }
    PerformanceTest pt;
    pt.init(config_path);
    //std::cout << pt.to_string_configs();
    pt.run_all_tests();
    return 0;
}