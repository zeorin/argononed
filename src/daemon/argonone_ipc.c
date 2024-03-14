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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <signal.h>
#include <errno.h>

#include "argononed.common.h"
#include "argonone_ipc.h"
#include "argonone_shm.h"

static void *_ipc_thread();
static pthread_t ipc_pthread;

#define MAX_CONNECTIONS  5
argon_client_context sock_clients[MAX_CONNECTIONS];

extern Daemon_Conf Configuration;

void trace_log_msg(arg_msg_t *msg) __attribute__((no_instrument_function));
const char* print_msg(arg_msg_t *msg) __attribute__((no_instrument_function));

static void init_sock_fds()
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) 
    {
        sock_clients[i].socket_descriptor = -1;
        sock_clients[i].status = 0;
        sock_clients[i].type = 0;
    }
}
static int add_sock_fds(int fd)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (sock_clients[i].socket_descriptor == -1)
        {
            sock_clients[i].socket_descriptor = fd;
            return 0;
        }
    }
    return -1;
}
static int rm_sock_fds(int fd)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (sock_clients[i].socket_descriptor == fd)
        {
            sock_clients[i].socket_descriptor = -1;
            sock_clients[i].status = 0;
            sock_clients[i].type = 0;
            return 0;
        }
    }
    return -1;
}
static void refresh_sock_fds(fd_set *fdsetp)
{
    FD_ZERO(fdsetp);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (sock_clients[i].socket_descriptor != -1)
        {
            FD_SET(sock_clients[i].socket_descriptor, fdsetp);
        }
    }
}
static int max_sock_fds()
{
    int max = -1;
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (sock_clients[i].socket_descriptor > max) max = sock_clients[i].socket_descriptor;
    }
    return max;
}
static int count_sock_fds()
{
    int count = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (sock_clients[i].socket_descriptor > -1) count++;
    }
    return count;
}
static int index_from_fds(int fds)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (sock_clients[i].socket_descriptor == fds) return i;
    }
    return -1;
}
static int set_client_type(int fds, argon_clientid_e type)
{
    int i = index_from_fds(fds);
    if (i >= 0)
    {
        sock_clients[i].type = type;
        return 0;
    }
    return -1;
}
static int set_client_status (int fds, argon_clientstate_e status)
{
    int i = index_from_fds(fds);
    if (i >= 0)
    {
        sock_clients[i].status = status;
        return 0;
    }
    return -1;
}
static int set_client_pid (int fds, pid_t pid)
{
    int i = index_from_fds(fds);
    if (i >= 0)
    {
        sock_clients[i].pid = pid;
        return 0;
    }
    return -1;
}

