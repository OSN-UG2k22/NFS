#include "common.h"
#define CHUNK_SIZE 2 * FILENAME_MAX_LEN

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

void sendfile(int sock, const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        perror("[SELF] Could not open file");
        return;
    }

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    rewind(file);

    MessageFile data;
    data.op = OP_RAW;
    data.size = file_size;
    sock_send(sock, (Message *)&data);

    char buffer[CHUNK_SIZE];
    int read_bytes;
    while ((read_bytes = fread(buffer, 1, CHUNK_SIZE, file)) > 0)
    {
        MessageFile chunk;
        chunk.op = OP_SS_WRITE;
        chunk.size = read_bytes;
        memcpy(chunk.file, buffer, read_bytes);
        sock_send(sock, (Message *)&chunk);
    }
    fclose(file);
}

int main(int argc, char *argv[])
{
    char *storage_path = NULL;
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

    while (1)
    {
        struct sockaddr_in sock_addr;
        int conn_fd = sock_accept(sserver_fd, &sock_addr, NULL);
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
