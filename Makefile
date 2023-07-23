GCC_FLAGS = -std=c99 -Wall -Wextra
CLANG_FLAGS = -std=c99 -Weverything -Wno-vla -Wno-self-assign

#GCC_CXXFLAGS = -DMESSAGE='"Compiled with GCC"'
#CLANG_CXXFLAGS = -DMESSAGE='"Compiled with Clang"'
#UNKNOWN_CXXFLAGS = -DMESSAGE='"Compiled with UNKNOWN compiler"'

ifeq ($(CXX),gcc)
	CXX = gcc
	CXXFLAGS = $(GCC_FLAGS)
	CXXFLAGS += $(GCC_CXXFLAGS)
else ifeq ($(CXX),clang)
	CXX = clang
	CXXFLAGS = $(CLANG_FLAGS)
	CXXFLAGS += $(CLANG_CXXFLAGS)
else
	CXX = gcc
	CXXFLAGS = $(GCC_FLAGS)
	CXXFLAGS += $(GCC_CXXFLAGS)
endif

CXXFLAGS += -I.
LDFLAGS = -pthread
#LDFLAGS += -time
binaries = main test

all: main.c 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o main main.c
	./main
	make clean

test: test.c 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o test test.c
	./test
	make clean

clean:
	rm -f $(binaries) *.o