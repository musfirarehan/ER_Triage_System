CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -Iinclude
HEADERS  = $(wildcard include/*.h)

all: triage run_tests

triage: src/main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) src/main.cpp -o triage

run_tests: tests/test_main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/test_main.cpp -o run_tests

test: run_tests
	./run_tests

clean:
	rm -f triage run_tests triage.exe run_tests.exe *.tmp