int act_msg(int socket_fd, arg_msg_t *msg)
{
    // trace_log_msg(msg);
    switch (msg->request)
    {
        case ARG_REQ_PING :
            log_message(LOG_DEBUG, "IPC: RECV << Ping");
            send_msg(socket_fd, msg);
            usleep(1000);
            break;
        case ARG_REQ_ACK : // needs a context
        case ARG_REQ_NACK : // needs a context
            send_nack(socket_fd, ARG_ERR_UNSUPPORTED);
            break;
        case ARG_REQ_MODE :
            log_message(LOG_INFO, "IPC: Request mode change");
            if (Configuration.runstate != msg->mode.fanmode && msg->mode.fanmode < 4)
            {
                Configuration.runstate = msg->mode.fanmode;
                log_message(LOG_INFO,"Changing Mode to [%s]", RUN_STATE_STR[msg->mode.fanmode]);
            }
            if (Configuration.temperature_target != msg->mode.temperature_target && msg->mode.temperature_target >= 30)
            {
                Configuration.temperature_target = msg->mode.temperature_target;
                log_message(LOG_INFO,"Set temperature target to %d", Configuration.temperature_target);
            }
            if(Configuration.fanspeed_Overide != msg->mode.fanspeed_Overide && msg->mode.fanspeed_Overide <= 100)
            {
                Configuration.fanspeed_Overide   = msg->mode.fanspeed_Overide;
                log_message(LOG_INFO,"Set overide fanspeed to %d%%", Configuration.fanspeed_Overide);
            }
            reset_shm();
            send_ack(socket_fd);
            break;
        case ARG_REQ_SCHD :
            if (msg->data_type == ARG_DATATYPE_SCHD)
            {
                log_message(LOG_INFO, "IPC: Request schedule change");
                for (int i = 0; i < 3; i++)
                {
                    int lastval = 30;
                    if (msg->schedule.thresholds[i] < lastval)
                    {
                        log_message(LOG_WARN,"IPC: Message contains bad value at threshold %d ABORTING request", i);
                        //set_errorflag_ipc(1);
                        send_nack(socket_fd,ARG_ERR_INVALID);
                        return -1;
                    }
                }
                log_message(LOG_DEBUG,"Threshold values look good");
                for (int i = 0; i < 3; i++)
                {
                    if (msg->schedule.fanstages[i] > 100 )
                    {
                        log_message(LOG_WARN,"IPC: Message contains bad value at fanstage %d ABORTING reload", i);
                        //set_errorflag_ipc(1);
                        send_nack(socket_fd,ARG_ERR_INVALID);
                        return -1;
                    }
                }
                log_message(LOG_DEBUG,"Fan speed values look good");
                memcpy(&Configuration.configuration.fanstages,
                    msg->schedule.fanstages,
                    sizeof(Configuration.configuration.fanstages)
                    );
                memcpy(&Configuration.configuration.thresholds,
                    msg->schedule.thresholds,
                    sizeof(Configuration.configuration.thresholds)
                    );
                if (msg->schedule.hysteresis > 10) // soft error 
                {
                    log_message(LOG_INFO,"IPC: Message contains bad value at hysteresis FORCING to 10");
                    msg->schedule.hysteresis = 10;
                }
                Configuration.configuration.hysteresis = msg->schedule.hysteresis;
                log_message(LOG_INFO,"Schedule update successful");    
                reset_shm();
                send_ack(socket_fd);
            } else {
                log_message(LOG_INFO, "IPC: Request schedules");
                msg->data_type = ARG_DATATYPE_SCHD;
                memcpy(&msg->schedule,&Configuration.configuration,sizeof(Schedule));
                send_ackmsg(socket_fd, msg);
            }
            // send_nack(socket_fd, ARG_ERR_INVALID);
            break;
        case ARG_REQ_COMMAND :
            log_message(LOG_DEBUG, "IPC: RECV << Command");
            switch (msg->command.command_type)
            {
                case ARG_CMD_GET_VER:
                    send_nack(socket_fd, ARG_ERR_CMD_RANGE);
                    break;
                default:
                    send_nack(socket_fd, ARG_ERR_CMD_RANGE);  
            }
            break;
        case ARG_REQ_STATUS :
            log_message(LOG_DEBUG, "IPC: RECV << Status Request");
            send_nack(socket_fd, ARG_ERR_INVALID);
            break;
        case ARG_REQ_ERRORS :
            log_message(LOG_DEBUG, "IPC: RECV << Error Request");
            send_nack(socket_fd, ARG_ERR_UNSUPPORTED);
            break;
        case ARG_REQ_COMPLETE : 
            log_message(LOG_INFO, "IPC: Client disconnecting [closing connection]");
            memset(msg, 0, sizeof(arg_msg_t));
            msg->request=ARG_REQ_COMPLETE;
            send_msg(socket_fd,msg);
            close(socket_fd);
            rm_sock_fds(socket_fd);
        break;
        default: 
            log_message(LOG_DEBUG, "IPC: RECV << UNKNOWN Request");
            send_nack(socket_fd, ARG_ERR_INVALID);
            return -1;
    }
    return 0;
}

int New_Client(int connection_socket)
{
    log_message(LOG_INFO,"IPC: New Client Connection Attempt");
    int data_socket = accept(connection_socket, NULL, NULL);
    if (data_socket < 0)
    {
        perror("accept() failed");
        return -1;
    }
    arg_msg_t msg = { 0 };
    if (count_sock_fds() >= MAX_CONNECTIONS -1)
    {
        send_nack(data_socket,ARG_ERR_BUSY);
        log_message(LOG_INFO,"IPC: Rejecting client Max connections reached");
        close(data_socket);
        return 0;
    }
    fd_set set = { 0 };
    FD_ZERO(&set);
    FD_SET(data_socket, &set);
    struct timeval timeout={.tv_sec=0, .tv_usec=250000};
    log_message(LOG_DEBUG,"IPC: Query Client type");
    msg.request = ARG_REQ_REGISTER;
    if (send_msg(data_socket,&msg) < 0) goto Client_Error;
    if (select(data_socket + 1, &set, NULL, NULL, &timeout) <= 0)
    {
        log_message(LOG_DEBUG,"IPC: Client responce timeout");
        goto Client_Error;
    }
    if (get_msg(data_socket,&msg) < 0) goto Client_Error;
    if (msg.request != ARG_REQ_ACK) goto Client_Error;
    //memset(&msg,0,BUFFER_LENGTH);
    //msg.request = ARG_REQ_ACK;
    //if (send_msg(data_socket,&msg) < 0) goto Client_Error;
    add_sock_fds(data_socket);
    set_client_pid(data_socket,msg.client.pid);
    set_client_type(data_socket,msg.client.id);
    set_client_status(data_socket,ARG_CLIENTSAT_INIT);
    log_message(LOG_INFO,"IPC: New Client Connected");
    return 1;
Client_Error:
    log_message(LOG_DEBUG,"IPC: Client Rejected malformed responce");
    close(data_socket);
    return -1;
}

