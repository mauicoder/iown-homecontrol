# --- COMPILER SETTINGS ---
CXX      = g++
CXXFLAGS = -std=gnu++2a -Wall -Wextra -I src -I tests -isystem /Users/maui/projects/iown-homecontrol/.pio/libdeps/heltec_wifi_lora_32_V2/RadioLib/src

# --- TARGETS ---
TARGET = tests/run_tests
SRC    = src/IoHome.cpp tests/test_IoHome.cpp tests/RadioMock.cpp
OBJ    = $(SRC:.cpp=.o)

# --- BUILD RULES ---
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

test: all
	./$(TARGET)

