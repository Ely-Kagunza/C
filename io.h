#ifndef IO_H
#define IO_H

#include "database.h"

Database* db_load_from_file(const char *filename);
int db_save_to_file(Database *db, const char *filename);
int append_record_to_file(const char *filename, Person record);
Person get_record_by_position(const char *filename, int position);

#endif