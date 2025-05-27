NVCC=/usr/local/cuda-12.9/bin/nvcc
CXX=g++

CUDA_PATH = /usr/local/cuda-12.9
CUDAINCLUDE := -I$(CUDA_PATH)/include
CUDALIB := -L$(CUDA_PATH)/lib64 -L$(CUDA_PATH)/lib64/stubs -lcudart -lcudadevrt -lnvidia-ml

OPTFLAGS :=
CUDAGENCODE := -gencode=arch=compute_89,code=sm_89
NVCCFLAGS := $(CUDAGENCODE) -x cu -rdc=true -m64 --compile --std=c++20 -ccbin g++ -Xcompiler -fPIC
NVCCLINKFLAGS := $(CUDAGENCODE) -m64 --std=c++20 -ccbin g++ $(CUDALIB) --ptxas-options=-v --compiler-options '-fPIC'

CXXFLAGS= -Wall -g -O0 -Wextra -m64 --std=c++20 -fPIC
CXXLIBFLAGS= -lz -lpthread -fPIC -lstatgrab -lcurl
OUT_DIR_ROOT= bin/x64/
OUT_DIR :=
CUDAOBJS = cuda_base.o
OBJS:= $(OUT_DIR)singleton.o $(OUT_DIR)read_config.o $(OUT_DIR)http_request.o $(OUT_DIR)performance_test.o
EXEC:= performance-test

Release: NVCCFLAGS += -Xcompiler -O2
Release: NVCCLINKFLAGS += -Xcompiler -O2
Release: OPTFLAGS += -DRELEASE -O2
Release: performance-test
Release: OUT_DIR :=$(OUT_DIR_ROOT)Release/
Release: EXEC:= $(OUT_DIR)performance-test
Release: CUDAOBJS:= $(OUT_DIR)cuda_base.o
Release: OBJS:= $(OUT_DIR)singleton.o $(OUT_DIR)read_config.o $(OUT_DIR)http_request.o $(OUT_DIR)performance_test.o

Debug: NVCCFLAGS += -g -Xcompiler -O0
Debug: NVCCLINKFLAGS += -g -Xcompiler -O0
Debug: OPTFLAGS += -DDEBUG -g -Wall -O0
Debug: performance-test
Debug: OUT_DIR :=$(OUT_DIR_ROOT)Debug/
Debug: EXEC:= $(OUT_DIR)performance-test
Debug: CUDAOBJS:= $(OUT_DIR)cuda_base.o
Debug: OBJS:= $(OUT_DIR)singleton.o $(OUT_DIR)read_config.o $(OUT_DIR)http_request.o $(OUT_DIR)performance_test.o

MKDIR_P = mkdir -p

all: $(EXEC)

$(EXEC): $(CUDAOBJS) $(OBJS) $(OUT_DIR)cuda_link.o src/main.cpp
	$(CXX) $(CUDAINCLUDE) $(CXXFLAGS) $(OPTFLAGS) $(CUDAOBJS) $(OUT_DIR)cuda_link.o $(OBJS) src/main.cpp -o $(EXEC) $(CUDALIB) $(CXXLIBFLAGS)

$(OUT_DIR)performance_test.o : src/performance_test.cpp include/performance_test.h
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c src/performance_test.cpp -o $(OUT_DIR)performance_test.o -fPIC

$(OUT_DIR)singleton.o : src/singleton.cpp include/singleton.h
	$(CXX) $(CUDAINCLUDE) $(CXXFLAGS) $(OPTFLAGS) -c src/singleton.cpp -o $(OUT_DIR)singleton.o -fPIC -lstatgrab

$(OUT_DIR)read_config.o : src/read_config.cpp include/read_config.h
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c src/read_config.cpp -o $(OUT_DIR)read_config.o -fPIC

$(OUT_DIR)http_request.o : src/http_request.cpp include/http_request.h
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c src/http_request.cpp -o $(OUT_DIR)http_request.o -fPIC

$(OUT_DIR)cuda_link.o : $(CUDAOBJS)
	$(NVCC) -dlink -o $(OUT_DIR)cuda_link.o $(NVCCLINKFLAGS) $(CUDAOBJS)

$(OUT_DIR)cuda_base.o : src/cuda_base.cu
	${MKDIR_P} ${OUT_DIR}
	$(NVCC) $(CUDAINCLUDE) $(NVCCFLAGS) -c src/cuda_base.cu -o $(OUT_DIR)cuda_base.o

clean:
	-rm -rf ./bin
