/**
 * LogFlux C SDK - Basic Example
 * 
 * Demonstrates how to use the LogFlux C SDK to send logs to the agent
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "logflux_client.h"

void demonstrate_unix_socket(void) {
    printf("=== Unix Socket Example ===\n");
    
    /* Create client for Unix socket connection */
    logflux_client_t* client = logflux_client_new_unix("/tmp/logflux-agent.sock");
    if (!client) {
        fprintf(stderr, "Failed to create Unix socket client\n");
        return;
    }
    
    /* Connect to agent */
    logflux_error_t result = logflux_client_connect(client);
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "Failed to connect via Unix socket: %s\n", logflux_error_string(result));
        logflux_client_free(client);
        return;
    }
    
    printf("Connected to LogFlux agent via Unix socket\n");
    
    /* Send a simple log message */
    result = logflux_client_send_log(client, "Hello from LogFlux C SDK!");
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "Failed to send log: %s\n", logflux_error_string(result));
    } else {
        printf("Sent simple log message\n");
    }
    
    /* Create and send a structured log entry */
    logflux_entry_t* entry = logflux_entry_new("Application started");
    if (entry) {
        logflux_entry_set_level(entry, LOGFLUX_LEVEL_INFO);
        logflux_entry_set_source(entry, "basic-example");
        logflux_entry_add_label(entry, "component", "demo");
        logflux_entry_add_label(entry, "version", "1.0.0");
        
        result = logflux_client_send_entry(client, entry);
        if (result != LOGFLUX_OK) {
            fprintf(stderr, "Failed to send structured entry: %s\n", logflux_error_string(result));
        } else {
            printf("Sent structured log entry\n");
        }
        
        logflux_entry_free(entry);
    }
    
    /* Clean up */
    logflux_client_close(client);
    logflux_client_free(client);
    printf("Unix socket connection closed\n\n");
}

void demonstrate_tcp_connection(void) {
    printf("=== TCP Connection Example ===\n");
    
    /* Create client for TCP connection */
    logflux_client_t* client = logflux_client_new_tcp("127.0.0.1", 8080);
    if (!client) {
        fprintf(stderr, "Failed to create TCP client\n");
        return;
    }
    
    /* Connect to agent */
    logflux_error_t result = logflux_client_connect(client);
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "Failed to connect via TCP: %s\n", logflux_error_string(result));
        logflux_client_free(client);
        return;
    }
    
    printf("Connected to LogFlux agent via TCP\n");
    
    /* Send a log message with authentication */
    result = logflux_client_send_log(client, "Hello from TCP connection!");
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "Failed to send TCP log: %s\n", logflux_error_string(result));
    } else {
        printf("Sent log via TCP\n");
    }
    
    /* Clean up */
    logflux_client_close(client);
    logflux_client_free(client);
    printf("TCP connection closed\n\n");
}

void demonstrate_batch_sending(void) {
    printf("=== Batch Sending Example ===\n");
    
    logflux_client_t* client = logflux_client_new_unix("/tmp/logflux-agent.sock");
    if (!client) {
        fprintf(stderr, "Failed to create client for batch example\n");
        return;
    }
    
    logflux_error_t result = logflux_client_connect(client);
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "Failed to connect for batch example: %s\n", logflux_error_string(result));
        logflux_client_free(client);
        return;
    }
    
    /* Create multiple log entries */
    logflux_entry_t entries[3];
    logflux_entry_t* entry_ptrs[3];
    
    for (int i = 0; i < 3; i++) {
        char message[256];
        snprintf(message, sizeof(message), "Batch log entry #%d", i + 1);
        
        logflux_entry_t* entry = logflux_entry_new(message);
        if (entry) {
            logflux_entry_set_level(entry, LOGFLUX_LEVEL_INFO);
            logflux_entry_set_source(entry, "batch-example");
            
            char label_value[32];
            snprintf(label_value, sizeof(label_value), "%d", i + 1);
            logflux_entry_add_label(entry, "sequence", label_value);
            
            entries[i] = *entry;
            entry_ptrs[i] = &entries[i];
            free(entry); /* Free the wrapper, keep the content */
        }
    }
    
    /* Send as batch */
    result = logflux_client_send_batch(client, entry_ptrs, 3);
    if (result != LOGFLUX_OK) {
        fprintf(stderr, "Failed to send batch: %s\n", logflux_error_string(result));
    } else {
        printf("Sent batch of 3 log entries\n");
    }
    
    /* Clean up entries */
    for (int i = 0; i < 3; i++) {
        logflux_entry_free(&entries[i]);
    }
    
    logflux_client_close(client);
    logflux_client_free(client);
    printf("Batch example completed\n\n");
}

void demonstrate_error_handling(void) {
    printf("=== Error Handling Example ===\n");
    
    /* Try to connect to non-existent socket */
    logflux_client_t* client = logflux_client_new_unix("/nonexistent/socket");
    if (client) {
        logflux_error_t result = logflux_client_connect(client);
        if (result != LOGFLUX_OK) {
            printf("Expected error connecting to non-existent socket: %s\n", 
                   logflux_error_string(result));
        }
        
        /* Try to send without being connected */
        result = logflux_client_send_log(client, "This should fail");
        if (result == LOGFLUX_ERROR_NOT_CONNECTED) {
            printf("Expected error sending while not connected: %s\n",
                   logflux_error_string(result));
        }
        
        logflux_client_free(client);
    }
    
    /* Test invalid parameters */
    logflux_error_t result = logflux_client_send_log(NULL, "Invalid client");
    if (result == LOGFLUX_ERROR_INVALID_PARAM) {
        printf("Expected error with NULL client: %s\n", logflux_error_string(result));
    }
    
    printf("Error handling examples completed\n\n");
}

int main(void) {
    printf("LogFlux C SDK - Basic Example\n");
    printf("============================\n\n");
    
    /* Check if agent is running */
    if (logflux_is_agent_running()) {
        printf("LogFlux agent is running\n\n");
    } else {
        printf("Warning: LogFlux agent does not appear to be running\n");
        printf("Some examples may fail to connect\n\n");
    }
    
    /* Run examples */
    demonstrate_unix_socket();
    demonstrate_tcp_connection();
    demonstrate_batch_sending();
    demonstrate_error_handling();
    
    printf("All examples completed!\n");
    return 0;
}