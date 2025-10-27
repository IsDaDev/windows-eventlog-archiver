# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS :=

# Directories
NATIVE_DIR := native

# Source files
SOURCES := $(NATIVE_DIR)/evtx2json.cpp $(NATIVE_DIR)/readEvtx.cpp
TARGETS := $(NATIVE_DIR)/evtx2json.exe $(NATIVE_DIR)/readEvtx.exe

# Default target
all: $(TARGETS)

# Build evtx2json
$(NATIVE_DIR)/evtx2json.exe: $(NATIVE_DIR)/evtx2json.cpp
	$(CXX) $(CXXFLAGS) -I$(NATIVE_DIR)/headers $< -o $@ $(LDFLAGS)

# Build readEvtx
$(NATIVE_DIR)/readEvtx.exe: $(NATIVE_DIR)/readEvtx.cpp
	$(CXX) $(CXXFLAGS) -I$(NATIVE_DIR)/headers $< -o $@ $(LDFLAGS) -lwevtapi

# Clean build artifacts
clean:
	rm -f $(TARGETS)

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build both binaries"
	@echo "  clean    - Remove binaries"
	@echo "  evtx2json - Build only evtx2json"
	@echo "  readEvtx  - Build only readEvtx"

# Individual targets
evtx2json: $(NATIVE_DIR)/evtx2json.exe
readEvtx: $(NATIVE_DIR)/readEvtx.exe

.PHONY: all clean help evtx2json readEvtx