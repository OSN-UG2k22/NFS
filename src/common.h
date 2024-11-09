#ifndef _COMMON_H
#define _COMMON_H

#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>

#define FILENAME_MAX_LEN 4096

#define NS_MAX_CONN 1024

#define DEFAULT_HOST "127.0.0.1"
#define NS_DEFAULT_PORT "4269"
#define SS_DEFAULT_PORT "6942"

/* Any message sent over the network must have one of these values in the
 * header */
typedef enum _Operation
{
    OP_ACK,            /* MessageInt */
    OP_RAW,            /* Message */
    OP_NS_INIT_SS,     /* MessageInt */
    OP_NS_INIT_CLIENT, /* Message */
    OP_NS_INIT_FILE,   /* MessageFile */
    OP_NS_CREATE,      /* MessageFile */
    OP_NS_DELETE,      /* MessageFile */
    OP_NS_COPY,        /* MessageFile2 */
    OP_NS_GET_SS,      /* MessageFile */
    OP_NS_REPLY_SS,    /* MessageAddr */
    OP_NS_LS,          /* Message */
    OP_SS_READ,        /* MessageFile */
    OP_SS_WRITE,       /* MessageFile */
    OP_SS_INFO,        /* MessageFile */
    OP_SS_STREAM,      /* MessageFile */
} Operation;

typedef enum _ErrCode
{
    ERR_NONE, /* No error, successful transaction */
    ERR_CONN, /* Some error in connection */
} ErrCode;

typedef struct _Message
{
    Operation op;
    int size;
} Message;

typedef struct _MessageFile
{
    Operation op;
    int size;
    char file[FILENAME_MAX_LEN];
} MessageFile;

typedef struct _MessageFile2
{
    Operation op;
    int size;
    char file[FILENAME_MAX_LEN];
    char file2[FILENAME_MAX_LEN];
} MessageFile2;

typedef struct _MessageInt
{
    Operation op;
    int size;
    int info;
} MessageInt;

typedef struct _MessageAddr
{
    Operation op;
    int size;
    struct sockaddr addr;
} MessageAddr;

typedef struct _SServerInfo
{
    struct sockaddr_in addr;
    int sock_fd;
    int is_used;
} SServerInfo;

void ipv4_print_addr(struct sockaddr_in *addr, const char *interface);

int sock_connect(char *node, char *port, char *listen_port);
int sock_init(char *port);
int sock_accept(
    int sock_fd, struct sockaddr_in *sock_addr, struct sockaddr_in *ss_sock_addr);
int sock_send(int sock, Message *message);
Message *sock_get(int sock);

#endif
