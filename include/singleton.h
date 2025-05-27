#pragma once
#include <shared_mutex>
#include <future>
#include <unordered_map>
#include <variant>

namespace Imgix {
    // Variant to hold different types of variable values
    using VarValue = std::variant<double, bool, std::string>;

    constexpr double Double_Max = std::numeric_limits<double>::infinity();
    constexpr double Double_Min = -std::numeric_limits<double>::infinity();
    constexpr unsigned long long Ull_Max = std::numeric_limits<long>::max();

    class HardwareStats {
    public:
        double cpu_usage{ 0 };
        double ram_usage{ 0 };
        double gpu_usage{ 0 };
        double gpu_ram_usage{ 0 };
        double gpu_temperature{ 0 };
        double gpu_encoder_usage{ 0 };
        double gpu_decoder_usage{ 0 };
        double gpu_jpg_usage{ 0 };

        HardwareStats() = default;

        HardwareStats(double cpu, double ram, double gpu, double gpu_ram,
                    double temp, double enc, double dec, double jpg)
            : cpu_usage(cpu), ram_usage(ram), gpu_usage(gpu), gpu_ram_usage(gpu_ram),
            gpu_temperature(temp), gpu_encoder_usage(enc),
            gpu_decoder_usage(dec), gpu_jpg_usage(jpg)
        { }

        HardwareStats(const HardwareStats&) = default;
        HardwareStats(HardwareStats&&) = default;
        HardwareStats& operator=(HardwareStats&&) = default;

        HardwareStats& operator=(const HardwareStats& other) {
            cpu_usage = other.cpu_usage;
            ram_usage = other.ram_usage;
            gpu_usage = other.gpu_usage;
            gpu_ram_usage = other.gpu_ram_usage;
            gpu_temperature = other.gpu_temperature;
            gpu_encoder_usage = other.gpu_encoder_usage;
            gpu_decoder_usage = other.gpu_decoder_usage;
            gpu_jpg_usage = other.gpu_jpg_usage;
            return *this;
        }

        HardwareStats& operator+=(const HardwareStats& other) {
            cpu_usage += other.cpu_usage;
            ram_usage += other.ram_usage;
            gpu_usage += other.gpu_usage;
            gpu_ram_usage += other.gpu_ram_usage;
            gpu_temperature += other.gpu_temperature;
            gpu_encoder_usage += other.gpu_encoder_usage;
            gpu_decoder_usage += other.gpu_decoder_usage;
            gpu_jpg_usage += other.gpu_jpg_usage;
            return *this;
        }

        HardwareStats divide(int divisor) const {
            HardwareStats res;
            res.cpu_usage = cpu_usage / divisor;
            res.ram_usage = ram_usage / divisor;
            res.gpu_usage = gpu_usage / divisor;
            res.gpu_ram_usage = gpu_ram_usage / divisor;
            res.gpu_temperature = gpu_temperature / divisor;
            res.gpu_encoder_usage = gpu_encoder_usage / divisor;
            res.gpu_decoder_usage = gpu_decoder_usage / divisor;
            res.gpu_jpg_usage = gpu_jpg_usage / divisor;
            return res;
        }

        void set_zero() {
            cpu_usage = 0;
            ram_usage = 0;
            gpu_usage = 0;
            gpu_ram_usage = 0;
            gpu_temperature = 0;
            gpu_encoder_usage = 0;
            gpu_decoder_usage = 0;
            gpu_jpg_usage = 0;
        }

        void update_max(const HardwareStats& other) {
            cpu_usage = std::max(cpu_usage, other.cpu_usage);
            ram_usage = std::max(ram_usage, other.ram_usage);
            gpu_usage = std::max(gpu_usage, other.gpu_usage);
            gpu_ram_usage = std::max(gpu_ram_usage, other.gpu_ram_usage);
            gpu_temperature = std::max(gpu_temperature, other.gpu_temperature);
            gpu_encoder_usage = std::max(gpu_encoder_usage, other.gpu_encoder_usage);
            gpu_decoder_usage = std::max(gpu_decoder_usage, other.gpu_decoder_usage);
            gpu_jpg_usage = std::max(gpu_jpg_usage, other.gpu_jpg_usage);
        }

        bool is_zero() const {
            return cpu_usage == 0.0 && ram_usage == 0.0 &&
                gpu_usage == 0.0 && gpu_ram_usage == 0.0 &&
                gpu_temperature == 0.0 && gpu_encoder_usage == 0.0 &&
                gpu_decoder_usage == 0.0 && gpu_jpg_usage == 0.0;
        }
    };

    class HardwareStatsAggregator {
    private:
        HardwareStats sum;
        HardwareStats max_val;
        HardwareStats average;
        int count;
    public:
        HardwareStatsAggregator() : sum(), 
            max_val(Double_Min, Double_Min, Double_Min, Double_Min, Double_Min, Double_Min, Double_Min, Double_Min),
            average(), count(0)
        { }

        void add(const HardwareStats& value) {
            sum += value;
            ++count;
            max_val.update_max(value);
        }

        HardwareStats getSum() const {
            return sum;
        }

        int getCount() const {
            return count;
        }

        HardwareStats getAverage() {
            if (count > 0) {
                average = sum.divide(count);
                return average;
            }
            return HardwareStats();
        }

        HardwareStats getMax() const {
            return count > 0 ? max_val : HardwareStats();
        }

        void reset() {
            sum.set_zero();
            count = 0;
            average.set_zero();
            max_val = HardwareStats(Double_Min, Double_Min, Double_Min, 
                Double_Min, Double_Min, Double_Min, Double_Min, Double_Min);
        }
    };

    // this class is Singleton design and only run constructor, init and destructor once.
    class Singleton {
    private:
        std::shared_mutex mutex;
        HardwareStatsAggregator hardware_aggregator;
        std::thread hardware_thread;
        std::promise<void> exit_signal;
        void InitNativeNonGpu();
        void HardwareMonitorLoop(std::future<void> futureObj);
        Singleton() { InitNativeNonGpu(); }            // Constructor
    public:
        static Singleton& GetInstance();
        Singleton(Singleton const&) = delete;   // Don't Implement, don't allow construct by reference
        void operator=(Singleton const&) = delete;    // Don't implement, don't allow assign
        std::string GetStatsInfo();
        void set_stats_map(std::unordered_map<std::string, VarValue>& map);
        void ResetStats();
        ~Singleton();
    };
}