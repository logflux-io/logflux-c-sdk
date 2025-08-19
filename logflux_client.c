/**
 * LogFlux C SDK - Client Implementation
 * 
 * Lightweight C SDK for communicating with LogFlux Agent
 * Supports Unix socket and TCP connections
 */

/* Enable POSIX features for strdup and kill */
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "logflux_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <uuid/uuid.h>

/* Internal structures */
struct logflux_client {
    logflux_config_t config;
    int socket_fd;
    bool connected;
};

struct logflux_entry {
    char* id;
    char* message;
    char* source;
    logflux_level_t level;
    logflux_entry_type_t type;
    time_t timestamp;
    struct {
        char** keys;
        char** values;
        size_t count;
        size_t capacity;
    } labels;
};

/* Internal functions */
static logflux_error_t connect_unix_socket(logflux_client_t* client);
static logflux_error_t connect_tcp_socket(logflux_client_t* client);
static logflux_error_t send_json_data(logflux_client_t* client, const char* json);
static char* entry_to_json(const logflux_entry_t* entry, const char* shared_secret);
static char* generate_uuid(void);
static logflux_error_t set_socket_timeout(int fd, uint32_t timeout_ms);

/* Client Functions */

logflux_client_t* logflux_client_new_unix(const char* socket_path) {
    if (!socket_path) {
        return NULL;
    }
    
    logflux_client_t* client = calloc(1, sizeof(logflux_client_t));
    if (!client) {
        return NULL;
    }
    
    client->config.connection_type = LOGFLUX_CONN_UNIX;
    strncpy(client->config.socket_path, socket_path, sizeof(client->config.socket_path) - 1);
    client->config.timeout_ms = 10000;  /* 10 seconds default */
    client->config.retry_count = 3;
    client->config.retry_delay_ms = 1000;
    client->socket_fd = -1;
    
    return client;
}

logflux_client_t* logflux_client_new_tcp(const char* host, uint16_t port) {
    if (!host || port == 0) {
        return NULL;
    }
    
    logflux_client_t* client = calloc(1, sizeof(logflux_client_t));
    if (!client) {
        return NULL;
    }
    
    client->config.connection_type = LOGFLUX_CONN_TCP;
    strncpy(client->config.host, host, sizeof(client->config.host) - 1);
    client->config.port = port;
    client->config.timeout_ms = 10000;  /* 10 seconds default */
    client->config.retry_count = 3;
    client->config.retry_delay_ms = 1000;
    client->socket_fd = -1;
    
    /* Auto-load shared secret for TCP connections */
    logflux_load_shared_secret(client->config.shared_secret, sizeof(client->config.shared_secret));
    
    return client;
}

logflux_client_t* logflux_client_new_config(const logflux_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    logflux_client_t* client = calloc(1, sizeof(logflux_client_t));
    if (!client) {
        return NULL;
    }
    
    memcpy(&client->config, config, sizeof(logflux_config_t));
    client->socket_fd = -1;
    
    return client;
}

logflux_error_t logflux_client_connect(logflux_client_t* client) {
    if (!client) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    if (client->connected) {
        return LOGFLUX_OK;
    }
    
    logflux_error_t result;
    
    if (client->config.connection_type == LOGFLUX_CONN_UNIX) {
        result = connect_unix_socket(client);
    } else {
        result = connect_tcp_socket(client);
    }
    
    if (result == LOGFLUX_OK) {
        client->connected = true;
    }
    
    return result;
}

bool logflux_client_is_connected(const logflux_client_t* client) {
    return client && client->connected && client->socket_fd >= 0;
}

logflux_error_t logflux_client_send_log(logflux_client_t* client, const char* message) {
    if (!client || !message) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    logflux_entry_t* entry = logflux_entry_new(message);
    if (!entry) {
        return LOGFLUX_ERROR_MEMORY;
    }
    
    logflux_error_t result = logflux_client_send_entry(client, entry);
    logflux_entry_free(entry);
    
    return result;
}

