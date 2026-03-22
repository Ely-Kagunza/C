#ifndef NET_PROTICAL_H
#define NET_PROTICAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"

// ============== MESSAGE TYPES ==============

typedef enum {
    MSG_QUERY_REQUEST = 1,          // Client → Server
    MSG_QUERY_RESPONSE = 2,         // Server → Client
    MSG_RECORD = 3,                 // Single record
    MSG_ERROR = 4,                  // Error message
    MSG_HEARTBEAT = 5,              // Connection keepalive
    MSG_DISCONNECT = 6,             // Close connection
} MessageType;

// ============== MESSAGE STRUCTURES ==============

#define QUERY_MAX_LEN 1024
#define ERROR_MSG_LEN 256
#define PROTOCOL_VERSION 1

typedef struct {
    int version;                    // Protocol version
    MessageType type;               // Message type
    int payload_size;               // Size of payload
    int record_count;               // Number of records (for response)
} MessageHeader;

typedef struct {
    MessageHeader header;
    char query[QUERY_MAX_LEN];      // SQL query or command
} QueryRequest;

typedef struct {
    MessageHeader header;
    Person records[100];            // Up to 100 records per response
} QueryResponse;

typedef struct {
    MessageHeader header;
    char error_msg[ERROR_MSG_LEN];   // Error message
} ErrorMessage;

// ============= SERIALIZATION FUNCTIONS =============

// Serialize Person struct to bytes
int serialize_person(Person *p, char *buffer, int buffer_size);

// Deserialize bytes to Person struct
int deserialize_person(const char *buffer, int buffer_size, Person *p);

// Serialize QueryRequest to bytes
int serialize_query_request(QueryRequest *req, char *buffer, int buffer_size);

// Deserialize bytes to QueryRequest
int deserialize_query_request(const char *buffer, int buffer_size, QueryRequest *req);

// Serialize QueryResponse to bytes
int serialize_query_response(QueryResponse *resp, char *buffer, int buffer_size);

// Deserialize bytes to QueryResponse
int deserialize_query_response(const char *buffer, int buffer_size, QueryResponse *resp);

#endif