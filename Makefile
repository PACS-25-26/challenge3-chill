# Compiler for MPI
CXX = mpicxx

# Compiler flags
CXXFLAGS = -Wall -Wextra -O3 -std=c++20 -Iinclude -fopenmp
# Other useful alternatives: -O2 -DNDEBUG

# get all *.cpp files
SRCS=$(wildcard *.cpp)
# get the corresponding object files
OBJS = $(SRCS:.cpp=.o)

# set default goal for make command
.DEFAULT_GOAL = $(TARGET)

# set target
TARGET = main

# linking
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# compilation
%.o: %.cpp parameters.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# clean rule: remove object files, static libraries, executable
clean:
	-rm -f *.o *.a $(TARGET)

clean-data: clean
	find . -type f -name "*.csv" -delete
	find . -type f -name "*.vtk" -delete
	find . -type f -name "*.log" -delete
	

.PHONY: clean clean-data