logflux_error_t logflux_client_send_entry(logflux_client_t* client, const logflux_entry_t* entry) {
    if (!client || !entry) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    if (!client->connected) {
        return LOGFLUX_ERROR_NOT_CONNECTED;
    }
    
    const char* shared_secret = (client->config.connection_type == LOGFLUX_CONN_TCP) 
                               ? client->config.shared_secret : NULL;
    
    char* json = entry_to_json(entry, shared_secret);
    if (!json) {
        return LOGFLUX_ERROR_MEMORY;
    }
    
    logflux_error_t result = send_json_data(client, json);
    free(json);
    
    return result;
}

logflux_error_t logflux_client_send_batch(logflux_client_t* client, const logflux_entry_t* entries, size_t count) {
    if (!client || !entries || count == 0) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    if (!client->connected) {
        return LOGFLUX_ERROR_NOT_CONNECTED;
    }
    
    /* For simplicity, send entries one by one */
    /* In a full implementation, you would create a batch JSON object */
    for (size_t i = 0; i < count; i++) {
        logflux_error_t result = logflux_client_send_entry(client, &entries[i]);
        if (result != LOGFLUX_OK) {
            return result;
        }
    }
    
    return LOGFLUX_OK;
}

logflux_error_t logflux_client_close(logflux_client_t* client) {
    if (!client) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }
    
    client->connected = false;
    return LOGFLUX_OK;
}

void logflux_client_free(logflux_client_t* client) {
    if (client) {
        logflux_client_close(client);
        free(client);
    }
}

const char* logflux_error_string(logflux_error_t error) {
    switch (error) {
        case LOGFLUX_OK: return "Success";
        case LOGFLUX_ERROR_INVALID_PARAM: return "Invalid parameter";
        case LOGFLUX_ERROR_MEMORY: return "Memory allocation error";
        case LOGFLUX_ERROR_CONNECTION: return "Connection error";
        case LOGFLUX_ERROR_TIMEOUT: return "Timeout";
        case LOGFLUX_ERROR_FORMAT: return "Format error";
        case LOGFLUX_ERROR_NOT_CONNECTED: return "Not connected";
        default: return "Unknown error";
    }
}

/* Log Entry Functions */

logflux_entry_t* logflux_entry_new(const char* message) {
    if (!message) {
        return NULL;
    }
    
    logflux_entry_t* entry = calloc(1, sizeof(logflux_entry_t));
    if (!entry) {
        return NULL;
    }
    
    entry->id = generate_uuid();
    entry->message = strdup(message);
    entry->source = strdup("c-sdk");
    entry->level = LOGFLUX_LEVEL_INFO;
    entry->type = LOGFLUX_TYPE_LOG;
    entry->timestamp = time(NULL);
    
    if (!entry->id || !entry->message || !entry->source) {
        logflux_entry_free(entry);
        return NULL;
    }
    
    return entry;
}

logflux_error_t logflux_entry_set_level(logflux_entry_t* entry, logflux_level_t level) {
    if (!entry || level < LOGFLUX_LEVEL_EMERGENCY || level > LOGFLUX_LEVEL_DEBUG) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    entry->level = level;
    return LOGFLUX_OK;
}

logflux_error_t logflux_entry_set_type(logflux_entry_t* entry, logflux_entry_type_t type) {
    if (!entry || type < LOGFLUX_TYPE_LOG || type > LOGFLUX_TYPE_AUDIT) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    entry->type = type;
    return LOGFLUX_OK;
}

logflux_error_t logflux_entry_set_source(logflux_entry_t* entry, const char* source) {
    if (!entry || !source) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    char* new_source = strdup(source);
    if (!new_source) {
        return LOGFLUX_ERROR_MEMORY;
    }
    
    free(entry->source);
    entry->source = new_source;
    
    return LOGFLUX_OK;
}

logflux_error_t logflux_entry_set_timestamp(logflux_entry_t* entry, time_t timestamp) {
    if (!entry) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    entry->timestamp = timestamp;
    return LOGFLUX_OK;
}

