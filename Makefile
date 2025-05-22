NVCC=/usr/local/cuda-12.9/bin/nvcc
CXX=g++

CUDA_PATH = /usr/local/cuda-12.9
CUDAINCLUDE := -I$(CUDA_PATH)/include
CUDALIB := -L$(CUDA_PATH)/lib64 -L$(CUDA_PATH)/lib64/stubs -lcudart -lcudadevrt -lnvidia-ml

OPTFLAGS :=
CUDAGENCODE := -gencode=arch=compute_86,code=sm_86 -gencode=arch=compute_89,code=sm_89
NVCCFLAGS := $(CUDAGENCODE) -x cu -rdc=true -m64 --compile --std=c++20 -ccbin g++ -Xcompiler -fPIC
NVCCLINKFLAGS := $(CUDAGENCODE) -m64 --std=c++20 -ccbin g++ $(CUDALIB) --ptxas-options=-v --compiler-options '-fPIC'

CXXFLAGS= -Wall -g -O0 -Wextra -m64 --std=c++20 -fPIC
CXXLIBFLAGS= -lz -lpthread -fPIC -lstatgrab -lcurl
OUT_DIR_ROOT= bin/x64/
OUT_DIR :=
CUDAOBJS = cuda_base.o
OBJS:= $(OUT_DIR)singleton.o $(OUT_DIR)read_config.o $(OUT_DIR)http_request.o
EXEC:= performance-test

Release: NVCCFLAGS += -Xcompiler -O2
Release: NVCCLINKFLAGS += -Xcompiler -O2
Release: OPTFLAGS += -DRELEASE -O2
Release: performance-test
Release: OUT_DIR :=$(OUT_DIR_ROOT)Release/
Release: EXEC:= $(OUT_DIR)performance-test
Release: CUDAOBJS:= $(OUT_DIR)cuda_base.o
Release: OBJS:= $(OUT_DIR)singleton.o $(OUT_DIR)read_config.o $(OUT_DIR)http_request.o

Debug: NVCCFLAGS += -g -Xcompiler -O0
Debug: NVCCLINKFLAGS += -g -Xcompiler -O0
Debug: OPTFLAGS += -DDEBUG -g -Wall -O0
Debug: performance-test
Debug: OUT_DIR :=$(OUT_DIR_ROOT)Debug/
Debug: EXEC:= $(OUT_DIR)performance-test
Debug: CUDAOBJS:= $(OUT_DIR)cuda_base.o
Debug: OBJS:= $(OUT_DIR)singleton.o $(OUT_DIR)read_config.o $(OUT_DIR)http_request.o

MKDIR_P = mkdir -p

all: $(EXEC)

$(EXEC): $(CUDAOBJS) $(OBJS) cuda_link.o main.cpp
	$(CXX) $(CUDAINCLUDE) $(CXXFLAGS) $(OPTFLAGS) $(CUDAOBJS) $(OUT_DIR)cuda_link.o $(OBJS) main.cpp -o $(EXEC) $(CUDALIB) $(CXXLIBFLAGS)

$(OUT_DIR)singleton.o : singleton.cpp singleton.h
	$(CXX) $(CUDAINCLUDE) $(CXXFLAGS) $(OPTFLAGS) -c singleton.cpp -o $(OUT_DIR)singleton.o -fPIC -lstatgrab

$(OUT_DIR)read_config.o : read_config.cpp read_config.h
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c read_config.cpp -o $(OUT_DIR)read_config.o -fPIC

$(OUT_DIR)http_request.o : http_request.cpp http_request.h
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c http_request.cpp -o $(OUT_DIR)http_request.o -fPIC

$(OUT_DIR)cuda_link.o : $(CUDAOBJS)
	$(NVCC) -dlink -o $(OUT_DIR)cuda_link.o $(NVCCLINKFLAGS) $(CUDAOBJS)

$(OUT_DIR)cuda_base.o : cuda_base.cu
	${MKDIR_P} ${OUT_DIR}
	$(NVCC) $(CUDAINCLUDE) $(NVCCFLAGS) -c cuda_base.cu -o $(OUT_DIR)cuda_base.o

clean:
	-rm -rf ./bin
