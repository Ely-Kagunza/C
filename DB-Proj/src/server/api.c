#define _CRT_SECURE_NO_WARNINGS
#include "../../include/api.h"
#include "../../include/json_utils.h"
#include "../../include/hash_index.h"
#include <stdio.h>
#include <string.h>

// ================== QUERY PARAMETER HELPERS ==================

int api_get_query_int(const char *query, const char *param_name) {
    if (!query || !param_name) return 0;

    char search_str[256];
    snprintf(search_str, sizeof(search_str) - 1, "%s=", param_name);

    const char *pos = strstr(query, search_str);
    if (!pos) return 0;

    int value = 0;
    sscanf(pos + strlen(search_str), "%d", &value);
    return value;
}

double api_get_query_double(const char *query, const char *param_name) {
    if (!query || !param_name) return 0.0;

    char search_str[256];
    snprintf(search_str, sizeof(search_str) - 1, "%s=", param_name);

    const char *pos = strstr(query, search_str);
    if (!pos) return 0.0;

    double value = 0.0;
    sscanf(pos + strlen(search_str), "%lf", &value);
    return value;
}

// ================== HEALTH CHECK ==================

void api_health(AsyncServer *server, ClientConnection *client) {
    if (!server || !client) return;

    EnterCriticalSection(&server->db_lock);

    char response[256];
    snprintf(response, sizeof(response) - 1, "{\"status\":\"ok\",\"records\":%d,\"timestamp\":%ld}", server->db->count, (long)time(NULL));

    LeaveCriticalSection(&server->db_lock);

    http_build_response(client, response, 200);
}

// ================== GET ALL PEOPLE ==================

void api_get_all(AsyncServer *server, ClientConnection *client) {
    if (!server || !client) return;

    EnterCriticalSection(&server->db_lock);

    char *response;
    if (server->db->count == 0) {
        response = "[]";
    } else {
        response = person_array_to_json(server->db->records, server->db->count);
    }

    char response_copy[MAX_RESPONSE_SIZE];
    strncpy(response_copy, response, sizeof(response_copy) - 1);

    LeaveCriticalSection(&server->db_lock);

    http_build_response(client, response_copy, 200);
}

// ================== GET PERSON BY ID (HASH TABLE - O(1) LOOKUP) ==================

void api_get_by_id(AsyncServer *server, ClientConnection *client, int id) {
    if (!server || !client) return;

    EnterCriticalSection(&server->db_lock);

    Person *person = hash_index_find_by_id(server->id_index, id);
    char *response;
    int status = 404;

    if (person) {
        response = person_to_json(person);
        status = 200;
    } else {
        response = json_error("Person not found");
    }

    char response_copy[512];
    strncpy(response_copy, response, sizeof(response_copy) - 1);

    LeaveCriticalSection(&server->db_lock);

    http_build_response(client, response_copy, status);
}

// ================== AGE RANGE QUERY (B-TREE - O(log n + k) LOOKUP) ==================

void api_age_range(AsyncServer *server, ClientConnection *client, int min_age, int max_age) {
    if (!server || !client) return;

    EnterCriticalSection(&server->db_lock);

    RangeResult result = btree_range_query(server->age_index, min_age, max_age);

    char *response;
    if (result.count == 0) {
        response = "[]";
    } else {
        response = person_array_to_json(*result.results, result.count);
    }

    char response_copy[MAX_RESPONSE_SIZE];
    strncpy(response_copy, response, sizeof(response_copy) - 1);

    range_result_free(&result);
    LeaveCriticalSection(&server->db_lock);

    http_build_response(client, response_copy, 200);
}

// ================== SALARY RANGE QUERY (B-TREE - O(log n + k) LOOKUP) ==================

void api_salary_range(AsyncServer *server, ClientConnection *client, double min_salary, double max_salary) {
    if (!server || !client) return;

    EnterCriticalSection(&server->db_lock);

    RangeResult result = btree_range_query(server->salary_index, min_salary, max_salary);

    char *response;
    if (result.count == 0) {
        response = "[]";
    } else {
        response = person_array_to_json(*result.results, result.count);
    }

    char response_copy[MAX_RESPONSE_SIZE];
    strncpy(response_copy, response, sizeof(response_copy) - 1);

    range_result_free(&result);
    LeaveCriticalSection(&server->db_lock);

    http_build_response(client, response_copy, 200);
}

// ================== ADD PERSON (DATABASE - O(1) INSERT) ==================

void api_add_person(AsyncServer *server, ClientConnection *client, const char *json_body) {
    if (!server || !client || !json_body) {
        http_build_response(client, json_error("Invalid request"), 400);return;
    }

    Person p = {0};
    if (!person_from_json(json_body, &p)) {
        http_build_response(client, json_error("Invalid JSON format"), 400);
        return;
    }

    EnterCriticalSection(&server->db_lock);

    // Check if ID already exists
    if (hash_index_find_by_id(server->id_index, p.id)) {
        LeaveCriticalSection(&server->db_lock);
        http_build_response(client, json_error("Person with this ID already exists"), 400);
        return;
    }

    // Add to database
    if (!database_add_person(server->db, p)) {
        LeaveCriticalSection(&server->db_lock);
        http_build_response(client, json_error("Failed to add person"), 500);
        server->total_errors++;
        return;
    }

    // Add to B-Tree indexes
    Person *added_person = &server->db->records[server->db->count - 1];
    hash_index_insert(server->id_index, added_person);
    btree_insert(server->age_index, p.age, added_person);
    btree_insert(server->salary_index, (int)p.salary, added_person);

    LeaveCriticalSection(&server->db_lock);

    http_build_response(client, json_success(person_to_json(added_person)), 201);
}

