CXX=clang++
# CXX=clang++-8

CPPFLAGS=-ggdb -std=c++14 -Wall -I../src -I.

all: ttest

ttest: ttest.o millis.o timezone.o
	$(CXX) -o ttest ttest.o millis.o timezone.o -lpthread

ttest.o: ../src/SecMilli.h linux_bntp.h ../src/ntp.h
timezone.o: ../src/timezone.cpp
	$(CXX) $(CPPFLAGS) -c  ../src/timezone.cpp