logflux_error_t logflux_entry_add_label(logflux_entry_t* entry, const char* key, const char* value) {
    if (!entry || !key || !value) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    /* Expand labels array if needed */
    if (entry->labels.count >= entry->labels.capacity) {
        size_t new_capacity = entry->labels.capacity == 0 ? 4 : entry->labels.capacity * 2;
        
        char** new_keys = realloc(entry->labels.keys, new_capacity * sizeof(char*));
        char** new_values = realloc(entry->labels.values, new_capacity * sizeof(char*));
        
        if (!new_keys || !new_values) {
            free(new_keys);
            free(new_values);
            return LOGFLUX_ERROR_MEMORY;
        }
        
        entry->labels.keys = new_keys;
        entry->labels.values = new_values;
        entry->labels.capacity = new_capacity;
    }
    
    char* key_copy = strdup(key);
    char* value_copy = strdup(value);
    
    if (!key_copy || !value_copy) {
        free(key_copy);
        free(value_copy);
        return LOGFLUX_ERROR_MEMORY;
    }
    
    entry->labels.keys[entry->labels.count] = key_copy;
    entry->labels.values[entry->labels.count] = value_copy;
    entry->labels.count++;
    
    return LOGFLUX_OK;
}

void logflux_entry_free(logflux_entry_t* entry) {
    if (entry) {
        free(entry->id);
        free(entry->message);
        free(entry->source);
        
        for (size_t i = 0; i < entry->labels.count; i++) {
            free(entry->labels.keys[i]);
            free(entry->labels.values[i]);
        }
        free(entry->labels.keys);
        free(entry->labels.values);
        
        free(entry);
    }
}

/* Utility Functions */

