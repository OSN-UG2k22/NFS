#include "common.h"

pthread_mutex_t sservers_lock = PTHREAD_MUTEX_INITIALIZER;
SServerInfo sservers[NS_MAX_CONN] = {0};

SServerInfo *sserver_register(struct sockaddr_in *addr, int sock_fd)
{
    SServerInfo *ret = NULL;
    pthread_mutex_lock(&sservers_lock);
    for (int i = 0; i < NS_MAX_CONN; i++)
    {
        if (!sservers[i].is_used)
        {
            sservers[i].addr = *addr;
            sservers[i].sock_fd = sock_fd;
            sservers[i].is_used = 1;
            ret = &sservers[i];
            break;
        }
    }
    pthread_mutex_unlock(&sservers_lock);
    return ret;
}

ErrCode sserver_by_path(char *path, struct sockaddr_in *addr)
{
    int ret = ERR_NONE;
    if (!path || !path[0])
    {
        return ERR_REQ;
    }

    pthread_mutex_lock(&sservers_lock);
    /* TODO: actual implementation */
    if (!sservers[0].is_used)
    {
        ret = ERR_SS;
    }
    else
    {
        *addr = sservers[0].addr;
    }
    pthread_mutex_unlock(&sservers_lock);
    return ret;
}

void *handle_client(void *client_socket)
{
    int sock = (int)(intptr_t)client_socket;
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(sock);
        if (!msg)
        {
            break;
        }

        ErrCode ecode = ERR_NONE;
        MessageAddr reply_addr;
        reply_addr.op = OP_NS_REPLY_SS;
        switch (msg->op)
        {
        case OP_NS_CREATE:
            printf("[CLIENT %d] Created file '%s'\n", sock, msg->file);
            sock_send_ack(sock, &ecode);
            break;
        case OP_NS_DELETE:
            ecode = sserver_by_path(msg->file, &reply_addr.addr);
            sock_send_ack(sock, &ecode);
            printf("[CLIENT %d] Deleted path '%s'\n", sock, msg->file);
            break;
        case OP_NS_COPY:
            /* TODO: Split into two */
            ecode = sserver_by_path(msg->file, &reply_addr.addr);
            sock_send_ack(sock, &ecode);
            printf("[CLIENT %d] Copied paths '%s'\n", sock, msg->file);
            break;
        case OP_NS_GET_SS:
            ecode = sserver_by_path(msg->file, &reply_addr.addr);
            sock_send_ack(sock, &ecode);
            if (ecode == ERR_NONE)
            {
                sock_send(sock, (Message *)&reply_addr);
            }
            printf("[CLIENT %d] Requested SS for path '%s'\n", sock, msg->file);
            break;
        case OP_NS_LS:
            sock_send_ack(sock, &ecode);
            printf("[CLIENT %d] Listing all files at path '%s'\n", sock, msg->file);
            break;
        default:
            /* Invalid OP at this case */
            ecode = ERR_REQ;
            sock_send_ack(sock, &ecode);
        }
        free(msg);
    }

    printf("[CLIENT %d] Disconnected\n", sock);
    close(sock);
    return NULL;
}

void *handle_ss(void *sserver_void)
{
    SServerInfo *sserver = (SServerInfo *)sserver_void;
    /* Get available paths */
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(sserver->sock_fd);
        if (msg->op != OP_NS_INIT_FILE || !(msg->file[0]))
        {
            free(msg);
            break;
        }
        printf("[STORAGE SERVER %d] Has file '%s'\n", sserver->sock_fd, msg->file);
        free(msg);
    }
    printf("[STORAGE SERVER %d] Finished exposing all files, now ready to process requests\n", sserver->sock_fd);
    while (1)
    {
        Message *msg = sock_get(sserver->sock_fd);
        if (!msg)
        {
            break;
        }
        free(msg);
    }
    printf("[STORAGE SERVER %d] Disconnected\n", sserver->sock_fd);
    pthread_mutex_lock(&sservers_lock);
    close(sserver->sock_fd);
    sserver->is_used = 0;
    pthread_mutex_unlock(&sservers_lock);
    return NULL;
}

int main(int argc, char *argv[])
{
    char *port = NS_DEFAULT_PORT;
    if (argc == 2)
    {
        port = argv[1];
    }
    else if (argc > 2)
    {
        fprintf(stderr, "[SELF] Expected at most 1 argument, got %d\n", argc - 1);
        return 1;
    }

    int server_fd = sock_init(port);
    if (server_fd >= 0)
    {
        while (1)
        {
            struct sockaddr_in sock_addr, ss_sock_addr = {0};
            int conn_fd = sock_accept(server_fd, &sock_addr, &ss_sock_addr);
            if (conn_fd < 0)
            {
                continue;
            }
            pthread_t thread_id;
            if (ss_sock_addr.sin_port)
            {
                SServerInfo *sserver = sserver_register(&sock_addr, conn_fd);
                if (!sserver)
                {
                    close(conn_fd);
                    fprintf(stderr, "[SELF] Exceeded limit on storage servers!\n");
                    continue;
                }
                if (pthread_create(&thread_id, NULL, handle_ss, (void *)sserver) != 0)
                {
                    perror("[SELF] Thread creation failed");
                    continue;
                }
            }
            else
            {
                if (pthread_create(&thread_id, NULL, handle_client, (void *)(intptr_t)conn_fd) != 0)
                {
                    perror("[SELF] Thread creation failed");
                    continue;
                }
            }

            pthread_detach(thread_id);
        }
    }
    close(server_fd);
    return 0;
}
