#include "cuda_base.cuh"
#include <iostream>
#include <sstream>

using namespace ImgixCuda;

// *************** FOR ERROR CHECKING *******************
#ifndef NVML_RT_CALL
#define NVML_RT_CALL( call ) {                                                                                         \
        auto status = static_cast<nvmlReturn_t>( call );                                                               \
        if ( status != NVML_SUCCESS )                                                                                  \
            fprintf( stderr,                                                                                           \
                     "ERROR: CUDA NVML call \"%s\" in line %d of file %s failed "                                      \
                     "with "                                                                                           \
                     "%s (%d).\n",                                                                                     \
                     #call,                                                                                            \
                     __LINE__,                                                                                         \
                     __FILE__,                                                                                         \
                     nvmlErrorString( status ),                                                                        \
                     status );                                                                                         \
    }
#endif  // NVML_RT_CALL

int constexpr nvml_device_name_buffer_size{ 100 };

bool HasCudaDevice() {
    int devCount = -1;
    try {
        cudaError_t status = cudaGetDeviceCount(&devCount);
        if (cudaSuccess == status) {
            return devCount > 0;
        }
        else {
            std::cerr << "HasCudaDevice->cudaGetDeviceCount return error code:"
                << cudaGetErrorString(status) << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "HasCudaDevice->cudaGetDeviceCount throw an exception:"
            << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "HasCudaDevice->cudaGetDeviceCount throw an exception: unknown exception"
            << std::endl;
    }
    return false;
}

SingletonCuda::SingletonCuda() : _nvmlDevice(NULL), s_totalMem(0), _onceFlag{} {
    std::call_once(_onceFlag, [this]() {
        if (_firstRun) {
            InitCUDA();
            *(const_cast<bool*>(&_firstRun)) = false;
        }
    });
}

SingletonCuda& SingletonCuda::GetInstance() {
    static SingletonCuda instance;                // Guaranteed to be destroyed. Instantiated on first use.
    return instance;
}

void SingletonCuda::InitCUDA() {
    if (HasCudaDevice()) {
        *(const_cast<bool*>(&HasGpu)) = true;
    }
    else {
        *(const_cast<bool*>(&HasGpu)) = false;
        std::cout << "There is no available CUDA GPU device in this machine." << std::endl;
        return;
    }
    ErrChk(cudaSetDevice(0));
    char name[nvml_device_name_buffer_size];
    // Initialize NVML library
    NVML_RT_CALL(nvmlInit());
    // Query device handle
    NVML_RT_CALL(nvmlDeviceGetHandleByIndex(0, &_nvmlDevice));
    // Query device name
    NVML_RT_CALL(nvmlDeviceGetName(_nvmlDevice, name, nvml_device_name_buffer_size));
    cudaDeviceProp devProp;
    ErrChk(cudaGetDeviceProperties(&devProp, 0));
    s_totalMem = devProp.totalGlobalMem / 1024 / 1024 / 1024;
    std::cout << name << std::endl;
    std::cout << "Total memory: " << s_totalMem << "GB" << std::endl;
    std::cout << "SingletonCuda InitCUDA finished." << std::endl;
}

__host__ void SingletonCuda::ResetGpu() {
    cudaDeviceReset();
}

SingletonCuda::~SingletonCuda() {
    if (HasGpu) {
        std::cout << "SingletonCuda destructor start." << std::endl;
        NVML_RT_CALL(nvmlShutdown());
        std::cout << "SingletonCuda destructor finished." << std::endl;
    }
}

__host__ std::string SingletonCuda::getDevProp(cudaDeviceProp devProp) {
    std::ostringstream stringStream;
    stringStream << "Major revision number: " << devProp.major << "\n";
    stringStream << "Minor revision number: " << devProp.minor << "\n";
    stringStream << "Name: " << devProp.name << "\n";
    stringStream << "Total global memory: " << devProp.totalGlobalMem << "\n";
    stringStream << "Total shared memory per block: " << devProp.sharedMemPerBlock << "\n";
    stringStream << "Total registers per block: " << devProp.regsPerBlock << "\n";
    stringStream << "Warp size: " << devProp.warpSize << "\n";
    stringStream << "Maximum memory pitch: " << devProp.memPitch << "\n";
    stringStream << "Maximum threads per block: " << devProp.maxThreadsPerBlock << "\n";
    for (int i = 0; i < 3; ++i)
        stringStream << "Maximum dimension " << i << " of block: " << devProp.maxThreadsDim[i] << "\n";
    for (int i = 0; i < 3; ++i)
        stringStream << "Maximum dimension " << i << " of grid: " << devProp.maxGridSize[i] << "\n";
    stringStream << "Clock rate: " << devProp.clockRate << "\n";
    stringStream << "Total constant memory: " << devProp.totalConstMem << "\n";
    stringStream << "Texture alignment: " << devProp.textureAlignment << "\n";
    stringStream << "Concurrent copy and execution: " << (devProp.deviceOverlap ? "Yes" : "No") << "\n";
    stringStream << "Number of multiprocessors: " << devProp.multiProcessorCount << "\n";
    stringStream << "Kernel execution timeout: " << (devProp.kernelExecTimeoutEnabled ? "Yes" : "No") << "\n";
    stringStream << "Unified Addressing: " << (devProp.unifiedAddressing ? "Yes" : "No") << "\n";
    return stringStream.str();
}

__host__ std::string SingletonCuda::GetCudaDeviceInfo() {
    std::ostringstream stringStream;
    // Number of CUDA devices
    int devCount;
    ErrChk(cudaGetDeviceCount(&devCount));
    stringStream << "CUDA Device Query...\n";
    if (devCount > 1) {
        stringStream << "There are " << devCount << " CUDA devices.\n";
    }
    else if (devCount == 1) {
        stringStream << "There is " << devCount << " CUDA device.\n";
    }
    // Iterate through devices
    for (int i = 0; i < devCount; ++i) {
        // Get device properties
        stringStream << "\nCUDA Device " << i << "\n";
        cudaDeviceProp devProp;
        cudaGetDeviceProperties(&devProp, i);
        stringStream << SingletonCuda::getDevProp(devProp);
    }
    return stringStream.str();
}

gpu_stats SingletonCuda::GetStats() {
    gpu_stats device_stats{};
    NVML_RT_CALL(nvmlDeviceGetTemperature(_nvmlDevice, NVML_TEMPERATURE_GPU, &device_stats.temperature));
    nvmlUtilization_t util;
    NVML_RT_CALL(nvmlDeviceGetUtilizationRates(_nvmlDevice, &util));
    unsigned int samplingPeriod = 0; // us
    NVML_RT_CALL(nvmlDeviceGetEncoderUtilization(_nvmlDevice, &device_stats.utilization_encoder, &samplingPeriod));
    NVML_RT_CALL(nvmlDeviceGetDecoderUtilization(_nvmlDevice, &device_stats.utilization_decoder, &samplingPeriod));
    NVML_RT_CALL(nvmlDeviceGetJpgUtilization(_nvmlDevice, &device_stats.utilization_jpg, &samplingPeriod));
    
    device_stats.utilization_gpu = util.gpu;
    device_stats.utilization_mem = util.memory;
    return device_stats;
}