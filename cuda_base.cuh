#pragma once

#include <cuda_runtime.h>
#include <nvml.h>
#include <string>
#include <mutex>
#include <iostream>

namespace ImgixCuda {

#define ErrChk(ans) { checkCudaReturn((ans), __FILE__, __LINE__); }

    static inline void checkCudaReturn(cudaError_t code, const char* file, int line, bool abort = false) {
        if (code != cudaSuccess) {
            char errorMessage[500];
            sprintf(errorMessage, "Cuda Error: %s, FILE: %s, LINE: %d.\n", cudaGetErrorString(code), file, line);
            std::cerr << errorMessage << std::endl;
            if (abort) {
                exit(code); 
            }
            else { 
                throw std::domain_error(errorMessage); 
            }
        }
    }

    #define ErrReturn(status) { if(status != cudaSuccess) return status;}

    __host__ __device__ inline void cudaAssert(bool res, const char* file, int line, const char* errorMsg = "") {
        if (!res) {
            printf("cudaAssert: %s %d, Error Message: %s\n", file, line, errorMsg);
#if defined(__CUDA_ARCH__)
            printf("%s\n", errorMsg);
            assert(false);
#else
            errorMsg ? throw std::domain_error("") : throw std::domain_error(errorMsg);
#endif
        }
    }

#define cudaReleaseAssert(res) { cudaAssert((res), __FILE__, __LINE__, ("")); }
#define cudaReleaseAssert2(res, errorMsg) { cudaAssert((res), __FILE__, __LINE__, (errorMsg)); }

    struct gpu_stats {
        unsigned int temperature;           // GPU temperature in Celsius
        unsigned int utilization_gpu;       // GPU core utilization in percentage
        unsigned int utilization_mem;       // GPU memory utilization in percentage
        unsigned int utilization_encoder;   // GPU encoder utilization in percentage
        unsigned int utilization_decoder;   // GPU decoder utilization in percentage
        unsigned int utilization_jpg;       // GPU JPG utilization in percentage

        // Default constructor
        gpu_stats()
            : temperature(0), utilization_gpu(0), utilization_mem(0),
            utilization_encoder(0), utilization_decoder(0), utilization_jpg(0)
        { }

        // Constructor with main utilization values
        gpu_stats(unsigned int t, unsigned int ug, unsigned int um,
                unsigned int ue = 0, unsigned int ud = 0, unsigned int uj = 0)
            : temperature(t), utilization_gpu(ug), utilization_mem(um),
            utilization_encoder(ue), utilization_decoder(ud), utilization_jpg(uj)
        { }

        gpu_stats(const gpu_stats&) = default;
        gpu_stats(gpu_stats&&) = default;
        gpu_stats& operator=(gpu_stats&&) = default;

        // Copy assignment operator
        gpu_stats& operator=(const gpu_stats& other) {
            if (this != &other) {
                temperature = other.temperature;
                utilization_gpu = other.utilization_gpu;
                utilization_mem = other.utilization_mem;
                utilization_encoder = other.utilization_encoder;
                utilization_decoder = other.utilization_decoder;
                utilization_jpg = other.utilization_jpg;
            }
            return *this;
        }
    };

    // this class is Singleton design and only run constructor, init and destructor once.
    class SingletonCuda {
    private:
        nvmlDevice_t _nvmlDevice;
        const bool _firstRun = true;
        std::once_flag _onceFlag;
        __host__ void InitCUDA();
        __host__ SingletonCuda();
        __host__ static std::string getDevProp(cudaDeviceProp devProp);
    public:
        const bool HasGpu = true;
        __host__ static SingletonCuda& GetInstance();
        SingletonCuda(SingletonCuda const&) = delete;   // Don't Implement, don't allow construct by reference
        void operator=(SingletonCuda const&) = delete;    // Don't implement, don't allow assign
        size_t s_totalMem; //unit GB
        __host__ std::string GetCudaDeviceInfo();
        __host__ gpu_stats GetStats();
        __host__ void ResetGpu();
        __host__ ~SingletonCuda();
    };
}