logflux_error_t logflux_load_shared_secret(char* secret, size_t size) {
    if (!secret || size == 0) {
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    
    /* Try to load shared secret from agent runtime directory */
    const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
    char secret_path[512];
    
    if (xdg_runtime) {
        snprintf(secret_path, sizeof(secret_path), "%s/logflux/agent.secret", xdg_runtime);
    } else {
        const char* home = getenv("HOME");
        if (home) {
            snprintf(secret_path, sizeof(secret_path), "%s/.logflux/runtime/agent.secret", home);
        } else {
            snprintf(secret_path, sizeof(secret_path), "/tmp/.logflux-runtime/agent.secret");
        }
    }
    
    FILE* file = fopen(secret_path, "r");
    if (!file) {
        return LOGFLUX_ERROR_CONNECTION;
    }
    
    if (!fgets(secret, size, file)) {
        fclose(file);
        return LOGFLUX_ERROR_FORMAT;
    }
    
    fclose(file);
    
    /* Remove trailing newline */
    size_t len = strlen(secret);
    if (len > 0 && secret[len - 1] == '\n') {
        secret[len - 1] = '\0';
    }
    
    return LOGFLUX_OK;
}

bool logflux_is_agent_running(void) {
    /* Check if agent PID file exists and process is running */
    const char* xdg_runtime = getenv("XDG_RUNTIME_DIR");
    char pid_path[512];
    
    if (xdg_runtime) {
        snprintf(pid_path, sizeof(pid_path), "%s/logflux/agent.pid", xdg_runtime);
    } else {
        const char* home = getenv("HOME");
        if (home) {
            snprintf(pid_path, sizeof(pid_path), "%s/.logflux/runtime/agent.pid", home);
        } else {
            snprintf(pid_path, sizeof(pid_path), "/tmp/.logflux-runtime/agent.pid");
        }
    }
    
    FILE* file = fopen(pid_path, "r");
    if (!file) {
        return false;
    }
    
    int pid;
    if (fscanf(file, "%d", &pid) != 1) {
        fclose(file);
        return false;
    }
    fclose(file);
    
    /* Check if process exists */
    return kill(pid, 0) == 0;
}

/* Internal Functions */

static logflux_error_t connect_unix_socket(logflux_client_t* client) {
    client->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client->socket_fd < 0) {
        return LOGFLUX_ERROR_CONNECTION;
    }
    
    if (set_socket_timeout(client->socket_fd, client->config.timeout_ms) != LOGFLUX_OK) {
        close(client->socket_fd);
        client->socket_fd = -1;
        return LOGFLUX_ERROR_TIMEOUT;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    /* Safe copy of socket path */
    size_t path_len = strlen(client->config.socket_path);
    if (path_len >= sizeof(addr.sun_path)) {
        close(client->socket_fd);
        client->socket_fd = -1;
        return LOGFLUX_ERROR_INVALID_PARAM;
    }
    memcpy(addr.sun_path, client->config.socket_path, path_len + 1);
    
    if (connect(client->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
        return LOGFLUX_ERROR_CONNECTION;
    }
    
    return LOGFLUX_OK;
}

static logflux_error_t connect_tcp_socket(logflux_client_t* client) {
    client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_fd < 0) {
        return LOGFLUX_ERROR_CONNECTION;
    }
    
    if (set_socket_timeout(client->socket_fd, client->config.timeout_ms) != LOGFLUX_OK) {
        close(client->socket_fd);
        client->socket_fd = -1;
        return LOGFLUX_ERROR_TIMEOUT;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(client->config.port);
    
    if (inet_pton(AF_INET, client->config.host, &addr.sin_addr) <= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
        return LOGFLUX_ERROR_CONNECTION;
    }
    
    if (connect(client->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
        return LOGFLUX_ERROR_CONNECTION;
    }
    
    return LOGFLUX_OK;
}

static logflux_error_t send_json_data(logflux_client_t* client, const char* json) {
    size_t json_len = strlen(json);
    size_t total_len = json_len + 1; /* Add newline */
    
    char* data = malloc(total_len + 1);
    if (!data) {
        return LOGFLUX_ERROR_MEMORY;
    }
    
    snprintf(data, total_len + 1, "%s\n", json);
    
    ssize_t sent = send(client->socket_fd, data, total_len, 0);
    free(data);
    
    if (sent < 0 || (size_t)sent != total_len) {
        return LOGFLUX_ERROR_CONNECTION;
    }
    
    return LOGFLUX_OK;
}

static char* entry_to_json(const logflux_entry_t* entry, const char* shared_secret) {
    /* Simple JSON generation - in production, use a proper JSON library */
    size_t buffer_size = 4096;
    char* json = malloc(buffer_size);
    if (!json) {
        return NULL;
    }
    
    int len = snprintf(json, buffer_size,
        "{"
        "\"id\":\"%s\","
        "\"message\":\"%s\","
        "\"source\":\"%s\","
        "\"entry_type\":%d,"
        "\"level\":%d,"
        "\"timestamp\":%ld",
        entry->id,
        entry->message,
        entry->source,
        entry->type,
        entry->level,
        entry->timestamp
    );
    
    /* Add shared secret for TCP connections */
    if (shared_secret && strlen(shared_secret) > 0) {
        len += snprintf(json + len, buffer_size - len, ",\"shared_secret\":\"%s\"", shared_secret);
    }
    
    /* Add labels */
    if (entry->labels.count > 0) {
        len += snprintf(json + len, buffer_size - len, ",\"labels\":{");
        for (size_t i = 0; i < entry->labels.count; i++) {
            if (i > 0) {
                len += snprintf(json + len, buffer_size - len, ",");
            }
            len += snprintf(json + len, buffer_size - len, "\"%s\":\"%s\"",
                           entry->labels.keys[i], entry->labels.values[i]);
        }
        len += snprintf(json + len, buffer_size - len, "}");
    }
    
    snprintf(json + len, buffer_size - len, "}");
    
    return json;
}

static char* generate_uuid(void) {
    uuid_t uuid;
    uuid_generate(uuid);
    
    char* uuid_str = malloc(37); /* 36 chars + null terminator */
    if (!uuid_str) {
        return NULL;
    }
    
    uuid_unparse(uuid, uuid_str);
    return uuid_str;
}

static logflux_error_t set_socket_timeout(int fd, uint32_t timeout_ms) {
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return LOGFLUX_ERROR_TIMEOUT;
    }
    
    return LOGFLUX_OK;
}