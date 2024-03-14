/*
MIT License

Copyright (c) 2022 DarkElvenAngel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef AR_REQUEST_H
#define AR_REQUEST_H

#include "argonone_ipc.h"

typedef struct 
{
    argon_clientid_e type;
    int socket_descriptor;
    argon_clientstate_e status;
}ar_request_context;

/**
 * @brief Alocate new Argon Request Context
 * 
 * @return ar_request_context* or NULL
 */
ar_request_context *new_arc();
/**
 * @brief Free an alocated new Argon Request Context
 * 
 * @param arc to be freed
 * @return 0 on success 
 */
int free_arc(ar_request_context *arc);

/**
 * @brief Open connection to argononed
 * 
 * @param arc Argon Request Context
 * @return 0 on success
 */
int open_arc(ar_request_context *arc);
/**
 * @brief Close connection to argononed
 * 
 * @param arc Argon Request Context
 * @return 0 on success 
 */
int close_arc(ar_request_context *arc);

#ifdef ARC_RAW
ssize_t send_arc_raw(ar_request_context *arc, arg_msg_t *msg);
ssize_t read_arc_raw(ar_request_context *arc, arg_msg_t *msg);
#endif

/**
 * @brief request the current schedule setting
 * 
 * @param arc 
 * @return Schedule 
 */
Schedule arc_get_schedule(ar_request_context *arc);
/**
 * @brief Request the schedule be changed
 * 
 * @param arc 
 * @param schedule 
 * @return int 
 */
int arc_set_schedule(ar_request_context *arc, Schedule schedule);
/**
 * @brief Request the fanstage at index be updated
 * 
 * @param arc 
 * @param index 
 * @param speed 
 * @return int 
 */
int arc_set_fanstage(ar_request_context *arc, uint8_t index, uint8_t speed);
/**
 * @brief Request the temperature threshold at index be updated
 * 
 * @param arc 
 * @param index 
 * @param temp 
 * @return int 
 */
int arc_set_threshold(ar_request_context *arc, uint8_t index, uint8_t temp);
/**
 * @brief Request the hysteresis value be changed
 * 
 * @param arc 
 * @param hysteresis 
 * @return int 
 */
int arc_set_hysteresis(ar_request_context *arc, uint8_t hysteresis);

/**
 * @brief Request the current fan mode
 * 
 * @param arc 
 * @return int 
 */
int arc_get_mode(ar_request_context *arc);
/**
 * @brief Request the mode data be updated
 * 
 * setting any paramiter to -1 leaves it unchanged
 * 
 * @param arc 
 * @param mode
 * @param speed 
 * @param temp 
 * @return int 
 */
int arc_set_mode(ar_request_context *arc, uint8_t mode, int8_t speed, int8_t temp);

/**
 * @brief Get current fan speed
 * 
 * @param arc 
 * @return int 
 */
int arc_get_fanspeed(ar_request_context *arc);
/**
 * @brief Request status
 * 
 * @param arc 
 * @return struct SHM_DAEMON_STATS 
 */
struct SHM_DAEMON_STATS arc_get_status(ar_request_context *arc);

/**
 * @brief Request Daemon report version information
 * 
 * @param arc
 * @param version_string is a pointer to a char array 
 * @param len is the length of th char array
 * @return int
 */
int arc_get_daemon_version(ar_request_context *arc, char* version_string, int len);
#endif