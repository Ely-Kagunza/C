#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"
#include "database.h"

// ============================================================================
// COMMAND PARSER - Determine what command user requested
// ============================================================================
//
// CONCEPT: Look at argv[1] to determine which command to run
// This is the "dispatcher" that routes to the right handler
//
// PYTHON COMPARISON:
// Similar to if/elif chains based on sys.argv[1]
// Or how web frameworks route URLs to handlers
// ============================================================================
CommandType parse_command(int argc, char *argv[])
{
    if (argc == 1)
        return CMD_INTERACTIVE;  // No arguments, go interactive mode

    if (argc < 2)
        return CMD_INVALID;      // Invalid command

    char *cmd = argv[1];

    if (strcmp(cmd, "query") == 0)
        return CMD_QUERY;
    else if (strcmp(cmd, "filter") == 0)
        return CMD_FILTER;
    else if (strcmp(cmd, "export") == 0)
        return CMD_EXPORT;
    else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0)
        return CMD_HELP;
    else
        return CMD_INVALID;
}

// ============================================================================
// HELP TEXT - Show usage information
// ============================================================================
void show_help(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║           Database CLI - Usage Information             ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    printf("USAGE:\n");
    printf("  ./database [COMMAND] [OPTIONS]\n\n");

    printf("COMMANDS:\n");
    printf("  query                Query a single record\n");
    printf("    --id <ID>          Find record by ID\n");
    printf("    --name <NAME>      Find record by name\n");
    printf("    Example: ./database query --id 1\n");
    printf("    Example: ./database query --name Joe\n\n");

    printf("  filter               Filter records by criteria\n");
    printf("    --age-min <NUM>    Minimum age\n");
    printf("    --age-max <NUM>    Maximum age\n");
    printf("    --salary-min <NUM> Minimum salary\n");
    printf("    --name <NAME>      Filter by name substring\n");
    printf("    Example: ./database filter --age-min 25 --age-max 35\n\n");

    printf("  export               Export records to file\n");
    printf("    --format <FORMAT>  Output format: csv or text\n");
    printf("    --output <FILE>    Output filename\n");
    printf("    Example: ./database export --format csv --output results.txt\n\n");

    printf("  help                 Show this help message\n\n");

    printf("INTERACTIVE MODE:\n");
    printf("  ./database           Run interactive menu (no arguments)\n\n");
}

// ============================================================================
// OPTION PARSER - Find option value in argv
// ============================================================================
//
// CONCEPT: Search argv for a flag like "--id" and return the next argument
// This is called by handlers to extract option values
//
// ALGORITHM:
// 1. Loop through argv looking for the flag
// 2. If found, check there's a next argument (the value)
// 3. Return that value
// 4. If not found or no value, return NULL
//
// EXAMPLE:
// argv = ["./db", "query", "--id", "1", "--name", "Joe"]
// find_option(argv, argc, "--id") returns "1"
// find_option(argv, argc, "--name") returns "Joe"
// ============================================================================
static char *find_option(int argc, char *argv[], const char *option)
{
    for (int i = 1; i < argc - 1; i++)
    {
        if (strcmp(argv[i], option) == 0)
        {
            // Found the option, return the next argument
            return argv[i + 1];
        }
    }
    return NULL;  // Option not found or no value
}

// ============================================================================
// QUERY HANDLER - Find a single record by ID or name
// ============================================================================
//
// COMMAND SYNTAX:
//   ./database query --id 1
//   ./database query --name Joe
// ============================================================================
int handle_query(int argc, char *argv[], Database *db)
{
    char *id_str = find_option(argc, argv, "--id");
    char *name_str = find_option(argc, argv, "--name");

    // Check that user provided at least one search criterion
    if (id_str == NULL && name_str == NULL)
    {
        printf("Error: Query requires --id or --name\n");
        printf("Usage: ./database query --id <ID>\n");
        printf("       ./database query --name <NAME>\n");
        return 1;
    }

    // Search by ID
    if (id_str != NULL)
    {
        int id = atoi(id_str);  // Convert string to int
        Person *found = db_get_by_id(db, id);

        if (found != NULL)
        {
            printf("\n✓ Found record:\n");
            printf("  ID:     %d\n", found->id);
            printf("  Name:   %s\n", found->name);
            printf("  Age:    %d\n", found->age);
            printf("  Salary: %.2f\n\n", found->salary);
            return 0;
        }
        else
        {
            printf("✗ No record found with ID %d\n\n", id);
            return 1;
        }
    }

    // Search by name
    if (name_str != NULL)
    {
        int idx = db_search_by_name(db, name_str);

        if (idx >= 0)
        {
            Person *found = &db->records[idx];
            printf("\n✓ Found record:\n");
            printf("  ID:     %d\n", found->id);
            printf("  Name:   %s\n", found->name);
            printf("  Age:    %d\n", found->age);
            printf("  Salary: %.2f\n\n", found->salary);
            return 0;
        }
        else
        {
            printf("✗ No record found with name '%s'\n\n", name_str);
            return 1;
        }
    }

    return 0;
}

