CXXFLAGS := -Wall -g -pthread 

CXX := g++
CXXFILES = mysort.cpp locks.cpp
OBJS= ${CXXFILES:.cpp=.o}

all: mysort

%.o:%.cpp
	@echo "compiling" $@ "using" $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

mysort: mysort.o
	@echo "Generating Target $@"
	$(CXX) $(CXXFLAGS) $^ -o $@


clean:
	-rm -f *.o
	-rm -f mysort