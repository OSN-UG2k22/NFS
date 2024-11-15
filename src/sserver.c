#include "common.h"

char *storage_path = NULL;

/* Returns 1 on success, 0 on error */
int _sserver_send_files(int sock, char *parent, MessageFile *cur)
{
    struct dirent *entry;
    char *actual_path = path_concat(parent, cur->file);
    if (!actual_path)
    {
        return 0;
    }

    DIR *dir = opendir(actual_path);
    if (!dir)
    {
        free(actual_path);
        switch (errno)
        {
        case EACCES:
        case ENOENT:
            /* Ignore permission error, don't handle path */
            return 1;
        case ENOTDIR:
            /* Send file */
            cur->op = OP_NS_INIT_FILE;
            printf("[SELF] Exposing file '%s'\n", cur->file);
            return sock_send(sock, (Message *)cur);
        default:
            perror("[SELF] open dir failed");
            return 0;
        }
    }
    free(actual_path);

    int ret = 1;
    while ((entry = readdir(dir)) != NULL)
    {
        MessageFile new;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        int len = snprintf(new.file, sizeof(new.file), "%s/%s", cur->file, entry->d_name);
        if (len <= 0 || len >= (int)sizeof(new.file))
        {
            continue;
        }

        if (!_sserver_send_files(sock, parent, &new))
        {
            ret = 0;
            break;
        }
    }
    closedir(dir);
    return ret;
}

/* Returns 1 on success, 0 on error */
int sserver_send_files(int sock, char *path)
{
    MessageFile cur = {0};
    int ret = _sserver_send_files(sock, path, &cur);

    /* Send empty file to signal end */
    cur.op = OP_NS_INIT_FILE;
    cur.file[0] = '\0';
    ret &= sock_send(sock, (Message *)&cur);
    if (ret)
    {
        printf("[SELF] Finsihed exposing all files, now ready to process requests\n");
    }
    else
    {
        printf("[SELF] Failed while exposing files to naming server\n");
    }
    return ret;
}

ErrCode sserver_create(char *input_file_path)
{
    char *file_path = path_concat(storage_path, input_file_path);
    for (char *p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/'))
    {
        *p = '\0';
        if (mkdir(file_path, 0755) == -1)
        {
            if (errno != EEXIST)
            {
                free(file_path);
                return ERR_SYS;
            }
        }
        *p = '/';
    }
    FILE *file = fopen(file_path, "w");
    free(file_path);
    if (!file)
    {
        return ERR_SYS;
    }

    fclose(file);
    return ERR_NONE;
}

/* Returns 1 on success, 0 on failure */
int _sserver_delete(char *file_path)
{
    struct dirent *entry;
    DIR *dir = opendir(file_path);
    if (!dir)
    {
        switch (errno)
        {
        case ENOTDIR:
            /* It is a file, delete it */
            if (unlink(file_path) != 0)
            {
                perror("[SELF] unlink failed");
                return 0;
            }
            return 1;
        default:
            perror("[SELF] open dir failed");
            return 0;
        }
    }

    int ret = 1;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char *concated_path = path_concat(file_path, entry->d_name);
        ret &= _sserver_delete(concated_path);
        free(concated_path);
    }
    closedir(dir);
    if (rmdir(file_path) != 0)
    {
        perror("[SELF] rmdir failed");
        ret = 0;
    }
    return ret;
}

ErrCode sserver_delete(char *input_file_path)
{
    char *file_path = path_concat(storage_path, input_file_path);
    ErrCode ret = _sserver_delete(file_path) ? ERR_NONE : ERR_SYS;
    free(file_path);
    return ret;
}

