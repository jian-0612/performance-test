#include "singleton.h"
#include "cuda_base.cuh"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <dlfcn.h>
#include <statgrab.h>

using namespace Imgix;
using namespace ImgixCuda;

constexpr int Get_Stats_Wait_Time_ms = 200;

void Singleton::HardwareMonitorLoop(std::future<void> futureObj)
{
    sg_init(1);
    /* Drop setuid/setgid privileges. */
    if (sg_drop_privileges() != SG_ERROR_NONE) {
        std::cerr << "Error. Failed to drop privileges" << std::endl;
    }
    sg_cpu_percents* cpu_percent;
    sg_cpu_stats* cpu_diff_stats;
    sg_mem_stats* mem_stats;
    sg_snapshot();
    cpu_percent = sg_get_cpu_percents(nullptr);
    SingletonCuda& singleton_cuda = SingletonCuda::GetInstance();
    while (futureObj.wait_for(std::chrono::milliseconds(Get_Stats_Wait_Time_ms)) == std::future_status::timeout) {
        if (((cpu_diff_stats = sg_get_cpu_stats_diff(nullptr)) == nullptr) ||
            ((cpu_percent = sg_get_cpu_percents_of(sg_last_diff_cpu_percent, nullptr)) == nullptr)) {
            std::cout << "sg_get_cpu_percents_error" << std::endl;
            break;
        }
        if ((mem_stats = sg_get_mem_stats(nullptr)) == nullptr) {
            std::cout << "sg_get_mem_stats_error" << std::endl;
            break;
        }
        double cpu_usage = cpu_percent->user + cpu_percent->kernel;
        double ram_usage = (double)mem_stats->used / (double)mem_stats->total * 100;
        gpu_stats gpu_stats = singleton_cuda.GetStats();
        HardwareStats stats(cpu_usage, ram_usage, gpu_stats.utilization_gpu, 
            gpu_stats.utilization_mem, gpu_stats.temperature, gpu_stats.utilization_encoder, 
            gpu_stats.utilization_decoder, gpu_stats.utilization_jpg);
        mutex.lock();
        hardware_aggregator.add(stats);
        mutex.unlock();
    }
    sg_shutdown();
    std::cout << "HardwareMonitorLoop finished." << std::endl;
}

Singleton& Singleton::GetInstance() {
    static Singleton instance;   // Guaranteed to be destroyed. Instantiated on first use.
    return instance;
}

void Singleton::InitNativeNonGpu() {
    std::future<void> futureObj = exit_signal.get_future();
    hardware_thread = std::thread(&Singleton::HardwareMonitorLoop, this, std::move(futureObj));
    std::cout << "Hardware Monitor Thread started." << std::endl;
    std::cout << "Singleton InitNative finished." << std::endl;
}

Singleton::~Singleton() {
    exit_signal.set_value();
    hardware_thread.join();
    std::cout << "Hardware Monitor Thread stopped." << std::endl;
    std::cout << "Singleton destructor finished." << std::endl;
}

std::string Singleton::GetStatsInfo() {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    mutex.lock();
    HardwareStats avg = hardware_aggregator.getAverage();
    HardwareStats max = hardware_aggregator.getMax();
    mutex.unlock();
    if (avg.is_zero()) {
        ss << "Run too fast, didn't get any hardware measurements." << std::endl;
    }
    else {
        ss << "CPU avg:" << avg.cpu_usage << "%,RAM avg:" << avg.ram_usage 
            << "%,GPU avg:" << avg.gpu_usage << "%,GPU_RAM avg:" << avg.gpu_ram_usage 
            << "%,GPU_encoder avg:" << avg.gpu_encoder_usage << "%,GPU_decoder avg:" << avg.gpu_decoder_usage
            << "%,GPU_jpg avg:" << avg.gpu_jpg_usage << "%,GPU_temperature avg:" << avg.gpu_temperature
            << " C" << std::endl;
        ss << "CPU max:" << max.cpu_usage << "%,RAM max:" << max.ram_usage 
            << "%,GPU max:" << max.gpu_usage << "%,GPU_RAM max:" << max.gpu_ram_usage 
            << "%,GPU_encoder max:" << max.gpu_encoder_usage << "%,GPU_decoder max:" << max.gpu_decoder_usage
            << "%,GPU_jpg max:" << max.gpu_jpg_usage << "%,GPU_temperature max:" << max.gpu_temperature
            << " C" << std::endl;
    }
    return ss.str();
}

void Singleton::ResetStats() {
    mutex.lock();
    hardware_aggregator.reset();
    mutex.unlock();
}