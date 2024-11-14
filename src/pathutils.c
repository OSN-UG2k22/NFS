#include "common.h"

/* Returns a path with prefix removed if prefix matches, else returns NULL.
 * The returned buffer is simply a pointer to memory within 'self' argument */
char *path_remove_prefix(char *self, char *op)
{
    if (!self || !op)
    {
        return self;
    }

    ssize_t op_len = strlen(op);
    if (op[op_len - 1] == '/')
    {
        op_len--;
    }
    if (!op_len)
    {
        /* "/" is a prefix for everything */
        return self;
    }

    if (!strncmp(self, op, op_len))
    {
        /* found match */
        char *ret = self + op_len;
        if (ret[0] != '/' && ret[0])
        {
            /* not a perfect match */
            return NULL;
        }
        return ret;
    }
    return NULL;
}

/* Returns a char buffer that should be freed by caller, or NULL on failure */
char *path_concat(char *first, char *second)
{
    ssize_t first_len = strlen(first);
    ssize_t second_len = strlen(second);
    while (first_len && first[first_len - 1] == '/')
    {
        /* Ignore trailing slashes */
        first_len--;
    }

    while (second[0] == '/')
    {
        /* Ignore leading slashes */
        second++;
        second_len--;
    }

    /* +2 is for / and trailing NULL */
    char *ret = malloc(first_len + second_len + 2);
    if (ret)
    {
        memcpy(ret, first, first_len);
        if (second_len)
        {
            ret[first_len] = '/';
            memcpy(ret + first_len + 1, second, second_len);
            ret[first_len + second_len + 1] = '\0';
        }
        else
        {
            /* Avoid trailing slash */
            ret[first_len] = '\0';
        }
    }
    return ret;
}

/* Returns a char buffer that should be freed by caller, or NULL on failure */
ErrCode path_sock_sendfile(int sock, FILE *infile)
{
    ErrCode ret = infile ? ERR_NONE : ERR_SYS;
    MessageChunk chunk;
    chunk.op = OP_RAW;
    while (ret == ERR_NONE)
    {
        int read_bytes = fread(chunk.chunk, 1, sizeof(chunk.chunk), infile);
        if (!read_bytes)
        {
            ret = ferror(infile) ? ERR_SYS : ERR_NONE;
            clearerr(infile);
            break;
        }
        chunk.size = read_bytes;
        if (!sock_send(sock, (Message *)&chunk))
        {
            ret = ERR_CONN;
            goto end;
        }
        ret = sock_get_ack(sock);
    }
end:
    sock_send_ack(sock, &ret);
    return ret;
}

ErrCode path_sock_getfile(int sock, Message *msg_header, FILE *outfile)
{
    ErrCode ret = outfile ? ERR_NONE : ERR_SYS;
    if (!sock_send(sock, msg_header))
    {
        ret = ERR_CONN;
    }
    while (ret == ERR_NONE)
    {
        Message *read_data = sock_get(sock);
        if (!read_data)
        {
            ret = ERR_CONN;
            break;
        }
        if (read_data->op != OP_RAW)
        {
            ret = (read_data->op == OP_ACK) ? ((MessageInt *)read_data)->info : ERR_SYNC;
            free(read_data);
            break;
        }
        MessageChunk *read_chunk = (MessageChunk *)read_data;
        fwrite(read_chunk->chunk, 1, read_chunk->size, outfile);
        free(read_data);
        sock_send_ack(sock, &ret);
    }
    return ret;
}
