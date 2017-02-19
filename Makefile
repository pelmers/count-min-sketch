
ifndef CPPC
	CPPC=g++
endif

CCFLAGS= -std=c++11

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

cms: cms.cpp
	$(CPPC) $^ $(INC) $(CCFLAGS) $(LIBS) -o $@


clean:
	rm -f cms
