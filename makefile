UNAME := $(shell uname)

CXX := g++
CXXFLAGS := -std=c++17 `pkg-config opencv4 --cflags`
LDLIBS := `pkg-config opencv4 --libs`

ifeq ($(UNAME), Darwin)
    # macOS向け
    CXX := clang++
endif

TARGET := sample
SRC := sample.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f $(TARGET)