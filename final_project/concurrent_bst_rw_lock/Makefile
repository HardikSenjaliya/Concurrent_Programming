CXXFLAGS := -Wall -g -pthread 

CXX := g++
CXXFILES = main.cpp
OBJS= ${CXXFILES:.cpp=.o}

all: main

%.o:%.cpp
	@echo "compiling" $@ "using" $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

main: main.o
	@echo "Generating Target $@"
	$(CXX) $(CXXFLAGS) $^ -o $@


clean:
	-rm -f *.o
	-rm -f main