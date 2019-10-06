CXXFLAGS := -Wall -g -pthread 

CXX := g++
CFILES = mysort.cpp
OBJS= ${CFILES:.cpp=.o}

all: mysort

mysort: mysort.o
	$(CXX) $(CXXFLAGS)  $(OBJS) -o $@

mysort.o: mysort.cpp
	$(CXX) $(CXXFLAGS)  mysort.cpp -c 

clean:
	-rm -f *.o
	-rm -f mysort