void *handle_client(void *fd_ptr)
{
    int sock_fd = (int)(intptr_t)fd_ptr;
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(sock_fd);
        if (!msg)
        {
            break;
        }
        char *actual_path = path_concat(storage_path, msg->file);
        ErrCode ecode = ERR_NONE;
        char *operation = "invalid operation";
        FILE *file = NULL;
        switch (msg->op)
        {
        case OP_SS_READ:
            operation = "read path";
            file = fopen(actual_path, "r");
            if (!file)
            {
                perror("[SELF] Could not open file");
                ecode = ERR_SYS;
            }
            ecode = path_sock_sendfile(sock_fd, file);
            if (file)
            {
                fclose(file);
            }
            break;
        case OP_SS_WRITE:
            operation = "write path";
            file = fopen(actual_path, "w");
            if (!file)
            {
                perror("[SELF] Could not open file");
                ecode = ERR_SYS;
            }
            MessageInt ack;
            ack.op = OP_ACK;
            ack.info = ecode;
            ecode = path_sock_getfile(sock_fd, (Message *)&ack, file);
            if (file)
            {
                fclose(file);
            }
            break;
        case OP_SS_STREAM:
            operation = "stream path";
            // sendfile(sock_fd, msg->file);
            uint16_t port = 0;

            int server_socket = sock_init(&port);
            // inform client of port and ip
            MessageInt port_msg;
            port_msg.op = OP_ACK;
            port_msg.info = port;
            printf("dEBUG1\n");
            ecode = sock_send(sock_fd, (Message *)&port_msg);
            if (!ecode)
            {
                close(server_socket);
                ecode = ERR_CONN;
                break;
            }

            // while (1) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            printf("dEBUG2\n");
            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

            if (client_socket < 0)
            {
                close(server_socket);
                ecode = ERR_CONN;
                break;
            }

            stream_file(client_socket, actual_path);
            close(client_socket);
            close(server_socket);
            break;
        case OP_SS_INFO:
            operation = "info of path";
            // use ls -l program to get file(actual_path) info
            // scrape size last modified date and name and permissions from it and send it to client
            char command[256];
            snprintf(command, sizeof(command), "ls -l \"%s\" | awk '{print $1, $5, $6, $7, $8}'", actual_path);
            file = popen(command, "r");
            if (!file)
            {
                perror("[SELF] Could not open file");
                ecode = ERR_SYS;
            }
            ecode = path_sock_sendfile(sock_fd, file);
            if (file)
            {
                pclose(file);
            }
            break;
        default:
            /* Invalid OP at this case */
            ecode = ERR_REQ;
            sock_send_ack(sock_fd, &ecode);
            break;
        }
        if (ecode == ERR_NONE)
        {
            printf("[CLIENT %d] Executed %s '%s'\n", sock_fd, operation, msg->file);
        }
        else
        {
            printf("[CLIENT %d] Failed to %s '%s': %s\n",
                   sock_fd, operation, msg->file, errcode_to_str(ecode));
        }
        free(actual_path);
        free(msg);
    }

    printf("[CLIENT %d] Disconnected\n", sock_fd);
    close(sock_fd);
    return NULL;
}

