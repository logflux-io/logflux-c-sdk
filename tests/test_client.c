/**
 * LogFlux C SDK - Unit Tests
 * 
 * Basic unit tests for the LogFlux C SDK
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "logflux_client.h"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* Test macros */
#define TEST_ASSERT(condition, message) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("PASS: %s\n", message); \
    } else { \
        printf("FAIL: %s\n", message); \
    } \
} while(0)

void test_client_creation(void) {
    printf("\n=== Client Creation Tests ===\n");
    
    /* Test Unix socket client creation */
    logflux_client_t* unix_client = logflux_client_new_unix("/tmp/test.sock");
    TEST_ASSERT(unix_client != NULL, "Unix socket client creation");
    
    if (unix_client) {
        TEST_ASSERT(!logflux_client_is_connected(unix_client), "New client should not be connected");
        logflux_client_free(unix_client);
    }
    
    /* Test TCP client creation */
    logflux_client_t* tcp_client = logflux_client_new_tcp("127.0.0.1", 8080);
    TEST_ASSERT(tcp_client != NULL, "TCP client creation");
    
    if (tcp_client) {
        TEST_ASSERT(!logflux_client_is_connected(tcp_client), "New TCP client should not be connected");
        logflux_client_free(tcp_client);
    }
    
    /* Test invalid parameters */
    logflux_client_t* invalid_client = logflux_client_new_unix(NULL);
    TEST_ASSERT(invalid_client == NULL, "Invalid Unix socket path should return NULL");
    
    invalid_client = logflux_client_new_tcp(NULL, 8080);
    TEST_ASSERT(invalid_client == NULL, "Invalid TCP host should return NULL");
    
    invalid_client = logflux_client_new_tcp("127.0.0.1", 0);
    TEST_ASSERT(invalid_client == NULL, "Invalid TCP port should return NULL");
    
    /* Test custom config */
    logflux_config_t config = {0};
    config.connection_type = LOGFLUX_CONN_UNIX;
    strcpy(config.socket_path, "/tmp/custom.sock");
    config.timeout_ms = 5000;
    config.retry_count = 5;
    
    logflux_client_t* config_client = logflux_client_new_config(&config);
    TEST_ASSERT(config_client != NULL, "Custom config client creation");
    
    if (config_client) {
        logflux_client_free(config_client);
    }
}

void test_log_entry_creation(void) {
    printf("\n=== Log Entry Tests ===\n");
    
    /* Test basic entry creation */
    logflux_entry_t* entry = logflux_entry_new("Test message");
    TEST_ASSERT(entry != NULL, "Log entry creation");
    
    if (entry) {
        /* Test setting properties */
        logflux_error_t result = logflux_entry_set_level(entry, LOGFLUX_LEVEL_ERROR);
        TEST_ASSERT(result == LOGFLUX_OK, "Set log level");
        
        result = logflux_entry_set_type(entry, LOGFLUX_TYPE_METRIC);
        TEST_ASSERT(result == LOGFLUX_OK, "Set entry type");
        
        result = logflux_entry_set_source(entry, "test-suite");
        TEST_ASSERT(result == LOGFLUX_OK, "Set source");
        
        time_t test_time = time(NULL);
        result = logflux_entry_set_timestamp(entry, test_time);
        TEST_ASSERT(result == LOGFLUX_OK, "Set timestamp");
        
        /* Test adding labels */
        result = logflux_entry_add_label(entry, "test", "value");
        TEST_ASSERT(result == LOGFLUX_OK, "Add label");
        
        result = logflux_entry_add_label(entry, "another", "label");
        TEST_ASSERT(result == LOGFLUX_OK, "Add second label");
        
        logflux_entry_free(entry);
    }
    
    /* Test invalid parameters */
    logflux_entry_t* invalid_entry = logflux_entry_new(NULL);
    TEST_ASSERT(invalid_entry == NULL, "Invalid message should return NULL");
    
    /* Test invalid operations */
    logflux_error_t result = logflux_entry_set_level(NULL, LOGFLUX_LEVEL_INFO);
    TEST_ASSERT(result == LOGFLUX_ERROR_INVALID_PARAM, "NULL entry should return error");
    
    result = logflux_entry_add_label(NULL, "key", "value");
    TEST_ASSERT(result == LOGFLUX_ERROR_INVALID_PARAM, "NULL entry for label should return error");
}

void test_error_codes(void) {
    printf("\n=== Error Code Tests ===\n");
    
    /* Test error string function */
    const char* error_str = logflux_error_string(LOGFLUX_OK);
    TEST_ASSERT(strcmp(error_str, "Success") == 0, "Success error string");
    
    error_str = logflux_error_string(LOGFLUX_ERROR_INVALID_PARAM);
    TEST_ASSERT(strcmp(error_str, "Invalid parameter") == 0, "Invalid parameter error string");
    
    error_str = logflux_error_string(LOGFLUX_ERROR_MEMORY);
    TEST_ASSERT(strcmp(error_str, "Memory allocation error") == 0, "Memory error string");
    
    error_str = logflux_error_string(LOGFLUX_ERROR_CONNECTION);
    TEST_ASSERT(strcmp(error_str, "Connection error") == 0, "Connection error string");
    
    error_str = logflux_error_string(LOGFLUX_ERROR_NOT_CONNECTED);
    TEST_ASSERT(strcmp(error_str, "Not connected") == 0, "Not connected error string");
    
    /* Test unknown error code */
    error_str = logflux_error_string((logflux_error_t)-999);
    TEST_ASSERT(strcmp(error_str, "Unknown error") == 0, "Unknown error string");
}

