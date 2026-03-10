#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"

// Load database from binary file into memory array
Database *db_load_from_file(const char *filename)
{
  FILE *file = fopen(filename, "rb");
  if (file == NULL)
  {
    printf("No database file found. Starting fresh.\n");
    return db_create(10);
  }

  // Read header
  DatabaseHeader header;
  if (fread(&header, sizeof(DatabaseHeader), 1, file) != 1)
  {
    printf("Invalid database file!\n");
    fclose(file);
    return db_create(10);
  }

  // Validate magic number
  if (strcmp(header.magic, "PEDB") != 0)
  {
    printf("Corrupted database file!\n");
    fclose(file);
    return db_create(10);
  }

  // Create database with capacity = record count + 10% buffer
  int capacity = header.record_count + (header.record_count / 10) + 1;
  Database *db = db_create(capacity);

  // Read all records into array
  for (int i = 0; i < header.record_count; i++)
  {
    Person person;
    if (fread(&person, sizeof(Person), 1, file) != 1)
    {
      printf("Error reading record %d!\n", i + 1);
      break;
    }
    db->records[db->count++] = person;
  }

  fclose(file);
  printf("Database loaded from %s with %d records.\n", filename, db->count);
  return db;
}

// Append a single record to the database file
// Returns 1 on success, 0 on failure
int append_record_to_file(const char *filename, Person record)
{
  FILE *file = fopen(filename, "r+b");

  if (file == NULL)
  {
    printf("Creating new database file...\n");
    file = fopen(filename, "w+b");
    if (file == NULL)
    {
      perror("Error creating new file");
      return 0;
    }

    // Write initial header
    DatabaseHeader header = {"PEDB", 1, 0};
    fwrite(&header, sizeof(DatabaseHeader), 1, file);
    fseek(file, 0, SEEK_SET);
  }

  // Step 1: Read and validate the header
  DatabaseHeader header;
  if (fread(&header, sizeof(DatabaseHeader), 1, file) != 1)
  {
    printf("Error reading header!\n");
    fclose(file);
    return 0;
  }

  // Step 2: Check magic number
  if (strcmp(header.magic, "PEDB") != 0)
  {
    printf("Invalid database file!\n");
    fclose(file);
    return 0;
  }

  // Step 3: Seek to the end of the file
  if (fseek(file, 0, SEEK_END) != 0)
  {
    printf("Error seeking to end!\n");
    fclose(file);
    return 0;
  }

  // Step 4: Write the new record at the end
  if (fwrite(&record, sizeof(Person), 1, file) != 1)
  {
    printf("Error writing record!\n");
    fclose(file);
    return 0;
  }

  // Step 5: Update the header with the new record count
  header.record_count++;

  // Step 6: Seek back to beginning and update header
  if (fseek(file, 0, SEEK_SET) != 0)
  {
    perror("Error seeking to beginning!");
    fclose(file);
    return 0;
  }

  if (fwrite(&header, sizeof(DatabaseHeader), 1, file) != 1)
  {
    perror("Error updating header!");
    fclose(file);
    return 0;
  }

  fclose(file);
  printf("Record appended successfully. File now has %d records.\n", header.record_count);
  return 1;
}

// Retrieve a single record by its position (1-indexed)
Person get_record_by_position(const char *filename, int position)
{
  Person empty_record = {0};

  FILE *file = fopen(filename, "rb");
  if (file == NULL)
  {
    perror("Error opening file");
    return empty_record;
  }

  // Step 1: Read and validate the header
  DatabaseHeader header;
  if (fread(&header, sizeof(DatabaseHeader), 1, file) != 1)
  {
    printf("Error reading header!\n");
    fclose(file);
    return empty_record;
  }

  // Step 2: Validate position (1-indexed)
  if (position < 1 || position > header.record_count)
  {
    printf("Invalid position %d. File has %d records.\n", position, header.record_count);
    fclose(file);
    return empty_record;
  }

  // Step 3: Calculate the byte offset for this record
  long record_offset = sizeof(DatabaseHeader) + (position - 1) * sizeof(Person);

  // Step 4: Seek to that position
  if (fseek(file, record_offset, SEEK_SET) != 0)
  {
    perror("Error seeking to record!");
    fclose(file);
    return empty_record;
  }

  // Step 5: Read the record
  Person record;
  if (fread(&record, sizeof(Person), 1, file) != 1)
  {
    printf("Error reading record %d!\n", position);
    fclose(file);
    return empty_record;
  }

  fclose(file);
  printf("Retrieved record at position %d: ID=%d, Name=%s\n", position, record.id, record.name);

  return record;
}