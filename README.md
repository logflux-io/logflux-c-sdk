# LogFlux C SDK

Official C SDK for LogFlux Agent - A lightweight, high-performance log collection and forwarding agent.

## Features

- Lightweight client for communicating with LogFlux Agent local server
- Support for both Unix socket and TCP connections
- Automatic retry logic with configurable timeouts
- Thread-safe operations (when used properly)
- Zero external dependencies (except libuuid)
- ANSI C99 compatible
- Comprehensive error handling

## Requirements

- C99 compatible compiler (GCC, Clang)
- libuuid (usually part of util-linux package)
- POSIX-compliant system (Linux, macOS)

## Installation

### From GitHub Releases

Download pre-built libraries from the [GitHub Releases](https://github.com/logflux-io/logflux-c-sdk/releases) page:

```bash
# Download the appropriate package for your architecture
wget https://github.com/logflux-io/logflux-c-sdk/releases/download/c-sdk-v1.0.0/logflux-c-sdk-1.0.0-linux-x64.tar.gz

# Extract the package
tar -xzf logflux-c-sdk-1.0.0-linux-x64.tar.gz

# Install headers and libraries
sudo cp logflux-c-sdk-*/include/* /usr/local/include/
sudo cp logflux-c-sdk-*/lib/* /usr/local/lib/
sudo ldconfig
```

### Using in Your Project

#### Method 1: System Installation
```bash
# After installation
gcc -o myapp myapp.c -llogflux -luuid
```

#### Method 2: Static Linking
```bash
# Copy logflux_client.h and logflux_client.c to your project
gcc -o myapp myapp.c logflux_client.c -luuid
```

## Quick Start

### Basic Unix Socket Example

```c
#include "logflux_client.h"
#include <stdio.h>

int main() {
    // Create client for Unix socket connection
    logflux_client_t* client = logflux_client_new_unix("/tmp/logflux-agent.sock");
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        return 1;
    }
    
    // Connect to agent
    logflux_error_t result = logflux_client_connect(client);
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "Failed to connect: %s\n", logflux_error_string(result));
        logflux_client_free(client);
        return 1;
    }
    
    // Send a log message
    result = logflux_client_send_log(client, "Hello from LogFlux C SDK!");
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "Failed to send log: %s\n", logflux_error_string(result));
    }
    
    // Clean up
    logflux_client_close(client);
    logflux_client_free(client);
    
    return 0;
}
```

### Structured Log Entry Example

```c
#include "logflux_client.h"
#include <stdio.h>

int main() {
    logflux_client_t* client = logflux_client_new_unix("/tmp/logflux-agent.sock");
    if (!client || logflux_client_connect(client) != LOGFLUX_OK) {
        fprintf(stderr, "Connection failed\n");
        return 1;
    }
    
    // Create structured log entry
    logflux_entry_t* entry = logflux_entry_new("Application started");
    if (entry) {
        logflux_entry_set_level(entry, LOGFLUX_LEVEL_INFO);
        logflux_entry_set_source(entry, "my-app");
        logflux_entry_add_label(entry, "component", "main");
        logflux_entry_add_label(entry, "version", "1.0.0");
        
        logflux_error_t result = logflux_client_send_entry(client, entry);
        if (result != LOGFLUX_OK) {
            fprintf(stderr, "Failed to send entry: %s\n", logflux_error_string(result));
        }
        
        logflux_entry_free(entry);
    }
    
    logflux_client_close(client);
    logflux_client_free(client);
    return 0;
}
```

### TCP Connection Example

```c
#include "logflux_client.h"
#include <stdio.h>

int main() {
    // Create TCP client (will auto-load shared secret)
    logflux_client_t* client = logflux_client_new_tcp("127.0.0.1", 8080);
    if (!client) {
        fprintf(stderr, "Failed to create TCP client\n");
        return 1;
    }
    
    logflux_error_t result = logflux_client_connect(client);
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "TCP connection failed: %s\n", logflux_error_string(result));
        logflux_client_free(client);
        return 1;
    }
    
    // Send authenticated log message
    result = logflux_client_send_log(client, "Hello via TCP!");
    if (result == LOGFLUX_OK) {
        printf("Log sent successfully via TCP\n");
    }
    
    logflux_client_close(client);
    logflux_client_free(client);
    return 0;
}
```

## Building from Source

The C SDK uses GitHub Actions for all builds and testing. To build from source locally:

```bash
# Clone the repository
git clone https://github.com/logflux-io/logflux-c-sdk.git
cd logflux-c-sdk

# Build static library
gcc -std=c99 -Wall -Wextra -O2 -fPIC -c logflux_client.c -o logflux_client.o
ar rcs liblogflux.a logflux_client.o

# Build shared library
gcc -std=c99 -Wall -Wextra -O2 -fPIC -shared logflux_client.c -o liblogflux.so -luuid -lpthread

# Build and run tests
gcc -std=c99 -Wall -Wextra -O2 -o test_client tests/test_client.c logflux_client.c -luuid -lpthread
./test_client

# Build example
gcc -std=c99 -Wall -Wextra -O2 -o basic_example examples/basic.c logflux_client.c -luuid -lpthread
./basic_example
```

### CI/CD Pipeline

All builds, tests, and releases are automated through GitHub Actions:

- **Testing**: Automated unit tests, memory leak detection, static analysis
- **Building**: Multi-architecture builds (x64, arm64, armv7)
- **Releases**: Automated packaging and distribution via GitHub Releases

See `.github/workflows/` for the complete CI/CD configuration.

## API Reference

### Client Functions

| Function | Description |
|----------|-------------|
| `logflux_client_new_unix(path)` | Create Unix socket client |
| `logflux_client_new_tcp(host, port)` | Create TCP client |
| `logflux_client_new_config(config)` | Create client with custom config |
| `logflux_client_connect(client)` | Connect to agent |
| `logflux_client_is_connected(client)` | Check connection status |
| `logflux_client_send_log(client, message)` | Send simple log message |
| `logflux_client_send_entry(client, entry)` | Send structured log entry |
| `logflux_client_send_batch(client, entries, count)` | Send multiple entries |
| `logflux_client_close(client)` | Close connection |
| `logflux_client_free(client)` | Free client resources |

### Log Entry Functions

| Function | Description |
|----------|-------------|
| `logflux_entry_new(message)` | Create new log entry |
| `logflux_entry_set_level(entry, level)` | Set log level |
| `logflux_entry_set_type(entry, type)` | Set entry type |
| `logflux_entry_set_source(entry, source)` | Set source identifier |
| `logflux_entry_set_timestamp(entry, timestamp)` | Set timestamp |
| `logflux_entry_add_label(entry, key, value)` | Add label |
| `logflux_entry_free(entry)` | Free entry resources |

### Log Levels

- `LOGFLUX_LEVEL_EMERGENCY` (0)
- `LOGFLUX_LEVEL_ALERT` (1)
- `LOGFLUX_LEVEL_CRITICAL` (2)
- `LOGFLUX_LEVEL_ERROR` (3)
- `LOGFLUX_LEVEL_WARNING` (4)
- `LOGFLUX_LEVEL_NOTICE` (5)
- `LOGFLUX_LEVEL_INFO` (6)
- `LOGFLUX_LEVEL_DEBUG` (7)

### Entry Types

- `LOGFLUX_TYPE_LOG` (1)
- `LOGFLUX_TYPE_METRIC` (2)
- `LOGFLUX_TYPE_TRACE` (3)
- `LOGFLUX_TYPE_EVENT` (4)
- `LOGFLUX_TYPE_AUDIT` (5)

### Error Codes

- `LOGFLUX_OK` (0) - Success
- `LOGFLUX_ERROR_INVALID_PARAM` (-1) - Invalid parameter
- `LOGFLUX_ERROR_MEMORY` (-2) - Memory allocation error
- `LOGFLUX_ERROR_CONNECTION` (-3) - Connection error
- `LOGFLUX_ERROR_TIMEOUT` (-4) - Timeout
- `LOGFLUX_ERROR_FORMAT` (-5) - Format error
- `LOGFLUX_ERROR_NOT_CONNECTED` (-6) - Not connected

## Configuration

### Custom Configuration Example

```c
logflux_config_t config = {0};
config.connection_type = LOGFLUX_CONN_UNIX;
strcpy(config.socket_path, "/tmp/logflux-agent.sock");
config.timeout_ms = 5000;        // 5 second timeout
config.retry_count = 5;          // Max 5 retries
config.retry_delay_ms = 2000;    // 2 second delay between retries

logflux_client_t* client = logflux_client_new_config(&config);
```

## Thread Safety

The LogFlux C SDK is **not thread-safe** by default. If you need to use it in a multi-threaded environment:

1. Use separate client instances per thread, OR
2. Protect shared client instances with mutexes

## Error Handling

Always check return codes:

```c
logflux_error_t result = logflux_client_connect(client);
if (result != LOGFLUX_OK) {
    fprintf(stderr, "Connection failed: %s\n", logflux_error_string(result));
    // Handle error...
}
```

## Memory Management

- Always call `logflux_client_free()` to free client resources
- Always call `logflux_entry_free()` to free log entry resources
- The SDK manages internal memory automatically
- Passing NULL to free functions is safe (no-op)

## Testing

The SDK includes comprehensive tests that are automatically run via GitHub Actions:

- **Unit Tests**: Core functionality testing
- **Memory Tests**: Valgrind memory leak detection
- **Static Analysis**: cppcheck and compiler warnings
- **Sanitizers**: AddressSanitizer and UndefinedBehaviorSanitizer
- **Cross-compilation**: Testing on multiple architectures

To run tests manually:

```bash
# Compile and run tests
gcc -std=c99 -Wall -Wextra -O2 -o test_client tests/test_client.c logflux_client.c -luuid -lpthread
./test_client
```

## License

This SDK is distributed under the Apache License, Version 2.0. See the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

For issues and questions, please use the GitHub issue tracker.