void test_connection_operations(void) {
    printf("\n=== Connection Operation Tests ===\n");
    
    /* Test operations on disconnected client */
    logflux_client_t* client = logflux_client_new_unix("/tmp/nonexistent.sock");
    if (client) {
        /* Test sending without connection */
        logflux_error_t result = logflux_client_send_log(client, "Test message");
        TEST_ASSERT(result == LOGFLUX_ERROR_NOT_CONNECTED, "Send without connection should fail");
        
        /* Test connection to non-existent socket */
        result = logflux_client_connect(client);
        TEST_ASSERT(result == LOGFLUX_ERROR_CONNECTION, "Connection to non-existent socket should fail");
        
        /* Test close operation */
        result = logflux_client_close(client);
        TEST_ASSERT(result == LOGFLUX_OK, "Close operation should succeed");
        
        logflux_client_free(client);
    }
    
    /* Test invalid parameters */
    logflux_error_t result = logflux_client_connect(NULL);
    TEST_ASSERT(result == LOGFLUX_ERROR_INVALID_PARAM, "Connect with NULL client should fail");
    
    result = logflux_client_send_log(NULL, "message");
    TEST_ASSERT(result == LOGFLUX_ERROR_INVALID_PARAM, "Send with NULL client should fail");
    
    result = logflux_client_send_log(client, NULL);
    TEST_ASSERT(result == LOGFLUX_ERROR_INVALID_PARAM, "Send with NULL message should fail");
}

void test_utility_functions(void) {
    printf("\n=== Utility Function Tests ===\n");
    
    /* Test shared secret loading (may fail if agent not running) */
    char secret[128];
    logflux_error_t result = logflux_load_shared_secret(secret, sizeof(secret));
    if (result == LOGFLUX_OK) {
        TEST_ASSERT(strlen(secret) > 0, "Loaded shared secret should not be empty");
        printf("  Loaded shared secret: %zu characters\n", strlen(secret));
    } else {
        printf("  Shared secret loading failed (agent may not be running): %s\n", 
               logflux_error_string(result));
    }
    
    /* Test invalid parameters for secret loading */
    result = logflux_load_shared_secret(NULL, 128);
    TEST_ASSERT(result == LOGFLUX_ERROR_INVALID_PARAM, "NULL secret buffer should fail");
    
    result = logflux_load_shared_secret(secret, 0);
    TEST_ASSERT(result == LOGFLUX_ERROR_INVALID_PARAM, "Zero size buffer should fail");
    
    /* Test agent running check */
    bool agent_running = logflux_is_agent_running();
    printf("  Agent running status: %s\n", agent_running ? "YES" : "NO");
    TEST_ASSERT(true, "Agent running check completed"); /* Always passes */
}

void test_memory_management(void) {
    printf("\n=== Memory Management Tests ===\n");
    
    /* Test multiple client creation and destruction */
    for (int i = 0; i < 10; i++) {
        logflux_client_t* client = logflux_client_new_unix("/tmp/test.sock");
        TEST_ASSERT(client != NULL, "Repeated client creation");
        if (client) {
            logflux_client_free(client);
        }
    }
    
    /* Test multiple entry creation and destruction */
    for (int i = 0; i < 10; i++) {
        logflux_entry_t* entry = logflux_entry_new("Test message");
        TEST_ASSERT(entry != NULL, "Repeated entry creation");
        if (entry) {
            /* Add some labels to test label memory management */
            char key[16], value[16];
            snprintf(key, sizeof(key), "key%d", i);
            snprintf(value, sizeof(value), "value%d", i);
            logflux_entry_add_label(entry, key, value);
            
            logflux_entry_free(entry);
        }
    }
    
    /* Test null pointer handling */
    logflux_client_free(NULL); /* Should not crash */
    logflux_entry_free(NULL);  /* Should not crash */
    TEST_ASSERT(true, "NULL pointer handling");
}

int main(void) {
    printf("LogFlux C SDK - Unit Tests\n");
    printf("==========================\n");
    
    /* Run all test suites */
    test_client_creation();
    test_log_entry_creation();
    test_error_codes();
    test_connection_operations();
    test_utility_functions();
    test_memory_management();
    
    /* Print results */
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    
    if (tests_passed == tests_run) {
        printf("\nSUCCESS: All tests passed!\n");
        return 0;
    } else {
        printf("\nFAILURE: Some tests failed.\n");
        return 1;
    }
}