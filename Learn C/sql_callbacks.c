#include "sql_callbacks.h"
#include <stdlib.h>
#include <stdio.h>

// Create callback system
CallbackSystem *callback_create(void)
{
    CallbackSystem *cs = malloc(sizeof(CallbackSystem));
    if (cs == NULL)
        return NULL;

    cs->on_insert.count = 0;
    cs->on_delete.count = 0;
    cs->on_update.count = 0;

    return cs;
}

// Free callback system
void callback_free(CallbackSystem *cs)
{ 
    if (cs != NULL)
        free(cs);
}

// Register an INSERT callback
void callbacks_register_insert(CallbackSystem *cs, OnInsertCallback callback, void *user_data)
{
    if (cs == NULL || callback == NULL)
        return;

    if (cs->on_insert.count >= 20)
    {
        printf("ERROR: Max INSERT callbacks reached (20)\n");
        return;
    }

    cs->on_insert.callbacks[cs->on_insert.count] = callback;
    cs->on_insert.user_data[cs->on_insert.count] = user_data;
    cs->on_insert.count++;

    printf("[Callback] Registered INSERT callback #%d\n", cs->on_insert.count);
}

// Register a DELETE callback
void callbacks_register_delete(CallbackSystem *cs, OnDeleteCallback callback, void *user_data)
{
    if (cs == NULL || callback == NULL)
        return;

    if (cs->on_delete.count >= 20)
    {
        printf("ERROR: Max DELETE callbacks reached (20)\n");
        return;
    }

    cs->on_delete.callbacks[cs->on_delete.count] = callback;
    cs->on_delete.user_data[cs->on_delete.count] = user_data;
    cs->on_delete.count++;

    printf("[Callback] Registered DELETE callback #%d\n", cs->on_delete.count);
}

// Register an UPDATE callback
void callbacks_register_update(CallbackSystem *cs, OnUpdateCallback callback, void *user_data)
{
    if (cs == NULL || callback == NULL)
        return;

    if (cs->on_update.count >= 20)
    {
        printf("ERROR: Max UPDATE callbacks reached (20)\n");
        return;
    }

    cs->on_update.callbacks[cs->on_update.count] = callback;
    cs->on_update.user_data[cs->on_update.count] = user_data;
    cs->on_update.count++;

    printf("[Callback] Registered UPDATE callback #%d\n", cs->on_update.count);
}

// Trigger all INSERT callbacks
void callbacks_trigger_insert(CallbackSystem *cs, Person *record)
{
    if (cs == NULL || record == NULL)
        return;

    printf("\n[Trigger] INSERT event - calling %d callback(s)...\n", cs->on_insert.count);

    for (int i = 0; i < cs->on_insert.count; i++)
    {
        cs->on_insert.callbacks[i](record, cs->on_insert.user_data[i]);
    }

    printf("[Trigger] INSERT callbacks completed\n");
}

// Trigger all DELETE callbacks
void callbacks_trigger_delete(CallbackSystem *cs, int id)
{
    if (cs == NULL)
        return;

    printf("\n[Trigger] DELETE event (ID %d) - calling %d callback(s)...\n", id, cs->on_delete.count);

    for (int i = 0; i < cs->on_delete.count; i++)
    {
        cs->on_delete.callbacks[i](id, cs->on_delete.user_data[i]);
    }

    printf("[Trigger] DELETE callbacks completed\n");
}

// Trigger all UPDATE callbacks
void callbacks_trigger_update(CallbackSystem *cs, Person *old_record, Person *new_record)
{
    if (cs == NULL || old_record == NULL || new_record == NULL)
        return;

    printf("\n[Trigger] UPDATE event (ID %d) - calling %d callback(s)...\n", new_record->id, cs->on_update.count);

    for (int i = 0; i < cs->on_update.count; i++)
    {
        cs->on_update.callbacks[i](old_record, new_record, cs->on_update.user_data[i]);
    }

    printf("[Trigger] UPDATE callbacks completed\n");
}

// Display callback statistics
void callbacks_display_stats(CallbackSystem *cs)
{
    if (cs == NULL)
        return;

      printf("\n=== Callback System Statistics ===\n");
      printf("INSERT callbacks registered: %d\n", cs->on_insert.count);
      printf("DELETE callbacks registered: %d\n", cs->on_delete.count);
      printf("UPDATE callbacks registered: %d\n", cs->on_update.count);
      printf("===================================\n\n");
  }