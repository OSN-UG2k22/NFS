#include "common.h"

int sock_send(int sock, Message *message)
{
    switch (message->op)
    {
    case OP_NS_INIT_CLIENT:
    case OP_NS_LS:
        message->size = 0;
        break;
    case OP_NS_COPY:
        message->size = 2 * FILENAME_MAX_LEN;
        break;
    case OP_NS_CREATE:
    case OP_NS_DELETE:
    case OP_SS_READ:
    case OP_SS_WRITE:
    case OP_SS_INFO:
    case OP_SS_STREAM:
    case OP_NS_GET_SS:
        message->size = FILENAME_MAX_LEN;
        break;
    case OP_NS_INIT_SS:
    case OP_ACK:
        message->size = sizeof(int);
        break;
    case OP_NS_REPLY_SS:
        message->size = sizeof(struct sockaddr);
        break;
    case OP_RAW:
        break;
    default:
        return -1;
    }
    return write(sock, message, sizeof(Message) + message->size) == sizeof(Message);
}

Message *sock_get(int sock)
{
    Message *ret = malloc(sizeof(Message));
    if (!ret)
    {
        return 0;
    }
    if (read(sock, ret, sizeof(Message)) != sizeof(Message))
    {
        return 0;
    }
    Message *new_ret = realloc(ret, sizeof(Message) + ret->size);
    if (!new_ret)
    {
        free(ret);
        return 0;
    }
    if (read(sock, (void *)new_ret + sizeof(Message), new_ret->size) != new_ret->size)
    {
        free(new_ret);
        return 0;
    }
    return new_ret;
}

void ipv4_print_addr(struct sockaddr_in *addr, const char *interface)
{
    if (addr && addr->sin_family == AF_INET)
    {
        char ip_str[16];
        inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));
        if (interface)
        {
            printf("%s (%s)\n", ip_str, interface);
        }
        else
        {
            printf("%s:%d\n", ip_str, ntohs(addr->sin_port));
        }
    }
    else if (!interface)
    {
        printf("unknown family: %d\n", addr->sin_family);
    }
}

int ipv4_is_wildcard(struct sockaddr *addr)
{
    if (addr && addr->sa_family == AF_INET)
    {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
        return addr_in->sin_addr.s_addr == INADDR_ANY;
    }
    return 0;
}

int _sock_print_info(char *port)
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return -1;
    }

    printf("Server listening in wildcard IPv4 address on port %s\n", port);
    printf("Here are some possible addresses clients can use:\n");
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        ipv4_print_addr((struct sockaddr_in *)ifa->ifa_addr, ifa->ifa_name);
    }

    freeifaddrs(ifaddr);
    return 0;
}

int sock_init(char *port)
{
    int status, ret;
    struct addrinfo hints, *res, *i;
    if (_sock_print_info(port) < 0)
    {
        goto error;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, port, &hints, &res)))
    {
        fprintf(stderr,
                "Could not connect to server: %s\n",
                gai_strerror(status));
        return -1;
    }

    for (i = res; i; i = i->ai_next)
    {
        if (ipv4_is_wildcard(i->ai_addr))
        {
            ret = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
            if (ret < 0)
            {
                goto error;
            }
            if (bind(ret, i->ai_addr, i->ai_addrlen) != 0)
            {
                goto error;
            }
            if (listen(ret, NS_MAX_CONN) != 0)
            {
                goto error;
            }
        }
    }
    goto end;
error:
    perror("Could not start server");
    ret = -1;
end:
    freeaddrinfo(res);
    return ret;
}

int sock_connect(char *node, char *port, char *listen_port)
{
    struct addrinfo *res, *i;
    int status;
    int sock_fd = -1;
    if ((status = getaddrinfo(node, port, NULL, &res)))
    {
        fprintf(stderr,
                "Could not connect to server: %s\n",
                gai_strerror(status));
        return -1;
    }
    for (i = res; i; i = i->ai_next)
    {
        sock_fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (sock_fd >= 0)
        {
            if (connect(sock_fd, i->ai_addr, i->ai_addrlen) == 0)
            {
                break;
            }
            close(sock_fd);
            sock_fd = -1;
        }
    }

    freeaddrinfo(res);
    if (sock_fd == -1)
    {
        fprintf(stderr, "Could not connect to server\n");
        return -1;
    }

    if (listen_port)
    {
        MessageInt msg;
        msg.info = atoi(listen_port);
        msg.op = OP_NS_INIT_SS;
        sock_send(sock_fd, (Message *)&msg);
    }
    else
    {
        Message msg;
        msg.op = OP_NS_INIT_CLIENT;
        sock_send(sock_fd, &msg);
    }

    return sock_fd;
}

int sock_accept(
    int sock_fd, struct sockaddr_in *sock_addr, struct sockaddr_in *ss_sock_addr)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int ret = accept(sock_fd, (struct sockaddr *)sock_addr, &addrlen);
    if (ret < 0)
    {
        perror("[SERVER] Accept failed");
        return -1;
    }
    Message *init_msg = sock_get(ret);
    if (!init_msg)
    {
        close(ret);
        return -1;
    }
    switch (init_msg->op)
    {
    case OP_NS_INIT_SS:
        printf("[SERVER] Connected storage server: ");
        ipv4_print_addr(sock_addr, NULL);

        printf("[SERVER] That storage server is listening on: ");
        *ss_sock_addr = *sock_addr;
        ss_sock_addr->sin_port = htons(((MessageInt *)init_msg)->info);
        ipv4_print_addr(ss_sock_addr, NULL);

        break;
    case OP_NS_INIT_CLIENT:
        printf("[SERVER] Connected client: ");
        ipv4_print_addr(sock_addr, NULL);
        break;
    default:
        close(ret);
        ret = -1;
    }
    free(init_msg);
    return ret;
}
