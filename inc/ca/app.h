#ifndef CA_APP_H
#define CA_APP_H

#include <stdbool.h>
#include <time.h>

#include <ca/meta.h>
#include <ca/net.h>
#include <ca/string.h>

typedef enum ca_state_e {
    CA_STATE_LOGIN,
    CA_STATE_CHAT,
    CA_STATE_EXIT
} ca_state_t;

typedef struct ca_user_s {
    ca_client_handle_t handle;
    ca_state_t state;
    ca_string_t username;
} ca_user_t;

typedef ca_map(ca_client_handle_t, ca_user_t *) ca_handle_to_user_ptr_map_t;
typedef ca_map(ca_string_t, ca_user_t *) ca_string_to_user_ptr_map_t;

typedef struct ca_app_s {
    ca_server_t server;

    struct {
        /** @brief Maps user handles to user pointers. */
        ca_handle_to_user_ptr_map_t from_handle;
        /** @brief Maps usernames to user pointers. */
        ca_string_to_user_ptr_map_t from_username;
    } users;

    /** @brief Application start time, used for calculating uptime when a user
     * connects. */
    time_t app_start_time;
} ca_app_t;

void ca_user_new(ca_user_t *user, ca_app_t *app, ca_client_handle_t handle);
void ca_user_free(ca_user_t *user);
void ca_user_update(ca_user_t *user, ca_app_t *app, ca_bytes_t *in);

void ca_app_new(ca_app_t *app);
void ca_app_free(ca_app_t *app);
void ca_app_update(ca_app_t *app);

void ca_state_new(ca_user_t *user, ca_app_t *app);
void ca_state_free(ca_user_t *user, ca_app_t *app);
void ca_state_update(ca_user_t *user, ca_app_t *app, ca_bytes_t *in);
void ca_state_switch(ca_user_t *user, ca_app_t *app, ca_state_t state);

#endif