// ================== UPDATE PERSON (DATABASE - O(1) UPDAT VIA HASH TABLE) ==================

void api_update_person(AsyncServer *server, ClientConnection *client, int id, const char *json_body) {
    if (!server || !client || !json_body) {
        http_build_response(client, json_error("Invalid request"), 400);return;
    }

    Person updated = {0};
    if (!person_from_json(json_body, &updated)) {
        http_build_response(client, json_error("Invalid JSON format"), 400);
        return;
    }

    updated.id = id;    // Ensure ID matches

    EnterCriticalSection(&server->db_lock);

    Person *existing = hash_index_find_by_id(server->id_index, id);
    if (!existing) {
        LeaveCriticalSection(&server->db_lock);
        http_build_response(client, json_error("Person not found"), 404);
        return;
    }

    if (!database_update_person(server->db, id, updated)) {
        LeaveCriticalSection(&server->db_lock);
        http_build_response(client, json_error("Failed to update person"), 500);
        server->total_errors++;
        return;
    }

    // Note: B-Tree indexes are read-only for now
    // In production, you'd rebuild indexes after updates

    LeaveCriticalSection(&server->db_lock);

    http_build_response(client, json_success(person_to_json(&updated)), 200);
}

// ================== DELETE PERSON (DATABASE - O(1) DELETE VIA HASH TABLE) ==================

void api_delete_person(AsyncServer *server, ClientConnection *client, int id) {
    if (!server || !client) return;

    EnterCriticalSection(&server->db_lock);

    if (hash_index_find_by_id(server->id_index, id) == NULL) {
        LeaveCriticalSection(&server->db_lock);
        http_build_response(client, json_error("Person not found"), 404);
        return;
    }

    // Delete from database
    if (!database_delete_person(server->db, id)) {
        LeaveCriticalSection(&server->db_lock);
        http_build_response(client, json_error("Failed to delete person"), 500);
        server->total_errors++;
        return;
    }

    // Remove from hash table (O(1))
    hash_index_remove(server->id_index, id);

    // Note B-Tree indexes are read-only for now
    // In production, you'd rebuild indexes after deletions

    LeaveCriticalSection(&server->db_lock);

    http_build_response(client, json_status("Person deleted successfully"), 200);
}

// ================== ROUTE DISPATCHER ==================

void api_dispatch(AsyncServer *server, ClientConnection *client) {
    if (!server || !client) return;

    http_server_log(server, "DEBUG", "[%s] Dispatching %s %s", client->request_id, client->method, client->path);

    // ------------ GET /api/health ------------
    if (strcmp(client->method, "GET") == 0 && strcmp(client->path, "/api/health") == 0) {
        api_health(server, client);
        return;
    }

    // ------------ GET /api/people ------------
    if (strcmp(client->method, "GET") == 0 && strcmp(client->path, "/api/people") == 0) {
        // Check for range queries
        int min_age = api_get_query_int(client->query, "age_min");
        int max_age = api_get_query_int(client->query, "age_max");

        if (min_age > 0 && max_age > 0) {
            api_age_range(server, client, min_age, max_age);
            return;
        }

        double min_salary = api_get_query_double(client->query, "salary_min");
        double max_salary = api_get_query_double(client->query, "salary_max");

        if (min_salary > 0 && max_salary > 0) {
            api_salary_range(server, client, min_salary, max_salary);
            return;
        }

        // No range query, get all
        api_get_all(server, client);
        return;
    }

    // ------------ GET /api/people/:id ------------
    if (strcmp(client->method, "GET") == 0 && strstr(client->path, "/api/people/") == client->path) {
        int id = 0;
        if (sscanf(client->path, "/api/people/%d", &id) == 1) {
            api_get_by_id(server, client, id);
            return;
        }
    }

    // ------------ POST /api/people ------------
    if (strcmp(client->method, "POST") == 0 && strcmp(client->path, "/api/people") == 0) {
        api_add_person(server, client, client->body);
        return;
    }

    // ------------ PUT /api/people/:id ------------
    if (strcmp(client->method, "PUT") == 0 && strstr(client->path, "/api/people/") == client->path) {
        int id = 0;
        if (sscanf(client->path, "/api/people/%d", &id) == 1) {
            api_update_person(server, client, id, client->body);
            return;
        }
    }

    // ------------ DELETE /api/people/:id ------------
    if (strcmp(client->method, "DELETE") == 0 && strstr(client->path, "/api/people/") == client->path) {
        int id = 0;
        if (sscanf(client->path, "/api/people/%d", &id) == 1) {
            api_delete_person(server, client, id);
            return;
        }
    }

    // ------------ 404 NOT FOUND ------------
    http_build_response(client, json_error("Endpoint not found"), 404);
    server->total_errors++;
}