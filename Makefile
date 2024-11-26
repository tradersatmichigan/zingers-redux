CXX = g++
CXXFLAGS = -O3 -g -Wpedantic -Wall -Wextra -Wsign-conversion \
					 -Wconversion -std=c++2b \
					 -flto
IFLAGS = -I/usr/local/include/uWebSockets -I/usr/local/include/uSockets \
				 -isystem /usr/local/include/glaze
LDFLAGS = uWebSockets/uSockets/*.o -lz

SRC_DIR = src
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
EXECUTABLES = $(patsubst $(SRC_DIR)/%.cpp,%,$(SRC_FILES))


all: uSockets $(EXECUTABLES)

uSockets:
	$(MAKE) -C uWebSockets/uSockets

%: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(IFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -rf $(EXECUTABLES) $(wildcard *.dSYM)
