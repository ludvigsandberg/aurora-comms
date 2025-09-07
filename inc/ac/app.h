#ifndef AC_APP_H
#define AC_APP_H

#include <stdbool.h>
#include <time.h>

#include <ac/meta.h>
#include <ac/net.h>
#include <ac/str.h>

typedef enum ac_state_e {
    AC_STATE_LOGIN,
    AC_STATE_CHAT,
    AC_STATE_EXIT
} ac_state_t;

typedef struct ac_user_s {
    ac_client_handle_t handle;
    ac_state_t state;
    ac_string_t username;
} ac_user_t;

typedef ac_map(ac_client_handle_t, ac_user_t *) ac_handle_to_user_ptr_map_t;
typedef ac_map(ac_string_t, ac_user_t *) ac_string_to_user_ptr_map_t;

typedef struct ac_app_s {
    ac_server_t server;

    struct {
        /** @brief Maps user handles to user pointers. */
        ac_handle_to_user_ptr_map_t from_handle;
        /** @brief Maps usernames to user pointers. */
        ac_string_to_user_ptr_map_t from_username;
    } users;

    /** @brief Application start time, used for calculating uptime when a user
     * connects. */
    time_t app_start_time;
} ac_app_t;

void ac_user_new(ac_user_t *user, ac_app_t *app, ac_client_handle_t handle);
void ac_user_free(ac_user_t *user);
void ac_user_update(ac_user_t *user, ac_app_t *app, ac_bytes_t *in);

void ac_app_new(ac_app_t *app);
void ac_app_free(ac_app_t *app);
void ac_app_update(ac_app_t *app);

void ac_state_new(ac_user_t *user, ac_app_t *app);
void ac_state_free(ac_user_t *user, ac_app_t *app);
void ac_state_update(ac_user_t *user, ac_app_t *app, ac_bytes_t *in);
void ac_state_switch(ac_user_t *user, ac_app_t *app, ac_state_t state);

#endif
