CXXFLAGS+=-std=c++14
LDLIBS+=-lstdc++
CXX=clang++

%: %.cpp

.PHONY: default-target
default-target: sudoku_bt-proxy
sudoku_bt-proxy: sudoku_bt

.PHONY: clean
clean:
	$(RM) sudoku_bt *.o
