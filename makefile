all: 
	g++ trab_twitter.cpp -o trab_twitter -Wall -std=c++14 -O3 `pkg-config --cflags --libs libmongocxx` -lcurl
