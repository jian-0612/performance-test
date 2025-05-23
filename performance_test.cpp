#include "performance_test.h"
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <cmath>

using namespace Imgix;


void Imgix::from_json(const json& j, TestConfig& config) {
    j.at("name").get_to(config.name);
    j.at("type").get_to(config.type);
    j.at("loop_time").get_to(config.loop_time);
    j.at("duration_s").get_to(config.duration_s);
    j.at("concurrency").get_to(config.concurrency);
    j.at("test_file_path").get_to(config.test_file_path);
    j.at("baseline_checks").get_to(config.baseline_checks);
}

std::string TestConfig::to_string() const {
    std::stringstream ss;
    ss << "Name: " << name << ", ";
    ss << "Type: " << type << ", ";
    ss << "Loop Time: " << loop_time << ", ";
    ss << "Duration (s): " << duration_s << ", ";
    ss << "Concurrency: " << concurrency << ", ";
    ss << "Test File Path: " << test_file_path << ", ";
    ss << "Baseline Checks: ";
    for (const auto& check : baseline_checks) {
        ss << check << ",";
    }
    ss << "\n";
    return ss.str();
}

bool PerformanceTest::init(const std::string& json_config_file) {
    std::ifstream config_file(json_config_file);
    if (!config_file.is_open()) {
        std::cerr << "Could not open config.json" << std::endl;
        return false;
    }
    json j;
    try {
        config_file >> j;
    } catch (json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
    try {
        configs = j.get<std::vector<TestConfig>>();
    } catch (const json::type_error& e) {
        std::cerr << "JSON type error: " << e.what() << std::endl;
        return false;
    }
    config_file.close();
    return true;
}

std::string PerformanceTest::to_string_configs() const {
    std::stringstream ss;
    for (TestConfig tc : configs) {
        ss << tc.to_string() << std::endl;
    }
    return ss.str();
}

bool PerformanceTest::evaluate_bool_expression(const std::string& expr, 
    const std::unordered_map<std::string, VarValue>& context) {
    std::string lhs, op, rhs;
    // Detect operator
    const std::vector<std::string> operators = {"==", "!=", "<=", ">=", "<", ">"};
    size_t op_pos = std::string::npos;
    for (const auto& candidate : operators) {
        op_pos = expr.find(candidate);
        if (op_pos != std::string::npos) {
            op = candidate;
            break;
        }
    }
    if (op_pos == std::string::npos) {
        std::cerr << "No valid operator found in expression." << std::endl;
        return false;
    }
    lhs = ReadConfig::trim(expr.substr(0, op_pos));
    rhs = ReadConfig::trim(expr.substr(op_pos + op.length()));
    // Check if lhs is in context
    if (context.find(lhs) == context.end()) {
        std::cerr << "Unknown variable: " << lhs << std::endl;
        return false;
    }
    const auto& lhs_val = context.at(lhs);
    //const auto& lhs_val = context[lhs];
    try {
        if (std::holds_alternative<double>(lhs_val)) {
            double lhs_num = std::get<double>(lhs_val);
            double rhs_num = std::stod(rhs);
            if (op == "==") return fabs(lhs_num - rhs_num) < 1e-9;
            if (op == "!=") return fabs(lhs_num - rhs_num) >= 1e-9;
            if (op == "<")  return lhs_num < rhs_num;
            if (op == "<=") return lhs_num <= rhs_num;
            if (op == ">")  return lhs_num > rhs_num;
            if (op == ">=") return lhs_num >= rhs_num;

        } else if (std::holds_alternative<bool>(lhs_val)) {
            bool lhs_bool = std::get<bool>(lhs_val);
            bool rhs_bool = (rhs == "true" || rhs == "1");

            if (op == "==") return lhs_bool == rhs_bool;
            if (op == "!=") return lhs_bool != rhs_bool;

        } else if (std::holds_alternative<std::string>(lhs_val)) {
            std::string lhs_str = std::get<std::string>(lhs_val);
            std::string rhs_str = rhs;
            if (rhs.front() == '"' && rhs.back() == '"') {
                rhs_str = rhs.substr(1, rhs.size() - 2);
            }
            if (op == "==") return lhs_str == rhs_str;
            if (op == "!=") return lhs_str != rhs_str;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error evaluating expression: " << e.what() << std::endl;
        return false;
    }
    std::cerr << "Unsupported operation for this type." << std::endl;
    return false;
}

bool PerformanceTest::benchmark_sync(const std::vector<std::pair<UrlEntry, std::string>>& url_file_pairs) {
    for (auto pair : url_file_pairs) {
        if (!http_req.download_to_file(pair.first, pair.second)) {
            return false;
        }
    }
    return true;
}

template <typename T>
std::vector<T> whenAll(std::vector<std::future<T>>& futures) {
    std::vector<T> results;
    for (auto& fut : futures) {
        results.push_back(fut.get()); // blocks until each is done
    }
    return results;
}

bool doWork(const UrlEntry& url, const std::string& path, const HttpRequest& req) {
    //std::cout << url.url << " ## " << path << std::endl;
    if (req.download_to_file(url, path)) {
        //std::cout << "download_to_file succeed!" << std::endl;
        return true;
    } else {
        //std::cout << "download_to_file failed!" << std::endl;
        return false;
    }
}

bool PerformanceTest::benchmark_async(const std::vector<std::pair<UrlEntry, std::string>>& url_file_pairs) {
    std::vector<std::future<bool>> futures;
    for (const auto& [a, b] : url_file_pairs) {
        futures.emplace_back(std::async(std::launch::async, doWork, a, b, http_req));
    }
    auto results = whenAll(futures);
    return std::all_of(results.begin(), results.end(), [](bool b){ return b; });
}

bool PerformanceTest::run_performance_test(const std::vector<std::pair<UrlEntry, std::string>>& url_file_pairs, 
    const TestConfig& tc, unsigned int& render_count) {
    bool res = true;
    render_count = 0;
    for (int i = 0; i < tc.loop_time; ++i) {
        if (tc.concurrency == "sync") {
            res = benchmark_sync(url_file_pairs);
            if (!res) {
                break;
            }
        } else if (tc.concurrency == "async") {
            res = benchmark_async(url_file_pairs);
            if (!res) {
                break;
            }
        }
        else {
            return false;
        }
        render_count += url_file_pairs.size();
    }
    return res;
}

bool PerformanceTest::run_stress_test(const std::vector<std::pair<UrlEntry, std::string>>& url_file_pairs, 
    const TestConfig& tc, unsigned int& render_count) {
    bool res = true;
    auto start = std::chrono::steady_clock::now();
    auto end = start + std::chrono::seconds(tc.duration_s);
    render_count = 0;
    while (std::chrono::steady_clock::now() < end) {
        if (tc.concurrency == "sync") {
            res = benchmark_sync(url_file_pairs);
            if (!res) {
                break;
            }
        } else if (tc.concurrency == "async") {
            res = benchmark_async(url_file_pairs);
            if (!res) {
                break;
            }
        }
        else {
            return false;
        }
        render_count += url_file_pairs.size();
    }
    return res;
}

bool PerformanceTest::check_benchmark_result(const std::unordered_map<std::string, VarValue>& context, const TestConfig& tc) {
    for (std::string expr : tc.baseline_checks) {
        if (!evaluate_bool_expression(expr, context)) {
            test_report_output_file << "baseline checks failed at: " << expr << std::endl;
            return false;
        }
    }
    return true;
}

void PerformanceTest::run_all_tests() {
    test_report_output_file.open("./output/report.txt");
    bool res = true;
    Singleton& single = Singleton::GetInstance();
    for (TestConfig tc : configs) {
        test_report_output_file << "Running " << tc.name << " test:\n";
        std::vector<UrlEntry> vec = read_config.read_config_file(tc.test_file_path);
        std::vector<std::pair<UrlEntry, std::string>> url_file_pairs;
        for (size_t i = 0; i < vec.size(); ++i) {
            std::string file_path = "./output/" + vec[i].url;
            vec[i].url = "http://localhost:8001/http://localhost:8889/" + vec[i].url;
            url_file_pairs.push_back(std::make_pair(vec[i], file_path));
        }
        single.ResetStats();
        unsigned int render_count = 0;
        std::chrono::_V2::system_clock::time_point start = std::chrono::high_resolution_clock::now();
        if (tc.type == "performance") {
            res = run_performance_test(url_file_pairs, tc, render_count);
            if (!res) {
                break;
            }
        } else if (tc.type == "stress") {
            res = run_stress_test(url_file_pairs, tc, render_count);
            if (!res) {
                break;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        double avg_render_time_s = (double)duration_ms.count() / (double)render_count / 1000.0;
        std::unordered_map<std::string, VarValue> stats_map;
        single.set_stats_map(stats_map);
        stats_map.insert(std::make_pair("render_avg", avg_render_time_s));

        test_report_output_file << "Time taken: " << duration_ms.count() << " ms. " 
            << "Average render time: " << avg_render_time_s << " seconds." << std::endl;
        test_report_output_file << single.GetStatsInfo();
        res = check_benchmark_result(stats_map, tc);
        if (!res) {
            break;
        }
    }
    if (!res) {
        test_report_output_file << "test failed.\n";
    } else {
        test_report_output_file << "All test passed.\n";
    }
    test_report_output_file.close();
}