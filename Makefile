NVCC=/usr/local/cuda-12.9/bin/nvcc
CXX=g++-

CUDA_PATH = /usr/local/cuda-12.9
CUDAINCLUDE := -I$(CUDA_PATH)/include
CUDALIB := -L$(CUDA_PATH)/lib64 -L$(CUDA_PATH)/lib64/stubs -lcudart -lcudadevrt -lnvidia-ml

OPTFLAGS :=
CUDAGENCODE := -gencode=arch=compute_61,code=sm_61 -gencode=arch=compute_75,code=sm_75 -gencode=arch=compute_86,code=sm_86 -gencode=arch=compute_89,code=sm_89
NVCCFLAGS := $(CUDAGENCODE) -x cu -rdc=true -m64 --compile --std=c++17 -ccbin g++ -Xcompiler -fPIC
NVCCLINKFLAGS := $(CUDAGENCODE) -m64 --std=c++17 -ccbin g++ $(CUDALIB) --ptxas-options=-v --compiler-options '-fPIC'

CXX=g++
CXXFLAGS= -m64 --std=c++17
CXXLIBFLAGS= -fPIC
OUT_DIR_ROOT= bin/x64/
OUT_DIR :=
CUDAOBJS = cuda_base.o
EXEC:= performance-test

Release: NVCCFLAGS += -Xcompiler -O2
Release: NVCCLINKFLAGS += -Xcompiler -O2
Release: OPTFLAGS += -DRELEASE -O2
Release: performance-test
Release: OUT_DIR :=$(OUT_DIR_ROOT)Release/
Release: EXEC:= $(OUT_DIR)performance-test
Release: CUDAOBJS:= $(OUT_DIR)cuda_base.o

Debug: NVCCFLAGS += -g -Xcompiler -O0
Debug: NVCCLINKFLAGS += -g -Xcompiler -O0
Debug: OPTFLAGS += -DDEBUG -g -Wall -O0
Debug: performance-test
Debug: OUT_DIR :=$(OUT_DIR_ROOT)Debug/
Debug: EXEC:= $(OUT_DIR)performance-test
Debug: CUDAOBJS:= $(OUT_DIR)cuda_base.o

MKDIR_P = mkdir -p

all: $(EXEC)

$(EXEC): $(CUDAOBJS) cuda_link.o main.cpp
	$(CXX) $(CUDAINCLUDE) $(CXXFLAGS) $(OPTFLAGS) $(CXXLIBFLAGS) $(CUDAOBJS) $(OUT_DIR)cuda_link.o main.cpp -o $(EXEC) $(CUDALIB) -lpthread -lcurl

$(OUT_DIR)cuda_link.o : $(CUDAOBJS)
	$(NVCC) -dlink -o $(OUT_DIR)cuda_link.o $(NVCCLINKFLAGS) $(CUDAOBJS)

$(OUT_DIR)cuda_base.o : cuda_base.cu
	${MKDIR_P} ${OUT_DIR}
	$(NVCC) $(CUDAINCLUDE) $(NVCCFLAGS) -c cuda_base.cu -o $(OUT_DIR)cuda_base.o

clean:
	-rm -rf ./bin