int initialize_ipc_socket()
{
    if(pthread_create(&ipc_pthread, NULL, _ipc_thread, NULL))
    {
        /*Thread creation failed*/
        return 0;
    }
    return 1;
}

int close_ipc_socket()
{
    pthread_cancel(ipc_pthread);
    pthread_join(ipc_pthread, NULL);
    log_message(LOG_INFO,"Shutting down IPC Connectios");
    if (sock_clients[0].socket_descriptor != -1)
    {
        rm_sock_fds(sock_clients[0].socket_descriptor);
        close(sock_clients[0].socket_descriptor);
    }
    for(int i = 1; i < MAX_CONNECTIONS; i++)
    {
        if (sock_clients[i].socket_descriptor == -1 /*|| sock_fds[i] == connection_socket*/) continue;
        arg_msg_t msg;
        int data_socket = sock_clients[i].socket_descriptor;
        memset(&msg, 0, sizeof(arg_msg_t));
        msg.request=ARG_REQ_COMPLETE;
        send_msg(data_socket,&msg);
        rm_sock_fds(data_socket);
        close(data_socket);
    }
    log_message(LOG_INFO,"Cleaup IPC Socket");
    unlink(SOCKET_NAME);
    return 0;
}

void *_ipc_thread()
{
    int    connection_socket, data_socket;
    int    ret;//, length;
    //char   buffer[BUFFER_LENGTH];
    struct sockaddr_un serveraddr;
    fd_set socket_set;
    //int comm_socket_fd, i;

    init_sock_fds();

    connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connection_socket < 0)
    {
        perror("socket() failed");
    }
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, SOCKET_NAME);

    ret = bind(connection_socket, (struct sockaddr *)&serveraddr, (socklen_t)SUN_LEN(&serveraddr));
    if (ret < 0)
    {
        perror("bind() failed");
        // break;
    }
    ret = listen(connection_socket, 5);
    if (ret< 0)
    {
        perror("listen() failed");
        // break;
    }
    add_sock_fds(connection_socket);
    log_message(LOG_INFO,"IPC Socket Ready for client");

    while(1)
    {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_testcancel();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        refresh_sock_fds(&socket_set);
        struct timeval timeout={.tv_sec=1, .tv_usec=0};
        int ret = select(max_sock_fds() + 1, &socket_set, NULL, NULL, &timeout);
        if (ret == 0) continue; // timed out
        if (ret == -1) { break; }
        if (FD_ISSET(connection_socket, &socket_set))
        {
            New_Client(connection_socket);
            continue;
        }
        else 
        {
            for(int i = 0; i < MAX_CONNECTIONS; i++){
                if(FD_ISSET(sock_clients[i].socket_descriptor, &socket_set))
                {
                    arg_msg_t msg;
                    memset(&msg, 0, BUFFER_LENGTH);
                    data_socket = sock_clients[i].socket_descriptor;
                    //printf("Client %d Data\n", i);
                    if (get_msg(data_socket, &msg) <= 0) continue;
                    act_msg(data_socket, &msg);
                    continue;
                }
            }
        }
    }
    return NULL;
}

static int (*HOOK_Disconnect)(int) = NULL;

void set_HOOK_Disconnect (int (*ptr)(int))
{
    HOOK_Disconnect = ptr;
}

void call_HOOK_Disconnect(int arg)
{
    if (HOOK_Disconnect != NULL)
    {
        HOOK_Disconnect(arg);
    }
}

ssize_t get_msg(int socket_fd, arg_msg_t *msg)
{
    ssize_t ret = read(socket_fd, msg, BUFFER_LENGTH);
    if (ret == -1) {
        perror("read() failed");
        return -1;
    }
    if (ret == 0) {
        log_message(LOG_INFO,"client %d disconnected!",index_from_fds(socket_fd));
        close(socket_fd);
        call_HOOK_Disconnect(socket_fd);
        rm_sock_fds(socket_fd);
        return -2;
    }
    // log_message(LOG_DEBUG, "IPC: RECV << %02X: %s [ 0x%02X ]\n", msg->request, argon_req_str[msg->request],msg->crc_8);
    log_message(LOG_DEBUG, "IPC: RECV << %s", print_msg(msg));
    if (msg->crc_8 != CRC8((char*)msg,BUFFER_LENGTH - 1))
        send_nack(socket_fd, ARG_ERR_CRC);
    return ret;
}
ssize_t send_msg(int socket_fd, arg_msg_t *msg)
{
    msg->crc_8 = CRC8((char*)msg,BUFFER_LENGTH - 1);
    ssize_t ret = send(socket_fd,msg,BUFFER_LENGTH,0);
    if (ret == -1) {
        if (errno == EPIPE)
        {
            log_message(LOG_INFO,"client %d disconnected!",index_from_fds(socket_fd));
            close(socket_fd);
            call_HOOK_Disconnect(socket_fd);
            rm_sock_fds(socket_fd);
            return -2;
        }
        log_message(LOG_ERROR,"\x1b[1mIPC:\x1b[0m %s", strerror(errno));
        return -1;
    }
    //printf("SEND >> %02X: %s [ 0x%02X ]\n", msg->request, argon_req_str[msg->request],msg->crc_8);
    log_message(LOG_DEBUG, "IPC: SEND >> %s", print_msg(msg));
    return ret;
}

