#ifndef CLI_H
#define CLI_H

#include "database.h"

// Command types
typedef enum {
    CMD_INVALID,
    CMD_QUERY,    // Query single record by ID or name
    CMD_FILTER,   // Filter records by criteria
    CMD_EXPORT,   // Export to file
    CMD_HELP,
    CMD_INTERACTIVE
} CommandType;

// Query criteria structure
typedef struct {
    int by_id;      // Search by ID
    int id_value;   // The ID to search for
    int by_name;    // Search by name
    char name_vale[100]; // The name to search for
} QueryCriteria;

// Filter criteria structure
typedef struct {
    int has_age_min;
    int age_min;
    int has_age_max;
    int age_max;
    int has_salary_min;
    double salary_min;
    int has_name_filter;
    char name_filter[100];
} FilterCriteria;

// Export options
typedef struct {
    char format[20];   // "csv" or "text"
    char output_file[256];
} ExportOptions;

// CLI functions
CommandType parse_command(int argc, char *argv[]);
void run_cli_mode(int argc, char *argv[], Database *db);
void show_help(void);

// Command handlers
int handle_query(int argc, char *argv[], Database *db);
int handle_filter(int argc, char *argv[], Database *db);
int handle_export(int argc, char *argv[], Database *db);

#endif