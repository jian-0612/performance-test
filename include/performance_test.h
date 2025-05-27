#pragma once
#include "http_request.h"
#include "singleton.h"
#include "json.hpp"  // Include the nlohmann::json header
#include <fstream>
#include <chrono>

namespace Imgix {
    using json = nlohmann::json;
    

    class TestConfig {
    public:
        std::string name;
        std::string type;
        int loop_time;
        int duration_s;
        std::string concurrency;
        std::string test_file_path;
        std::vector<std::string> baseline_checks;

        TestConfig() = default;
        TestConfig(std::string n, std::string t, int lt, int d, std::string c, std::string tfp, std::vector<std::string> checks)
            : name(move(n)), type(move(t)), loop_time(lt), duration_s(d), concurrency(move(c)), 
                test_file_path(move(tfp)), baseline_checks(checks) { }
        std::string to_string() const;
    };

    void from_json(const json& j, TestConfig& config);
    
    class PerformanceTest {
    private:
        std::ofstream test_report_output_file;
        json test_plan_config;
        HttpRequest http_req;
        ReadConfig read_config;
        std::chrono::_V2::system_clock::time_point timer_start;
        std::chrono::_V2::system_clock::time_point timer_end;
        std::vector<TestConfig> configs;
    public:
        bool init(const std::string& json_config_file);
        bool benchmark_sync(const std::vector<std::pair<UrlEntry, std::string>>& url_file_pairs);
        bool benchmark_async(const std::vector<std::pair<UrlEntry, std::string>>& url_file_pairs);
        bool check_benchmark_result(const std::unordered_map<std::string, VarValue>& context, const TestConfig& tc);
        bool run_performance_test(const std::vector<std::pair<UrlEntry, std::string>>& url_file_pairs, 
            const TestConfig& tc, unsigned int& render_count);
        bool run_stress_test(const std::vector<std::pair<UrlEntry, std::string>>& url_file_pairs, 
            const TestConfig& tc, unsigned int& render_count);
        void run_all_tests();
        std::string to_string_configs() const;
        static bool evaluate_bool_expression(const std::string& expr, 
            const std::unordered_map<std::string, VarValue>& context);
        static bool delete_all_files(const std::string& target_dir);
    };
}