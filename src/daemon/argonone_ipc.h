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
#ifndef IPC_FUNC_H
#define IPC_FUNC_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <signal.h>
#include <errno.h>
#include "argononed.common.h"

#define SOCKET_NAME     "/tmp/argonone"
#define BUFFER_LENGTH    sizeof(arg_msg_t)
#define ARG_MSG_MAX_LEN  20

typedef enum {
    ARG_REQ_PING    = 0,    // Request PING 
    ARG_REQ_PONG,           // Request PING 
    ARG_REQ_ACK,            // Acknowledge
    ARG_REQ_NACK,           // Negative Acknowledge
    ARG_REQ_MODE,           // Request Mode change
    ARG_REQ_SCHD,           // Request schedule data or change
    ARG_REQ_COMMAND,        // Request commang
    ARG_REQ_STATUS,         // Request status
    ARG_REQ_ERRORS,         // Request Error
    ARG_REQ_COMPLETE,       // Complete Transaction and disconnect
    ARG_REQ_CONFIRM,        // Request Confirmation 
    ARG_REQ_REGISTER,       // Request Client details
    ARG_REQ_DATAPACKET,     // Request data packet
    ARG_REQ_ERR     = -1,   // error
} argon_req_e;

typedef enum {
    ARG_ERR_GENERAL     = 0,// General Error : 
    ARG_ERR_BUSY,           // Server is busy and cannot connect or process request
    ARG_ERR_INVALID,        // The request was invalid
    ARG_ERR_CRC,            // The CRC check of the last packet failed
    ARG_ERR_CONFIRM,        // Cannot complete action confirm failed
    ARG_ERR_INCOMPLETE,     // Request incomplete missing data.
    ARG_ERR_CMD_RANGE,      // Requested command is out of range
    ARG_ERR_UNSUPPORTED,    // Request is unsupported
    ARG_ERR_CLIENTID,       // Client ID is not valid
    ARG_ERR_D_CRITICAL,     // Daemon error codes for retrival not an error
    ARG_ERR_D_ERROR,        // Daemon error codes for retrival not an error
    ARG_ERR_D_WARNING       // Daemon error codes for retrival not an error
} argon_err_e;

typedef enum {              // Clinet ID 0 is invalid.
    ARG_CLIENTID_CLI    = 1,// Command Line Interface client
    ARG_CLIENTID_CRL,       // Command Line Interface client extra features
    ARG_CLIENTID_LOG,       // Logging client
    ARG_CLIENTID_MON,       // Monitoring Client
    ARG_CLIENTID_APP,       // Application Client
    ARG_CLIENTID_OTHER,     // 
} argon_clientid_e;

typedef enum {
    ARG_DATATYPE_NULL   = 0,
    ARG_DATATYPE_SCHD,
    ARG_DATATYPE_STATUS,
    ARG_DATATYPE_MODE,
    ARG_DATATYPE_ERROR,
    ARG_DATATYPE_COMMAND,
    ARG_DATATYPE_NACK,
    ARG_DATATYPE_CLIENT,
    ARG_DATATYPE_BYTE,
    ARG_DATATYPE_INT,
    ARG_DATATYPE_STRING,
} argon_data_type_e;

typedef enum {
    ARG_CMD_GET_VER     = 0,
} argon_command_type_e;

typedef enum {
    ARG_CLIENTSAT_UINIT = 0, // Clinet is uninitialized
    ARG_CLIENTSAT_INIT,      // Clinet is initialized
    ARG_CLIENTSAT_ERROR,     // Clinet in error condition
} argon_clientstate_e;

static const char* argon_req_str[] __attribute__((unused)) = {
    "ARG_REQ_PING",
    "ARG_REQ_PONG",
    "ARG_REQ_ACK",
    "ARG_REQ_NACK",
    "ARG_REQ_MODE",
    "ARG_REQ_SCHD",
    "ARG_REQ_COMMAND",
    "ARG_REQ_STATUS",
    "ARG_REQ_ERRORS",
    "ARG_REQ_COMPLETE",
    "ARG_REQ_CONFIRM",
    "ARG_REQ_REGISTER",
    "ARG_REQ_DATAPACKET",
    "ARG_REQ_ERR",};

static const char* argon_err_str[] __attribute__((unused)) = {
    "General Error",
    "Server is busy and cannot connect or process request",
    "The request was invalid",
    "The CRC check of the last packet failed",
    "Cannot complete action confirm failed",
    "Request incomplete missing data.",
    "Requested command is out of range",
    "Request is unsupported",
    "Client ID is not valid",
    "Daemon error codes for retrival not an error",
    "Daemon error codes for retrival not an error",
    "Daemon error codes for retrival not an error",
};

static const char* argon_clientid_str[] __attribute__((unused)) = {
    "Invalid client",
    "Command Line Interface client",
    "Command Line Interface client extra features",
    "Logging client",
    "Monitoring Client",
    "Application Client",
    "Other",
};

static const char* RUN_STATE_STR[4] __attribute__((unused)) = {"AUTO", "OFF", "MANUAL", "COOLDOWN"};

typedef struct {
    uint8_t client_id;
    pid_t   client_pid;
} argon_reg;

#pragma pack(1)
typedef struct {
    union {
        struct 
        {
            uint8_t client_uid  : 3;
            uint8_t reserved    : 5;
            uint8_t operation   : 2;
            uint8_t cmd_req     : 6;
        };    
        int16_t request;                        // request - define the type of request
    };
    uint8_t data_type;                          // data_type - what type of data is stored in union
    union 
    {
        Schedule schedule;                      // schedule - schedule setting fan speed, thersholds, hysteresis
        struct {
            uint8_t current_temperature;
            uint8_t max_temperature;
            uint8_t min_temperature;
            uint8_t fanspeed;
            uint8_t fanmode;
            uint8_t EF_Warning;
            uint8_t EF_Error;
            uint8_t EF_Critical;
            uint8_t EF_Flags;   
        } status;                               // status - daemon status data
        struct {
            uint8_t fanmode;
            uint8_t temperature_target;
            uint8_t fanspeed_Overide;
        } mode;                                 // mode - mode change
        struct {
            uint8_t error_level;
            uint32_t error_code;
            uint8_t more;
        } error;                                // error - not used
        struct {
            uint16_t command_type;
        } command;                              // command - undefined
        struct {
            argon_err_e error;
        } nack;                                 // nack - contains nack error
        struct {
            argon_clientid_e id;
            pid_t pid;
        } client;                               // client - client data
        int integer;                            // integer - 32 bit number
        char byte;                              // byte - 8 bit number
        char string[20];                        // string - max length 20 can be unterminated
    };
    uint8_t crc_8;
} arg_msg_t;

typedef struct 
{
    argon_clientid_e type;
    pid_t pid;
    int socket_descriptor;
    argon_clientstate_e status;
} argon_client_context;


#ifndef AR_REQUEST_H
void set_HOOK_Disconnect (int (*ptr)(int));

int initialize_ipc_socket();
int close_ipc_socket();

const char *print_msg(arg_msg_t *msg);

#endif
ssize_t send_ack(int socket_fd);
ssize_t send_ackmsg(int socket_fd, arg_msg_t *msg);
ssize_t send_nack(int socket_fd, argon_err_e error);
ssize_t get_msg(int socket_fd, arg_msg_t *msg);
ssize_t send_msg(int socket_fd, arg_msg_t *msg);

ssize_t send_client_reply(int socket_fd);
char CRC8(const char *data,int length);

// int ar_ipcc_process_request(int socket_fd, arg_msg_t *msg);

#endif