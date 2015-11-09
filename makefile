all: 
	g++ trab2.cpp -o trab2 -Wall -std=c++14 `pkg-config --cflags --libs libmongocxx`
