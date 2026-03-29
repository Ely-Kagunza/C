#define _CRT_SECURE_NO_WARNINGS
#include "../../include/json_utils.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static char json_buffer[JSON_BUFFER_SIZE];

char *person_to_json(const Person *p) {
    if (!p) return "{}";

    memset(json_buffer, 0, sizeof(json_buffer));
    snprintf(json_buffer, sizeof(json_buffer) - 1,
        "{\"id\":%d,\"name\":\"%s\",\"age\":%d,\"salary\":%.2f}",
        p->id, p->name, p->age, p->salary);

    return json_buffer;
}

int person_from_json(const char *json, Person *p) {
    if (!json || !p) return 0;

    memset(p, 0, sizeof(Person));

    // Parse: {"id":1, "name":"John Doe", "age":30, "salary":50000.00}
    int result = sscanf(json,
        "{\"id\":%d, \"name\":\"%63[^\"]\", \"age\":%d, \"salary\":%lf}",
        &p->id, p->name, &p->age, &p->salary);

    return (result == 4) ? 1 : 0;
}

char *person_array_to_json(const Person *people, int count) {
    if (!people || count < 0) return "[]";

    memset(json_buffer, 0, sizeof(json_buffer));
    strcpy(json_buffer, "[");

    for (int i = 0; i < count; i++) {
        if (i > 0) strcat(json_buffer, ",");

        char person_json[512];
        snprintf(person_json, sizeof(person_json) - 1,
            "{\"id\":%d,\"name\":\"%s\",\"age\":%d,\"salary\":%.2f}",
            people[i].id, people[i].name, people[i].age, people[i].salary);
        strcat(json_buffer, person_json);
  
    }

    strcat(json_buffer, "]");
    return json_buffer;
}

char *json_error(const char *message) {
    if (!message) message = "Unknown error";

    memset(json_buffer, 0, sizeof(json_buffer));
    snprintf(json_buffer, sizeof(json_buffer) - 1,
        "{\"error\":\"%s\"}", message);

    return json_buffer;
}

char *json_success(const char *data) {
    if (!data) data = "";

    memset(json_buffer, 0, sizeof(json_buffer));

    // If data is already JSON, don't quote it
    if (data[0] == '{' || data[0] == '[') {
        snprintf(json_buffer, sizeof(json_buffer) - 1,
            "{\"success\":true,\"data\":%s}", data);
    } else {
        snprintf(json_buffer, sizeof(json_buffer) - 1,
            "{\"success\":true,\"data\":\"%s\"}", data);
    }

    return json_buffer;
}

char *json_status(const char *status) {
    if (!status) status = "ok";

    memset(json_buffer, 0, sizeof(json_buffer));
    snprintf(json_buffer, sizeof(json_buffer) - 1,
        "{\"status\":\"%s\"}", status);

    return json_buffer;
}

char *json_key_value(const char *key, const char *value) {
    if (!key) key = "key";
    if (!value) value = "";

    memset(json_buffer, 0, sizeof(json_buffer));
    snprintf(json_buffer, sizeof(json_buffer) - 1,
        "{\"%s\":\"%s\"}", key, value);

    return json_buffer;
}

char *json_escape_string(const char *str) {
    if (!str) return "";

    static char escaped[JSON_BUFFER_SIZE];
    memset(escaped, 0, sizeof(escaped));

    int j = 0;
    for (int i = 0; str[i] && j < (int)sizeof(escaped) - 2; i++) {
        switch (str[i]) {
            case '"':
                escaped[j++] = '\\';
                escaped[j++] = '"';
                break;
            case '\\':
                escaped[j++] = '\\';
                escaped[j++] = '\\';
                break;
            case '/':
                escaped[j++] = '\\';
                escaped[j++] = '/';
                break;
            case '\b':
                escaped[j++] = '\\';
                escaped[j++] = 'b';
                break;
            case '\f':
                escaped[j++] = '\\';
                escaped[j++] = 'f';
                break;
            case '\n':
                escaped[j++] = '\\';
                escaped[j++] = 'n';
                break;
            case '\r':
                escaped[j++] = '\\';
                escaped[j++] = 'r';
                break;
            case '\t':
                escaped[j++] = '\\';
                escaped[j++] = 't';
                break;
            default:
                if (isprint(str[i])) {
                    escaped[j++] = str[i];
                }
                break;
        }
    }

    escaped[j] = '\0';
    return escaped;
}