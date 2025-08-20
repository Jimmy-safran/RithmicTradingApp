# Simple Makefile for RithmicTradingApp project
CXX = g++
CXXFLAGS = -O3 -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing -std=c++11
INCLUDES = -I./include -I../rapi-sdk/13.5.0.0/include

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    TARGET = bin/RithmicTradingApp.exe
    LIBS = -L../rapi-sdk/13.5.0.0/win10/lib -lRApiPlus_md64 -lOmneStreamEngine_md64 -lOmneChannel_md64 -lOmneEngine_md64 -lapi_md64 -lapistb_md64 -lkit_md64 -llibssl_md64 -llibcrypto_md64 -lzlib_md64 -lws2_32 -liphlpapi -lcrypt32
    CXXFLAGS += -DWIN32 -D_WINDOWS
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        TARGET = bin/RithmicTradingApp
        LIBS = -L../rapi-sdk/13.5.0.0/darwin-10/lib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -lz -Wl,-search_paths_first
    else
        TARGET = bin/RithmicTradingApp
        LIBS = -L../rapi-sdk/13.5.0.0/linux-gnu-4.18-x86_64/lib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -lz -lpthread -lrt -ldl
    endif
endif

SOURCES = src/RithmicTradingApp.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LIBS)
	@echo "✓ Built $(TARGET) successfully"

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "✓ Cleaned build artifacts"

# Run the program
run: $(TARGET)
	./$(TARGET)

# Help
help:
	@echo "Available targets:"
	@echo "  all   - Build the RithmicTradingApp executable"
	@echo "  clean - Clean build artifacts"
	@echo "  run   - Build and run the program"
	@echo "  help  - Show this help"

.PHONY: all clean run help
