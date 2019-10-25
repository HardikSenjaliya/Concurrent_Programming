CXXFLAGS := -Wall -g -pthread 

CXX := g++
CFILES = counter.cpp
OBJS= ${CFILES:.cpp=.o}

all: counter

counter: counter.o
	$(CXX) $(CXXFLAGS)  $(OBJS) -o $@

counter.o: counter.cpp
	$(CXX) $(CXXFLAGS)  counter.cpp -c 

clean:
	-rm -f *.o
	-rm -f counter