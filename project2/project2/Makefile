CXXFLAGS := -Og -g -Wall -std=c++11 -lm
BINARY := schedulersim

all: $(BINARY)

schedulersim: schedulersim.o schedulersim_driver.o
	$(CXX) -o schedulersim schedulersim.o schedulersim_driver.o

clean:
	rm -rf $(BINARY) *.o myoutput
