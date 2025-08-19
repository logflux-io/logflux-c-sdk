/**
 * LogFlux C SDK - Client Header
 * 
 * Lightweight C SDK for communicating with LogFlux Agent
 * Supports Unix socket and TCP connections
 */

#ifndef LOGFLUX_CLIENT_H
#define LOGFLUX_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct logflux_client logflux_client_t;
typedef struct logflux_entry logflux_entry_t;

/* Log levels (syslog compatible) */
typedef enum {
    LOGFLUX_LEVEL_EMERGENCY = 0,
    LOGFLUX_LEVEL_ALERT = 1,
    LOGFLUX_LEVEL_CRITICAL = 2,
    LOGFLUX_LEVEL_ERROR = 3,
    LOGFLUX_LEVEL_WARNING = 4,
    LOGFLUX_LEVEL_NOTICE = 5,
    LOGFLUX_LEVEL_INFO = 6,
    LOGFLUX_LEVEL_DEBUG = 7
} logflux_level_t;

/* Entry types */
typedef enum {
    LOGFLUX_TYPE_LOG = 1,
    LOGFLUX_TYPE_METRIC = 2,
    LOGFLUX_TYPE_TRACE = 3,
    LOGFLUX_TYPE_EVENT = 4,
    LOGFLUX_TYPE_AUDIT = 5
} logflux_entry_type_t;

/* Connection types */
typedef enum {
    LOGFLUX_CONN_UNIX = 0,
    LOGFLUX_CONN_TCP = 1
} logflux_connection_type_t;

/* Error codes */
typedef enum {
    LOGFLUX_OK = 0,
    LOGFLUX_ERROR_INVALID_PARAM = -1,
    LOGFLUX_ERROR_MEMORY = -2,
    LOGFLUX_ERROR_CONNECTION = -3,
    LOGFLUX_ERROR_TIMEOUT = -4,
    LOGFLUX_ERROR_FORMAT = -5,
    LOGFLUX_ERROR_NOT_CONNECTED = -6
} logflux_error_t;

/* Client configuration */
typedef struct {
    logflux_connection_type_t connection_type;
    char socket_path[256];      /* For Unix socket */
    char host[256];             /* For TCP */
    uint16_t port;              /* For TCP */
    char shared_secret[128];    /* For TCP authentication */
    uint32_t timeout_ms;        /* Connection timeout */
    uint32_t retry_count;       /* Max retry attempts */
    uint32_t retry_delay_ms;    /* Delay between retries */
} logflux_config_t;

/* Client Functions */

/**
 * Create a new LogFlux client with Unix socket connection
 * @param socket_path Path to Unix socket
 * @return Client instance or NULL on error
 */
logflux_client_t* logflux_client_new_unix(const char* socket_path);

/**
 * Create a new LogFlux client with TCP connection
 * @param host TCP host address
 * @param port TCP port number
 * @return Client instance or NULL on error
 */
logflux_client_t* logflux_client_new_tcp(const char* host, uint16_t port);

/**
 * Create a new LogFlux client with custom configuration
 * @param config Client configuration
 * @return Client instance or NULL on error
 */
logflux_client_t* logflux_client_new_config(const logflux_config_t* config);

/**
 * Connect to the LogFlux agent
 * @param client Client instance
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_client_connect(logflux_client_t* client);

/**
 * Check if client is connected
 * @param client Client instance
 * @return true if connected, false otherwise
 */
bool logflux_client_is_connected(const logflux_client_t* client);

/**
 * Send a simple log message
 * @param client Client instance
 * @param message Log message
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_client_send_log(logflux_client_t* client, const char* message);

/**
 * Send a log entry
 * @param client Client instance
 * @param entry Log entry
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_client_send_entry(logflux_client_t* client, const logflux_entry_t* entry);

/**
 * Send multiple log entries as a batch
 * @param client Client instance
 * @param entries Array of log entries
 * @param count Number of entries
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_client_send_batch(logflux_client_t* client, const logflux_entry_t* entries, size_t count);

/**
 * Close the connection
 * @param client Client instance
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_client_close(logflux_client_t* client);

/**
 * Free client resources
 * @param client Client instance
 */
void logflux_client_free(logflux_client_t* client);

/**
 * Get error string for error code
 * @param error Error code
 * @return Human-readable error string
 */
const char* logflux_error_string(logflux_error_t error);

/* Log Entry Functions */

/**
 * Create a new log entry
 * @param message Log message
 * @return Log entry or NULL on error
 */
logflux_entry_t* logflux_entry_new(const char* message);

/**
 * Set log entry level
 * @param entry Log entry
 * @param level Log level
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_entry_set_level(logflux_entry_t* entry, logflux_level_t level);

/**
 * Set log entry type
 * @param entry Log entry
 * @param type Entry type
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_entry_set_type(logflux_entry_t* entry, logflux_entry_type_t type);

/**
 * Set log entry source
 * @param entry Log entry
 * @param source Source identifier
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_entry_set_source(logflux_entry_t* entry, const char* source);

/**
 * Set log entry timestamp
 * @param entry Log entry
 * @param timestamp Unix timestamp
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_entry_set_timestamp(logflux_entry_t* entry, time_t timestamp);

/**
 * Add a label to log entry
 * @param entry Log entry
 * @param key Label key
 * @param value Label value
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_entry_add_label(logflux_entry_t* entry, const char* key, const char* value);

/**
 * Free log entry resources
 * @param entry Log entry
 */
void logflux_entry_free(logflux_entry_t* entry);

/* Utility Functions */

/**
 * Load shared secret from agent configuration
 * @param secret Buffer to store secret
 * @param size Buffer size
 * @return LOGFLUX_OK on success, error code on failure
 */
logflux_error_t logflux_load_shared_secret(char* secret, size_t size);

/**
 * Check if LogFlux agent is running
 * @return true if agent is running, false otherwise
 */
bool logflux_is_agent_running(void);

#ifdef __cplusplus
}
#endif

#endif /* LOGFLUX_CLIENT_H */