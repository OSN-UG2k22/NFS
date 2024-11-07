#include "common.h"

int main(int argc, char *argv[])
{
    char *nserver_host = DEFAULT_HOST;
    char *nserver_port = NS_DEFAULT_PORT;
    switch (argc)
    {
    case 1:
        break;
    case 3:
        nserver_port = argv[2];
        /* Fall through */
    case 2:
        nserver_host = argv[1];
        break;
    default:
        fprintf(stderr, "usage: ./client [nserver_host] [nserver_port]\n");
        return 1;
    }

    int sock_fd = sock_connect(nserver_host, nserver_port, NULL);
    if (sock_fd >= 0)
    {
        printf("List of supported commands:\n");
        printf("- READ [path]\n");
        printf("- WRITE [path] [local file to read contents from]\n");
        printf("- STREAM [path]\n");
        printf("- INFO [path]\n");
        printf("- COPY [source path] [destination path]\n");
        printf("- LS\n");
        printf("- CREATE [path]\n");
        printf("- DELETE [path]\n");
        // MessageFile NameLater;
        while (1)
        {
            char op[16];
            if (scanf("%15s", op) != 1)
            {
                return 1;
            }

            char *input = (char *)malloc(sizeof(char) * 256);
            if (scanf("%256s", input) != 1)
            {
                return 1;
            }

            MessageFile path;
            path.op = OP_NS_GET_SS;
            strcpy(path.file, input);
            path.size = strlen(input);

            sock_send(sock_fd, (Message *)&path);
            MessageAddr *reply = (MessageAddr *)sock_get(sock_fd);
            struct sockaddr_in *ss_addr;
            if (reply->op == OP_NS_REPLY_SS)
            {
                ss_addr = (struct sockaddr_in *)&reply->addr;
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(ss_addr->sin_addr), ip, INET_ADDRSTRLEN);
                char port_str[6];
                sprintf(port_str, "%d", ntohs(ss_addr->sin_port));
                int sock_server = sock_connect(ip, port_str, NULL);

                if (strcasecmp(op, "READ") == 0)
                {
                    // char* chunk = (char*) malloc(sizeof(char)*4096);
                    Message *size_indic = sock_get(sock_server);
                    if (size_indic->op != OP_RAW)
                    {
                        // size_indic = sock_get(sock_server);
                        // error
                    }
                    int numchunks = size_indic->size / 1024;
                    int recv_chunks = 0;
                    MessageFile *read_data;
                    // while (recv_chunks < numchunks && (read_data = (MessageFile*) sock_get(sock_server))->op == OP_SS_READ && strcmp(read_data->file,"STOP") != 0)
                    while (recv_chunks < numchunks && strcmp((read_data = (MessageFile *)sock_get(sock_server))->file, "STOP") != 0)
                    {
                        if (read_data->op != OP_SS_READ)
                        {
                            // error;
                        }
                        recv_chunks++;
                        if (size_indic->size > 1024)
                        {
                            size_indic->size -= 1024;
                            printf("%s", read_data->file);
                        }
                    }
                }
                if (strcasecmp(op, "WRITE") == 0)
                {
                }
                if (strcasecmp(op, "STREAM") == 0)
                {
                }
                if (strcasecmp(op, "INFO") == 0)
                {
                }
                if (strcasecmp(op, "COPY") == 0)
                {
                }
                if (strcasecmp(op, "LS") == 0)
                {
                }
                if (strcasecmp(op, "CREATE") == 0)
                {
                }
                if (strcasecmp(op, "DELETE") == 0)
                {
                }
                else
                {
                    printf("Invalid Operation!\n");
                }
            }
        }
    }

    close(sock_fd);
    return 0;
}
