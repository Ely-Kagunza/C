#ifndef PERSON_H
#define PERSON_H

// Person record structure
typedef struct
{
    int id;
    char name[100];
    int age;
    double salary;
} Person;

// Database structure for in-memory storage
typedef struct
{
    Person *records; // Pointer to array of Person structs
    int count;       // Current number of records
    int capacity;    // Space allocated for records
} Database;

// Header for binary file format
typedef struct
{
    char magic[5];    // "PEDB" - magic number for validation
    int version;      // Version number
    int record_count; // Number of records in the file
} DatabaseHeader;

#endif