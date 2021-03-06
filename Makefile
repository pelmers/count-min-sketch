
ifndef CPPC
	CPPC=g++
endif

CCFLAGS= -std=c++11 -O2

INC = -I lib

LIBS = -lOpenCL -lrt

# Check our platform and make sure we define the APPLE variable
# and set up the right compiler flags and libraries
PLATFORM = $(shell uname -s)
ifeq ($(PLATFORM), Darwin)
	CPPC = clang++
	CCFLAGS += -stdlib=libc++
	LIBS = -framework OpenCL
endif
ifeq ($(PLATFORM), MINGW64_NT-10.0)
	LIBS = "C:\Windows\System32\OpenCL.dll"
endif

all: cms cms_bench

cms: cms.cpp
	$(CPPC) $^ $(INC) $(CCFLAGS) $(LIBS) -o $@
cms_bench: cms_bench.cpp
	$(CPPC) $^ $(INC) $(CCFLAGS) $(LIBS) -o $@

clean:
	rm -f cms
	rm -f cms_bench
