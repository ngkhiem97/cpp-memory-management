# run this before compiling with -std=c++20: module load gcc-11.2.0
# run the command below before executing the program (load the shared library):
#  export LD_LIBRARY_PATH=/opt/tbb-2021.3.0/lib/intel64/gcc4.8:$LD_LIBRARY_PATH
# tbb Include directory: -I/opt/tbb-2021.3.0/include
# tbb Linking directory: -L/opt/tbb-2021.3.0/lib/intel64/gcc4.8

CXX		 = g++
CXXFLAGS = -Wall -g -O0 -std=c++14 -pthread
TARGET	 = numa 
SRC	 = numa.cc
INCLUDE	 = -I/opt/tbb-2021.3.0/include
LIB	     = -L/opt/tbb-2021.3.0/lib/intel64/gcc4.8
TBB      = -ltbb

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LIB) $(SRC) -o $(TARGET) $(TBB) -lhwloc

clean:
	rm -f $(TARGET)