void *handle_ns(void *ns_fd_ptr)
{
    int ns_fd = (int)(intptr_t)ns_fd_ptr;
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(ns_fd);
        if (!msg)
        {
            break;
        }

        ErrCode ecode = ERR_NONE;
        char *operation = "invalid operation";
        switch (msg->op)
        {
        case OP_NS_CREATE:
            operation = "create path";
            ecode = sserver_create(msg->file);
            sock_send_ack(ns_fd, &ecode);
            break;
        case OP_NS_DELETE:
            operation = "delete path";
            ecode = sserver_delete(msg->file);
            sock_send_ack(ns_fd, &ecode);
            break;
        case OP_NS_COPY:
            operation = "copy paths";
            int dst_sock = -1;
            FILE *src_file = NULL;
            char *src_path = strchr(msg->file, ':');
            if (!src_path)
            {
                ecode = ERR_REQ;
            }
            else
            {
                *src_path = '\0';
                src_path++;
                src_path = path_concat(storage_path, src_path);
                if (src_path)
                {
                    src_file = fopen(src_path, "r");
                    free(src_path);
                    if (!src_file)
                    {
                        ecode = ERR_SYS;
                    }
                }
            }
            if (ecode == ERR_NONE)
            {
                MessageAddr *reply_addr = (MessageAddr *)sock_get(ns_fd);
                if (!reply_addr)
                {
                    ecode = ERR_CONN;
                }
                else if (reply_addr->op != OP_NS_REPLY_SS)
                {
                    ecode = ERR_SYNC;
                }
                else
                {
                    dst_sock = sock_connect_addr(&reply_addr->addr);
                    if (dst_sock < 0)
                    {
                        ecode = ERR_CONN;
                    }
                }
                free(reply_addr);
            }
            if (ecode == ERR_NONE)
            {
                msg->op = OP_SS_WRITE;
                ecode = sock_send(dst_sock, (Message *)msg) ? sock_get_ack(dst_sock) : ERR_CONN;
                if (ecode == ERR_NONE)
                {
                    ecode = path_sock_sendfile(dst_sock, src_file);
                }
                fclose(src_file);
                close(dst_sock);
            }

            sock_send_ack(ns_fd, &ecode);
            break;
        default:
            /* Invalid OP at this case */
            ecode = ERR_REQ;
            sock_send_ack(ns_fd, &ecode);
        }
        if (ecode == ERR_NONE)
        {
            printf("[NAMING SERVER] Executed %s '%s'\n", operation, msg->file);
        }
        else
        {
            printf("[NAMING SERVER] Failed to %s '%s': %s\n",
                   operation, msg->file, errcode_to_str(ecode));
        }
        free(msg);
    }

    printf("[NAMING SERVER] Disconnected\n");
    close(ns_fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    char *nserver_host = DEFAULT_HOST;
    uint16_t nserver_port = NS_DEFAULT_PORT;
    PortAndID pd;
    pd.port = 0;
    pd.id = -1;
    switch (argc)
    {
    case 4:
        nserver_port = (uint16_t)atoi(argv[3]);
        /* Fall through */
    case 3:
        nserver_host = argv[2];
        /* Fall through */
    case 2:
        storage_path = argv[1];
        break;
    default:
        fprintf(
            stderr,
            "usage: ./sserver <storage_path> [nserver_host] [nserver_port]\n");
        return 1;
    }

    int sserver_fd = sock_init(&pd.port);
    if (sserver_fd < 0)
    {
        return 1;
    }
    char *metadata_file = path_concat(storage_path, SS_METADATA);
    if (metadata_file)
    {
        /* Open a file for a single read, and create it if it does not exist */
        FILE *f = fopen(metadata_file, "a+");
        if (!f)
        {
            perror("[SELF] Could not access storage path");
            free(metadata_file);
            goto end;
        }
        fseek(f, 0, SEEK_SET);
        fscanf(f, "%hd", &pd.id);
        fclose(f);
    }

    int nserver_fd = sock_connect(nserver_host, &nserver_port, &pd);
    if (nserver_fd < 0)
    {
        free(metadata_file);
        goto end;
    }

    if (metadata_file)
    {
        FILE *f = fopen(metadata_file, "w");
        free(metadata_file);
        if (!f)
        {
            perror("[SELF] Could not access storage path");
            goto end;
        }
        fprintf(f, "%hd\n", pd.id);
        fclose(f);
    }

    printf("[SELF] Started storage server with id %hd on %s\n", pd.id, storage_path);
    if (!sserver_send_files(nserver_fd, storage_path))
    {
        goto end;
    }

    pthread_t ns_thread_id;
    if (pthread_create(&ns_thread_id, NULL, handle_ns, (void *)(intptr_t)nserver_fd) != 0)
    {
        perror("[SELF] Thread creation failed");
        goto end;
    }

    while (1)
    {
        struct sockaddr_in sock_addr;
        int conn_fd = sock_accept(sserver_fd, &sock_addr, NULL);
        if (conn_fd < 0)
        {
            continue;
        }
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)(intptr_t)conn_fd) != 0)
        {
            perror("[SELF] Thread creation failed");
            continue;
        }
        pthread_detach(thread_id);
    }

end:
    close(sserver_fd);
    close(nserver_fd);
    return 0;
}