ssize_t send_datapacket(int socket_fd, arg_msg_t *msg)
{
    msg->request=ARG_REQ_DATAPACKET;
    return send_msg(socket_fd,msg);
}

ssize_t send_ack(int socket_fd)
{
    arg_msg_t msg;
    memset(&msg, 0, sizeof(arg_msg_t));
    msg.request=ARG_REQ_ACK;
    return send_msg(socket_fd,&msg);
}

ssize_t send_ackmsg(int socket_fd, arg_msg_t *msg)
{
    msg->request=ARG_REQ_ACK;
    return send_msg(socket_fd,msg);
}

ssize_t send_nack(int socket_fd, argon_err_e error)
{
    arg_msg_t msg;
    memset(&msg, 0, sizeof(arg_msg_t));
    msg.request=ARG_REQ_NACK;
    msg.nack.error = error;
    return send_msg(socket_fd,&msg);
}

ssize_t send_client_reply(int socket_fd)
{
    arg_msg_t msg = { 0 };
    msg.data_type = ARG_DATATYPE_CLIENT;
    msg.client.id = ARG_CLIENTID_OTHER;
    msg.client.pid = getpid();
    return send_ackmsg(socket_fd,&msg);
}

char CRC8(const char *data,int length) 
{
   char crc = 0x00;
   char extract;
   char sum;
   for(int i=0;i<length;i++)
   {
      extract = *data;
      for (char tempI = 8; tempI; tempI--) 
      {
         sum = (crc ^ extract) & 0x01;
         crc >>= 1;
         if (sum)
            crc ^= 0x8C;
         extract >>= 1;
      }
      data++;
   }
   return crc;
}
static const char* print_msg_data(arg_msg_t *msg)
{ 
    static char return_char[512] = {0};
    memset(&return_char,0, 512);
    switch (msg->data_type)
    {
        case ARG_DATATYPE_NULL:
            sprintf(return_char,"NULL");
            break;
        case ARG_DATATYPE_SCHD:
            sprintf(return_char,"SCHD");
            break;
        case ARG_DATATYPE_STATUS:
            sprintf(return_char,"STATUS");
            break;
        case ARG_DATATYPE_MODE:
            sprintf(return_char,"MODE");
            break;
        case ARG_DATATYPE_ERROR:
            sprintf(return_char,"ERROR");
            break;
        case ARG_DATATYPE_COMMAND:
            sprintf(return_char,"COMMAND");
            break;
        case ARG_DATATYPE_NACK:
            sprintf(return_char,"NACK");
            break;
        case ARG_DATATYPE_CLIENT:
            sprintf(return_char,"CLIENT %s @ %d", argon_clientid_str[msg->client.id], msg->client.pid);
            break;
        case ARG_DATATYPE_BYTE:
            sprintf(return_char,"BYTE");
            break;
        case ARG_DATATYPE_INT:
            sprintf(return_char,"INT");
            break;
        case ARG_DATATYPE_STRING:
            sprintf(return_char,"STRING");
            break;
    }
    return return_char;
}
void trace_log_msg(arg_msg_t *msg)
{
    if (Configuration.Log_Level != 7) return;
    for (uint32_t i = 0; i < sizeof(arg_msg_t);i++)
    {
        uint8_t c = 97;
        if (i < 2) c = 95;
        if (i == 2) c = 35;
        if (i == 23) c = 92;
        printf("\x1b[%d;7m%02X \x1b[0m",c,  *((char*)msg + i));
    }
    printf("\n"); 
}
const char* print_msg(arg_msg_t *msg)
{
    trace_log_msg(msg);
    static char return_char[256] = { 0 };
    memset(&return_char,0, 256);
    if (msg->request < 14)
    {
        sprintf(return_char,"%s : ", argon_req_str[msg->request]);
        switch (msg->request)
        {
            case ARG_REQ_NACK:
                strcat(return_char,argon_err_str[msg->nack.error]);
                break;
            case ARG_REQ_ACK:
            case ARG_REQ_DATAPACKET:
                strcat(return_char,print_msg_data(msg));
                break;
            default:
                break;
        }
    } else {
        sprintf(return_char,"REQUEST OUT OF RANGE!");
    }
    return return_char;
}
