# --- PATHS ---
# Using the stable v3.x path found on your system
MBED_PATH = /usr/local/Cellar/mbedtls@3/3.6.5

# --- COMPILER SETTINGS ---
CXX      = g++
# Added -I for the mbedtls headers
CXXFLAGS = -std=gnu++2a -Wall -Wextra -I src -I tests \
           -I$(MBED_PATH)/include \
           -isystem /Users/maui/projects/iown-homecontrol/.pio/libdeps/heltec_wifi_lora_32_V2/RadioLib/src

# Added -L for the mbedtls library files
LDFLAGS  = -L$(MBED_PATH)/lib

# --- TARGETS ---
TARGET = tests/run_tests
SRC    = src/IoHome.cpp tests/test_IoHome.cpp tests/RadioMock.cpp
OBJ    = $(SRC:.cpp=.o)

# --- LIBRARIES ---
LIBS = -lmbedcrypto

# --- BUILD RULES ---
all: $(TARGET)

# Fixed $(OBJS) to $(OBJ) to match your variable definition
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

test: all
	./$(TARGET)
