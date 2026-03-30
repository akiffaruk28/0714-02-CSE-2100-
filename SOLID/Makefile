CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS  := $(shell pkg-config --libs gtk+-3.0)

TARGET := SmartBackup

SRCS := src/main/main.cpp         \
        src/core/backup.cpp       \
        src/core/settings.cpp     \
        src/rendering/window.cpp  \
        src/utils/fileops.cpp     \
        src/utils/utils.cpp

OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
