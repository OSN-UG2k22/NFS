#include "common.h"

/* Returns 1 on success, 0 on error */
int _sserver_send_files(int sock, MessageFile *cur)
{
    struct dirent *entry;
    DIR *dir = opendir(cur->file);
    if (!dir)
    {
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

        if (!_sserver_send_files(sock, &new))
        {
            ret = 0;
            break;
        }
    }
    closedir(dir);
    return ret;
}

/* Returns 1 on success, 0 on error */
int sserver_send_files(int sock, const char *path)
{
    MessageFile cur;
    strcpy(cur.file, path);
    int ret = _sserver_send_files(sock, &cur);

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

int main(int argc, char *argv[])
{
    char *storage_path = NULL;
    char *nserver_host = DEFAULT_HOST;
    char *nserver_port = NS_DEFAULT_PORT;
    char sserver_port[6] = "0";
    switch (argc)
    {
    case 4:
        nserver_port = argv[3];
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

    int sserver_fd = sock_init(sserver_port);
    if (sserver_fd < 0)
    {
        return 1;
    }

    printf("[SELF] Started storage server on %s\n", storage_path);
    int nserver_fd = sock_connect(nserver_host, nserver_port, sserver_port);
    if (nserver_fd < 0)
    {
        goto end;
    }

    if (!sserver_send_files(nserver_fd, storage_path))
    {
        goto end;
    }

    while (1)
    {
        struct sockaddr_in sock_addr, ss_sock_addr = {0};
        int conn_fd = sock_accept(sserver_fd, &sock_addr, &ss_sock_addr);
        if (conn_fd < 0)
        {
            break;
        }
    }

end:
    close(sserver_fd);
    close(nserver_fd);
    return 0;
}
