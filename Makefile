
all: build

build: pp_top_parker.cc
	g++ -Wall -g -o pp_top pp_top_parker.cc

clean:
	-rm pp_top