// ============================================================================
// FILTER HANDLER - Find records matching criteria
// ============================================================================
//
// COMMAND SYNTAX:
//   ./database filter --age-min 25 --age-max 35
//   ./database filter --salary-min 50000
//   ./database filter --name substring
// ============================================================================
int handle_filter(int argc, char *argv[], Database *db)
{
    FilterCriteria criteria = {0};

    // Parse age min
    char *age_min_str = find_option(argc, argv, "--age-min");
    if (age_min_str != NULL)
    {
        criteria.has_age_min = 1;
        criteria.age_min = atoi(age_min_str);
    }

    // Parse age max
    char *age_max_str = find_option(argc, argv, "--age-max");
    if (age_max_str != NULL)
    {
        criteria.has_age_max = 1;
        criteria.age_max = atoi(age_max_str);
    }

    // Parse salary min
    char *salary_min_str = find_option(argc, argv, "--salary-min");
    if (salary_min_str != NULL)
    {
        criteria.has_salary_min = 1;
        criteria.salary_min = atof(salary_min_str);  //atof for floating point
    }

    // Parse name filter
    char *name_str = find_option(argc, argv, "--name");
    if (name_str != NULL)
    {
        criteria.has_name_filter = 1;
        strncpy(criteria.name_filter, name_str, sizeof(criteria.name_filter) - 1);
    }

    // Check that user provided at least one criterion
    if (!criteria.has_age_min && !criteria.has_age_max && 
      !criteria.has_salary_min && !criteria.has_name_filter)
    {
        printf("Error: Filter requires at least one criterion\n");
        printf("Usage: ./database filter --age-min 25 --age-max 35\n");
        return 1;
    }

    // Apply filters
    int match_count = 0;
    printf("\n");

    for (int i = 0; i < db->count; i++)
    {
        Person *p = &db->records[i];
        int matches = 1;

        // Check age-min
        if (criteria.has_age_min && p->age < criteria.age_min)
            matches = 0;

        // Check age-max
        if (criteria.has_age_max && p->age > criteria.age_max)
            matches = 0;

        // Check salary-min
        if (criteria.has_salary_min && p->salary < criteria.salary_min)
            matches = 0;

        // Check name (substring match)
        if (criteria.has_name_filter && strstr(p->name, criteria.name_filter) == NULL)
            matches = 0;

        if (matches)
        {
            printf("ID: %d | Name: %-15s | Age: %3d | Salary: %.2f\n",
                    p->id, p->name, p->age, p->salary);
            match_count++;
        }
    }
    
    printf("\nFound %d matching record(s).\n\n", match_count);
    return 0;
}

// ============================================================================
// EXPORT HANDLER - Save records to file
// ============================================================================
//
// COMMAND SYNTAX:
//   ./database export --format csv --output results.txt
//   ./database export --format text --output results.txt
// ============================================================================
int handle_export(int argc, char *argv[], Database *db)
{
    ExportOptions opts = {0};

    // Parse format
    char *format_str = find_option(argc, argv, "--format");
    if (format_str == NULL)
    {
        printf("Error: Invalid format '%s'\n", format_str);
        return 1;
    }
    strncpy(opts.format, format_str, sizeof(opts.format) - 1);

    // Parse output file
    char *output_str = find_option(argc, argv, "--output");
    if (output_str == NULL)
    {
        printf("Error: Invalid output file '%s'\n", output_str);
        return 1;
    }
    strncpy(opts.output_file, output_str, sizeof(opts.output_file) - 1);

    // Validate format
    if (strcmp(opts.format, "csv") != 0 && strcmp(opts.format, "text") != 0)
    {
        printf("Error: Invalid format '%s'\n", opts.format);
        return 1;
    }

    // Open output file
    FILE *file = fopen(opts.output_file, "w");
    if (file == NULL)
    {
        printf("Error: Failed to open output file '%s'\n", opts.output_file);
        return 1;
    }

    // Export as CSV
    if (strcmp(opts.format, "csv") == 0)
    {
        fprintf(file, "ID,Name,Age,Salary\n");
        for (int i = 0; i < db->count; i++)
        {
            Person *p = &db->records[i];
            fprintf(file, "%d,%s,%d,%.2f\n", p->id, p->name, p->age, p->salary);
        }
    }

    // Export as text table
    else if (strcmp(opts.format, "text") == 0)
    {
        fprintf(file, "┌────┬──────────────┬─────┬──────────┐\n");
        fprintf(file, "│ ID │ Name         │ Age │ Salary   │\n");
        fprintf(file, "├────┼──────────────┼─────┼──────────┤\n");

        for (int i = 0; i < db->count; i++)
        {
            Person *p = &db->records[i];
            fprintf(file, "│ %-2d │ %-12s │ %3d │ %8.2f │\n",
                    p->id, p->name, p->age, p->salary);
        }

        fprintf(file, "└────┴──────────────┴─────┴──────────┘\n");
    }

    // Close file
    fclose(file);
    printf("✓ Exported %d record(s) to '%s' (%s format)\n\n",
           db->count, opts.output_file, opts.format);
    return 0;
}

// ============================================================================
// MAIN CLI DISPATCHER - Route commands to handlers
// ============================================================================
void run_cli_mode(int argc, char *argv[], Database *db)
{
    CommandType cmd = parse_command(argc, argv);

    switch (cmd)
    {
    case CMD_QUERY:
        handle_query(argc, argv, db);
        break;
    case CMD_FILTER:
        handle_filter(argc, argv, db);
        break;
    case CMD_EXPORT:
        handle_export(argc, argv, db);
        break;
    case CMD_HELP:
        show_help();
        break;
    case CMD_INTERACTIVE:
        // This won't happen here, but for completeness
        printf("Error: Shoul use interactive mode\n");
        break;
        case CMD_INVALID:
        printf("Error: Unknown command '%s'\n", argv[1]);
        printf("Use './database help' for usage information\n");
        break;
    }
}