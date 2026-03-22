#include "net_protocol.h"

// ================ SERIALIZATION: PERSON STRUCT ================

int serialize_person(Person *p, char *buffer, int buffer_size)
{
    if (p == NULL || buffer == NULL)
        return -1;

    // Pack: id (4) + name (100) + age(4) + salary(8)
    int offset = 0;

    // ID (int)
    if (offset + 4 > buffer_size) return -1;
    memcpy(buffer + offset, &p->id, sizeof(int));
    offset += sizeof(int);

    // Name (char[100])
    if (offset + 100 > buffer_size) return -1;
    strncpy(buffer + offset, p->name, 99);
    buffer[offset + 99] = '\0';
    offset += 100;

    // Age (int)
    if (offset + 4 > buffer_size) return -1;
    memcpy(buffer + offset, &p->age, sizeof(int));
    offset += sizeof(int);

    // Salary (double)
    if (offset + 8 > buffer_size) return -1;
    memcpy(buffer + offset, &p->salary, sizeof(double));
    offset += sizeof(double);

    return offset;
}

int deserialize_person(const char *buffer, int buffer_size, Person *p)
{
    if (buffer == NULL || p == NULL)
        return -1;

    int offset = 0;

    // ID
    if (offset + 4 > buffer_size) return -1;
    memcpy(&p->id, buffer + offset, sizeof(int));
    offset += sizeof(int);

    // Name
    if (offset + 100 > buffer_size) return -1;
    strncpy(p->name, buffer + offset, 99);
    offset += 100;

    // Age
    if (offset + 4 > buffer_size) return -1;
    memcpy(&p->age, buffer + offset, sizeof(int));
    offset += sizeof(int);

    // Salary
    if (offset + 8 > buffer_size) return -1;
    memcpy(&p->salary, buffer + offset, sizeof(double));
    offset += sizeof(double);

    return offset;
}

// ================ SERIALIZATION: QUERY REQUEST ================

int serialize_query_request(QueryRequest *req, char *buffer, int buffer_size)
{
    if (req == NULL || buffer == NULL)
        return -1;

    int offset = 0;

    // Header
    if (offset + sizeof(MessageHeader) > buffer_size) return -1;
    memcpy(buffer + offset, &req->header, sizeof(MessageHeader));
    offset += sizeof(MessageHeader);

    // Query
    if (offset + QUERY_MAX_LEN > buffer_size) return -1;
    strncpy(buffer + offset, req->query, QUERY_MAX_LEN - 1);
    offset += QUERY_MAX_LEN;

    return offset;
}

int deserialize_query_request(const char *buffer, int buffer_size, QueryRequest *req)
{
    if (buffer == NULL || req == NULL)
        return -1;

    int offset = 0;

    // Header
    if (offset + sizeof(MessageHeader) > buffer_size) return -1;
    memcpy(&req->header, buffer + offset, sizeof(MessageHeader));
    offset += sizeof(MessageHeader);

    // Query
    if (offset + QUERY_MAX_LEN > buffer_size) return -1;
    strncpy(req->query, buffer + offset, QUERY_MAX_LEN - 1);
    offset += QUERY_MAX_LEN;

    return offset;
}

// ================ SERIALIZATION: QUERY RESPONSE ================

int serialize_query_response(QueryResponse *resp, char *buffer, int buffer_size)
{
    if (resp == NULL || buffer == NULL)
        return -1;

    int offset = 0;

    // Header
    if (offset + sizeof(MessageHeader) > buffer_size) return -1;
    memcpy(buffer + offset, &resp->header, sizeof(MessageHeader));
    offset += sizeof(MessageHeader);

    // Records
    for (int i = 0; i < resp->header.record_count; i++)
    {
        int bytes = serialize_person(&resp->records[i], buffer + offset, buffer_size - offset);

        if (bytes < 0) return -1;
        offset += bytes;
    }

    return offset;
}

int deserialize_query_response(const char *buffer, int buffer_size, QueryResponse *resp)
{
    if (buffer == NULL || resp == NULL)
        return -1;

    int offset = 0;

    // Header
    if (offset + sizeof(MessageHeader) > buffer_size) return -1;
    memcpy(&resp->header, buffer + offset, sizeof(MessageHeader));
    offset += sizeof(MessageHeader);

    // Records
    for (int i = 0; i < resp->header.record_count; i++)
    {
        int bytes = deserialize_person(buffer + offset, buffer_size - offset, &resp->records[i]);

        if (bytes < 0) return -1;
        offset += bytes;
    }

    return offset;
}