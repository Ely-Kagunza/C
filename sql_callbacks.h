#ifndef SQL_CALLBACKS_H
#define SQL_CALLBACKS_H

#include "person.h"

// =========== CALLBACK TYPES ===========

// Callback function type: runs when a record is inserted
// Parameters:
//    - record: the Person that was just inserted
//    - user_data: custom data passed when registering callback
typedef void (*OnInsertCallback)(Person *record, void *user_data);

// Callback function type: runs when a record is deleted
typedef void (*OnDeleteCallback)(int id, void *user_data);

// Callback function type: runs when a record is updated
typedef void (*OnUpdateCallback)(Person *old_record, Person *new_record, void *user_data);

// =========== CALLBACK REGISTRATION ===========

// Stores registered callbacks for one event type
typedef struct {
    OnInsertCallback callbacks[20];         // Array of function pointers
    void *user_data[20];                    // Custom data for each callback
    int count;                              // Number of callbacks registered
} InsertCallbackRegistry;

typedef struct {
    OnDeleteCallback callbacks[20];
    void *user_data[20];
    int count;
} DeleteCallbackRegistry;

typedef struct {
    OnUpdateCallback callbacks[20];
    void *user_data[20];
    int count;
} UpdateCallbackRegistry;

// Complete callback system for database
typedef struct {
    InsertCallbackRegistry on_insert;
    DeleteCallbackRegistry on_delete;
    UpdateCallbackRegistry on_update;
} CallbackSystem;

// =========== PUBLIC API ===========

// Create callback system
CallbackSystem *callback_create();

// Free callback system
void callback_free(CallbackSystem *cs);

// Register callbacks
void callbacks_register_insert(CallbackSystem *cs, OnInsertCallback callback, void *user_data);
void callbacks_register_delete(CallbackSystem *cs, OnDeleteCallback callback, void *user_data);
void callbacks_register_update(CallbackSystem *cs, OnUpdateCallback callback, void *user_data);

// Trigger callbacks (called by database when events happen)
void callbacks_trigger_insert(CallbackSystem *cs, Person *record);
void callbacks_trigger_delete(CallbackSystem *cs, int id);
void callbacks_trigger_update(CallbackSystem *cs, Person *old_record, Person *new_record);

// Display registered callbacks
void callbacks_display_stats(CallbackSystem *cs);

#endif // SQL_CALLBACKS_H