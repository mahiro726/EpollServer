CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
TARGET = EpollServer
SRCS = Main.cc
OBJS = $(SRCS:.cc=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) 8080

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean run
