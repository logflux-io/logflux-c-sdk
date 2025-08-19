# LogFlux C SDK Makefile
#
# This Makefile provides common build targets for the LogFlux C SDK

# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -fPIC
INCLUDES = -I.

# Library dependencies
LIBS = -luuid -lpthread

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - use Homebrew UUID
    UUID_CONFIG = $(shell which uuid-config 2>/dev/null)
    ifneq ($(UUID_CONFIG),)
        UUID_CFLAGS = $(shell uuid-config --cflags)
        UUID_LDFLAGS = $(shell uuid-config --ldflags)
    else
        $(error UUID library not found. Install with: brew install ossp-uuid)
    endif
else
    # Linux - use system UUID
    UUID_CFLAGS = 
    UUID_LDFLAGS = 
endif

# Source files
SDK_SRC = logflux_client.c
SDK_HDR = logflux_client.h
TEST_SRC = tests/test_client.c
EXAMPLE_SRC = examples/basic.c

# Output files
STATIC_LIB = liblogflux.a
SHARED_LIB = liblogflux.so
TEST_BIN = test_client
EXAMPLE_BIN = basic_example

# Build directories
BUILD_DIR = build
DIST_DIR = dist

.PHONY: all clean test example static shared check install uninstall help

# Default target
all: static shared test example

# Static library
static: $(STATIC_LIB)

$(STATIC_LIB): $(SDK_SRC) $(SDK_HDR)
	$(CC) $(CFLAGS) $(INCLUDES) $(UUID_CFLAGS) -c $(SDK_SRC) -o logflux_client.o
	ar rcs $(STATIC_LIB) logflux_client.o
	@echo "Static library built: $(STATIC_LIB)"

# Shared library
shared: $(SHARED_LIB)

$(SHARED_LIB): $(SDK_SRC) $(SDK_HDR)
	$(CC) $(CFLAGS) $(INCLUDES) $(UUID_CFLAGS) -shared $(SDK_SRC) -o $(SHARED_LIB) $(UUID_LDFLAGS) $(LIBS)
	@echo "Shared library built: $(SHARED_LIB)"

# Test binary
test: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(SDK_SRC) $(SDK_HDR)
	$(CC) $(CFLAGS) $(INCLUDES) $(UUID_CFLAGS) -o $(TEST_BIN) $(TEST_SRC) $(SDK_SRC) $(UUID_LDFLAGS) $(LIBS)
	@echo "Test binary built: $(TEST_BIN)"

# Example binary
example: $(EXAMPLE_BIN)

$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(SDK_SRC) $(SDK_HDR)
	$(CC) $(CFLAGS) $(INCLUDES) $(UUID_CFLAGS) -o $(EXAMPLE_BIN) $(EXAMPLE_SRC) $(SDK_SRC) $(UUID_LDFLAGS) $(LIBS)
	@echo "Example binary built: $(EXAMPLE_BIN)"

# Run tests
check: $(TEST_BIN)
	@echo "Running C SDK tests..."
	./$(TEST_BIN)

# Install system-wide (requires sudo on most systems)
install: $(STATIC_LIB) $(SHARED_LIB) $(SDK_HDR)
	@echo "Installing LogFlux C SDK..."
	install -d /usr/local/lib
	install -d /usr/local/include
	install -m 644 $(STATIC_LIB) /usr/local/lib/
	install -m 755 $(SHARED_LIB) /usr/local/lib/
	install -m 644 $(SDK_HDR) /usr/local/include/
	@echo "LogFlux C SDK installed to /usr/local"

# Uninstall from system
uninstall:
	@echo "Uninstalling LogFlux C SDK..."
	rm -f /usr/local/lib/$(STATIC_LIB)
	rm -f /usr/local/lib/$(SHARED_LIB)
	rm -f /usr/local/include/$(SDK_HDR)
	@echo "LogFlux C SDK uninstalled"

# Create distribution package
dist: clean all
	@mkdir -p $(DIST_DIR)/logflux-c-sdk
	@mkdir -p $(DIST_DIR)/logflux-c-sdk/{lib,include,examples,docs}
	@cp $(STATIC_LIB) $(SHARED_LIB) $(DIST_DIR)/logflux-c-sdk/lib/
	@cp $(SDK_HDR) $(DIST_DIR)/logflux-c-sdk/include/
	@cp -r examples/* $(DIST_DIR)/logflux-c-sdk/examples/
	@cp README.md $(DIST_DIR)/logflux-c-sdk/docs/
	@echo "Distribution package created in $(DIST_DIR)/logflux-c-sdk"

# Clean build artifacts
clean:
	rm -f *.o *.a *.so $(TEST_BIN) $(EXAMPLE_BIN)
	rm -rf $(BUILD_DIR) $(DIST_DIR)
	@echo "Build artifacts cleaned"

# Help target
help:
	@echo "LogFlux C SDK Makefile"
	@echo "======================"
	@echo ""
	@echo "Available targets:"
	@echo "  all       - Build static library, shared library, test, and example"
	@echo "  static    - Build static library (liblogflux.a)"
	@echo "  shared    - Build shared library (liblogflux.so)"
	@echo "  test      - Build test binary"
	@echo "  example   - Build example binary"
	@echo "  check     - Run test suite"
	@echo "  install   - Install libraries and headers system-wide"
	@echo "  uninstall - Remove installed libraries and headers"
	@echo "  dist      - Create distribution package"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make all      # Build everything"
	@echo "  make check    # Run tests"
	@echo "  make clean    # Clean up"