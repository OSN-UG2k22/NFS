#include "common.h"
#define CHUNK_SIZE 2 * FILENAME_MAX_LEN

char *storage_path = NULL;

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
                *p = '/';
                return ERR_SYS;
            }
        }
        *p = '/';
    }
    FILE *file = fopen(file_path, "w");
    if (!file)
    {
        return ERR_SYS;
    }

    free(file_path);
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
    printf("holalal");

    char buffer[CHUNK_SIZE];
    int read_bytes;
    while ((read_bytes = fread(buffer, 1, CHUNK_SIZE, file)) > 0)
    {
        MessageFile chunk;
        chunk.op = OP_RAW;
        chunk.size = read_bytes;
        memcpy(chunk.file, buffer, read_bytes);
        sock_send(sock, (Message *)&chunk);
    }
    fclose(file);
}

void writefile(int sock, const char *path, int numchunks)
{
    // We come here after checking if the file is availabe to write
    FILE *file = fopen(path, "w");
    MessageFile *fdata;
    int recv_chunks = 0;
    int num_bytes;
    while (recv_chunks <= numchunks && (fdata = (MessageFile *)sock_get(sock)) != 0)
    {
        recv_chunks++;
        fwrite(fdata->file, sizeof(char), fdata->size, file);
    }
}

void *handle_client(void *fd_ptr)
{
    int sock_fd = (int)(intptr_t)fd_ptr;
    printf("sserver debug2\n");
    printf("[CLIENT %d] Connected to handle requests now\n", sock_fd);
    while (1)
    {printf("sserver debug3\n");
        MessageFile *msg = (MessageFile *)sock_get(sock_fd);
        printf("sserver debug4\n");
        if (!msg)
        {
            break;
        }

        ErrCode ecode = ERR_NONE;
        switch (msg->op)
        {
            case OP_SS_READ:
            {
                printf("[SELF] Reading file '%s'\n", msg->file);
                sendfile(sock_fd, msg->file);
            }
        default:
            /* Invalid OP at this case */
            ecode = ERR_REQ;
            sock_send_ack(sock_fd, &ecode);
        }
        free(msg);
    }

    printf("[CLIENT %d] Disconnected\n", sock_fd);
    close(sock_fd);
    return NULL;
}

void *handle_ns(void *ns_fd_ptr)
{
    int ns_fd = (int)(intptr_t)ns_fd_ptr;
    printf("[NAMING SERVER] Connected to handle requests now\n");
    while (1)
    {
        MessageFile *msg = (MessageFile *)sock_get(ns_fd);
        if (!msg)
        {
            break;
        }

        ErrCode ecode = ERR_NONE;
        int sserver_fd;
        switch (msg->op)
        {
        case OP_NS_CREATE:
            ecode = sserver_create(msg->file);
            sock_send_ack(ns_fd, &ecode);
            if (ecode == ERR_NONE)
            {
                printf("[SELF] Created path '%s'\n", msg->file);
            }
            else
            {
                printf("[SELF] Failed to create path '%s'\n", msg->file);
            }
            break;
        case OP_NS_DELETE:
            ecode = sserver_delete(msg->file);
            sock_send_ack(ns_fd, &ecode);
            if (ecode == ERR_NONE)
            {
                printf("[SELF] Deleted path '%s'\n", msg->file);
            }
            else
            {
                printf("[SELF] Failed to delete path '%s'\n", msg->file);
            }
            break;
        default:
            /* Invalid OP at this case */
            ecode = ERR_REQ;
            sock_send_ack(ns_fd, &ecode);
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
    {printf("sserver debug\n");
        struct sockaddr_in sock_addr;
        int conn_fd = sock_accept(sserver_fd, &sock_addr, NULL);
        printf("sserver debug1\n");
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
