#include "common.h"

int main(int argc, char *argv[])
{
    char *nserver_host = DEFAULT_HOST;
    uint16_t nserver_port = NS_DEFAULT_PORT;
    switch (argc)
    {
    case 1:
        break;
    case 3:
        nserver_port = (uint16_t)atoi(argv[2]);
        /* Fall through */
    case 2:
        nserver_host = argv[1];
        break;
    default:
        fprintf(stderr, "usage: ./client [nserver_host] [nserver_port]\n");
        return 1;
    }

    int sock_fd = sock_connect(nserver_host, &nserver_port, NULL);
    if (sock_fd >= 0)
    {
        printf("List of supported commands:\n");
        printf("- READ [path]\n");
        printf("- WRITE [path]:[local file to read contents from]\n");
        printf("- STREAM [path]\n");
        printf("- INFO [path]\n");
        printf("- COPY [source path]:[destination path]\n");
        printf("- LS [path]\n");
        printf("- CREATE [path]\n");
        printf("- DELETE [path]\n");
        while (1)
        {
            char op[16];
            printf("> ");
            if (scanf("%15s ", op) != 1)
            {
                break;
            }

            MessageFile request;
            // if (!fgets(request.file, sizeof(request.file), stdin))
            // {
            //     break;
            // }
            /* Strip newline */
            // size_t arg_len = strlen(request.file);
            // while (arg_len && request.file[arg_len - 1] == '\n')
            // {
            //     arg_len--;
            // }
            // request.file[arg_len] = '\0';
            char arg2[FILENAME_MAX_LEN];
            if (strcmp(op, "WRITE") != 0)
            {
                if (scanf("%[^\n]", request.file) != 1)
                {
                    // error
                }
            }
            else
            {
                if (scanf("%[^:]:%[^\n]", request.file, arg2) != 2)
                {
                    // error
                }
            }
            // error handling if filename exceeds ?

            if (strcasecmp(op, "COPY") == 0)
            {
                request.op = OP_NS_COPY;
            }
            else if (strcasecmp(op, "LS") == 0)
            {
                request.op = OP_NS_LS;
            }
            else if (strcasecmp(op, "CREATE") == 0)
            {
                request.op = OP_NS_CREATE;
            }
            else if (strcasecmp(op, "DELETE") == 0)
            {
                request.op = OP_NS_DELETE;
            }
            else
            {
                request.op = OP_NS_GET_SS;
            }

            sock_send(sock_fd, (Message *)&request);
            if (request.op == OP_NS_LS)
            {
                // write logic here
                continue;
            }
            ErrCode ret = sock_get_ack(sock_fd);
            MessageAddr *ss_addr = NULL;
            if (ret == ERR_NONE && request.op == OP_NS_GET_SS)
            {
                ss_addr = (MessageAddr *)sock_get(sock_fd);
                if (ss_addr)
                {
                    if (ss_addr->op != OP_NS_REPLY_SS)
                    {
                        ret = ERR_SYNC;
                        free(ss_addr);
                        ss_addr = NULL;
                    }
                }
                else
                {
                    ret = ERR_CONN;
                }
            }
            if (ret == ERR_NONE)
            {
                printf("[SELF] Operation succeeded\n");
            }
            else
            {
                printf("[SELF] Operation failed: %s\n", errcode_to_str(ret));
                continue;
            }

            if (ss_addr)
            {
                printf("[SELF] Received Storage Server address: ");
                ipv4_print_addr(&ss_addr->addr, NULL);
            }
            else
            {
                /* For create/delete/copy, we are done here */
                continue;
            }

            /* TODO: integrate below code later */
            // #if 0
            // char ip[INET_ADDRSTRLEN + 1];
            // // char port [6];
            // uint16_t port;
            // inet_ntop(AF_INET, &ss_addr->addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
            // // sprintf(port,"%d",ntohs(ss_addr->addr.sin_port));
            // port = ntohs(ss_addr->addr.sin_port);
            // int sock_server = sock_connect(ip, &port, NULL);
            // if (sock_server < 0)
            // {
            //     // error
            // }
            // port = ntohs(ss_addr->addr.sin_port);
            if (strcasecmp(op, "READ") == 0)
            {
                ErrCode ret = ERR_NONE;
                int sock_server = sock_connect_addr(&ss_addr->addr);
                request.op = OP_SS_READ;
                path_sock_getfile(sock_server, (Message *)&request, stdout);
                close(sock_server);
            }
            else if (strcasecmp(op, "WRITE") == 0)
            {
                ErrCode ret = ERR_NONE;
                int sock_server = sock_connect_addr(&ss_addr->addr);
                request.op = OP_SS_WRITE;
                if (!sock_send(sock_server, (Message *)&request))
                {
                    ret = ERR_CONN;
                }
                if (ret == ERR_NONE)
                {
                    ret = sock_get_ack(sock_server);
                }
                if (ret == ERR_NONE)
                {
                    ret = path_sock_sendfile(sock_server, arg2);
                }
                close(sock_server);
            }
            else if (strcasecmp(op, "STREAM") == 0)
            {
                char ip[INET_ADDRSTRLEN + 1];
                uint16_t port;
                inet_ntop(AF_INET, &ss_addr->addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
                port = ntohs(ss_addr->addr.sin_port);
                int sock_server = sock_connect(ip, &port, NULL);

                request.op = OP_SS_STREAM;
                sock_send(sock_server, (Message *)&request);
                stream_music(ip, port);
                // close(sock_server);
            }
            else if (strcasecmp(op, "INFO") == 0)
            {
                char ip[INET_ADDRSTRLEN + 1];
                uint16_t port;
                inet_ntop(AF_INET, &ss_addr->addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
                port = ntohs(ss_addr->addr.sin_port);
                int sock_server = sock_connect(ip, &port, NULL);

                sock_send(sock_server, (Message *)&request);
                // If need be we can implement acks
                MessageFile *datasize = (MessageFile *)sock_get(sock_server);
                if (datasize->op != OP_RAW)
                {
                    // error
                }
                int numchunks = datasize->size / (2 * FILENAME_MAX_LEN);
                int recv_chunks = 0;
                MessageFile *read_data;
                // while (recv_chunks < numchunks && (read_data = (MessageFile*) sock_get(sock_server))->op == OP_SS_READ && strcmp(read_data->file,"STOP") != 0)
                while (recv_chunks <= numchunks && strcmp((read_data = (MessageFile *)sock_get(sock_server))->file, "STOP") != 0)
                {
                    if (read_data->op != OP_SS_INFO)
                    {
                        // error;
                    }
                    recv_chunks++;
                    if (datasize->size > 2 * FILENAME_MAX_LEN)
                    {
                        datasize->size -= 2 * FILENAME_MAX_LEN;
                        printf("%s", read_data->file);
                    }
                    else
                    {
                        printf("%.*s\n", datasize->size, read_data->file);
                    }
                }
                close(sock_server);
            }
            else
            {
                printf("Invalid Operation!\n");
            }
            // #endif
        }
    }

    close(sock_fd);
    return 0;
}
