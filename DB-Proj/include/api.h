#ifndef API_H
#define API_H

#define _CRT_SECURE_NO_WARNINGS
#include "http_server.h"

// ================== API RESPONSE BUILDERS ==================

// Health check endpoint
void api_health(AsyncServer *server, ClientConnection *client);

// Query endpoints
void api_get_all(AsyncServer *server, ClientConnection *client);
void api_get_by_id(AsyncServer *server, ClientConnection *client, int id);
void api_age_range(AsyncServer *server, ClientConnection *client, int min_age, int max_age);
void api_salary_range(AsyncServer *server, ClientConnection *client, double min_salary, double max_salary);

// Mutation endpoints
void api_add_person(AsyncServer *server, ClientConnection *client, const char *json_body);
void api_update_person(AsyncServer *server, ClientConnection *client, int id, const char *json_body);
void api_delete_person(AsyncServer *server, ClientConnection *client, int id);

// ================== ROUTE DISPATCHER ==================

// Parse request and dispatch to appropriate handler
void api_dispatch(AsyncServer *server, ClientConnection *client);

// Helper: Extract integer from query parameter
int api_get_query_int(const char *query, const char *param_name);

// Helper: Extract double from query parameter
double api_get_query_double(const char *query, const char *param_name);

#endif