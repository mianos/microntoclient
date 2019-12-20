CXX=clang++-8

CPPFLAGS= -ggdb -std=c++14 -Wall

all: ttest

ttest: ttest.o millis.o
	$(CXX) -o ttest ttest.o millis.o -lpthread

ttest.o: SecMilli.h linux_bntp.h ntp.h
