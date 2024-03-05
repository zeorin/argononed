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

#include "arc_devkit.h"
#include <stdlib.h>

ssize_t get_msg(int socket_fd, arg_msg_t *msg)
{
    ssize_t ret = read(socket_fd, msg, BUFFER_LENGTH);
    if (ret == -1) {
        perror("read() failed");
        return -1;
    }
    if (ret == 0) {
        close(socket_fd);
        return -2;
    }
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
            close(socket_fd);
            return -2;
        }return -1;
    }
    return ret;
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

int is_error_msg(arg_msg_t msg)
{
    if (msg.request == ARG_REQ_NACK) return 1;
    return 0;
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

ar_request_context *new_arc()
{
    return (ar_request_context*)calloc(1, sizeof(ar_request_context));
}

int free_arc(ar_request_context *arc)
{
    if (arc == NULL) return -1;
    free(arc);
    arc = NULL;
    return 0;
}

int open_arc(ar_request_context *arc)
{
    struct sockaddr_un serveraddr;
    int sd = -1;
    int rc = -1;
    do {
        sd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sd < 0)
        {
            //perror("socket() failed");
            break;
        }
        //     fprintf(stderr,"SOCKET\n");
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sun_family = AF_UNIX;
        strcpy(serveraddr.sun_path, SOCKET_NAME);

        rc = connect(sd, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
        if (rc < 0)
        {
            //perror("connect() failed");
            break;
        }
        // fprintf(stderr,"CONNECT\n");
        arg_msg_t msg = { 0 };
        get_msg(sd, &msg);
        // printf("RECV <<\n");
        // print_msg(&msg);
        switch (msg.request)
        {
            case ARG_REQ_NACK:
                // printf("Server is busy and cannot accept new connetions\n");
                goto CLOSE_EXIT;
            case ARG_REQ_REGISTER:
                send_client_reply(sd);
                break;
            default:
                // printf ("Unexpected Responce ");
                // print_msg(&msg);
                goto CLOSE_EXIT;
        }
    }
    while (0);
CLOSE_EXIT:
    if (rc == -1) 
    {
        if (sd) close(sd);
        return -1;
    }
    arc->socket_descriptor = sd;
    arc->status = ARG_CLIENTSAT_INIT;
    return 0;
}

int close_arc(ar_request_context *arc)
{
    if (arc->socket_descriptor == -1) return -1;
    arg_msg_t msg = { 0 , .request = ARG_REQ_COMPLETE };
    send_msg(arc->socket_descriptor, &msg);
    close(arc->socket_descriptor);
    arc->socket_descriptor = -1;
    arc->status = ARG_CLIENTSAT_UINIT;
    return 0;
}

ssize_t send_arc_raw(ar_request_context *arc, arg_msg_t *msg)
{
    if (arc->socket_descriptor == -1) return -1;
    return send_msg(arc->socket_descriptor, msg);
}
ssize_t read_arc_raw(ar_request_context *arc, arg_msg_t *msg)
{
    if (arc->socket_descriptor == -1) return -1;
    return get_msg(arc->socket_descriptor, msg);
}

Schedule arc_get_schedule(ar_request_context *arc)
{
    arg_msg_t msg = { 0 };
    msg.request = ARG_REQ_SCHD;
    send_msg(arc->socket_descriptor, &msg);
    get_msg(arc->socket_descriptor, &msg);
    return msg.schedule;
}

int arc_set_schedule(ar_request_context *arc, Schedule schedule);
int arc_set_fanstage(ar_request_context *arc, uint8_t index, uint8_t speed);
int arc_set_threshold(ar_request_context *arc, uint8_t index, uint8_t temp);
int arc_set_hysteresis(ar_request_context *arc, uint8_t hysteresis)
{
    arg_msg_t msg = { 0 };
    msg.schedule = arc_get_schedule(arc);
    msg.schedule.hysteresis = hysteresis;
    msg.request = ARG_REQ_SCHD;
    msg.data_type = ARG_DATATYPE_SCHD;
    send_msg(arc->socket_descriptor, &msg);
    get_msg(arc->socket_descriptor, &msg);
    return msg.nack.error;
}

int arc_get_mode(ar_request_context *arc);
int arc_set_mode(ar_request_context *arc, uint8_t mode, int8_t speed, int8_t temp);
int arc_get_fanspeed(ar_request_context *arc);
struct SHM_DAEMON_STATS arc_get_status(ar_request_context *arc);

int arc_get_daemon_version(ar_request_context *arc, char* version_string, int len)
{
    if (arc->socket_descriptor == -1) return -1;
    
    arg_msg_t msg = { 0 , .request = ARG_REQ_COMMAND, .command.command_type = ARG_CMD_GET_VER };
    send_msg(arc->socket_descriptor, &msg);
    get_msg(arc->socket_descriptor, &msg);
    if (!is_error_msg(msg))
    {
        strncpy(version_string, msg.string,(len > ARG_MSG_MAX_LEN ?  ARG_MSG_MAX_LEN : len));
        return 0;
    }
    return -1;
}