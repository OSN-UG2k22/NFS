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
        printf("- READ [path]:[optional local file to write contents to]\n");
        printf("- WRITE [path]:[optional local file to read contents from]\n");
        printf("- STREAM [path]\n");
        printf("- INFO [path]\n");
        printf("- COPY [destination path]:[source path]\n");
        printf("- LS [path]\n");
        printf("- CREATE [path]\n");
        printf("- DELETE [path]\n");
        while (1)
        {
            ErrCode ret = ERR_NONE;
            MessageFile request;
            char op[16];
            printf("> ");
            if (scanf("%15s %[^\n]", op, request.file) != 2)
            {
                if (feof(stdin) || ferror(stdin))
                {
                    break;
                }
                ret = ERR_REQ;
                goto end_loop;
            }

            char *arg2 = NULL;
            if (strcasecmp(op, "READ") == 0 || strcasecmp(op, "WRITE") == 0)
            {
                arg2 = strchr(request.file, ':');
                if (arg2)
                {
                    arg2[0] = '\0';
                    arg2++;
                    if (!arg2[0])
                    {
                        arg2 = NULL;
                    }
                }
            }

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

            ret = sock_send(sock_fd, (Message *)&request) ? ERR_NONE : ERR_CONN;
            if (ret != ERR_NONE)
            {
                goto end_loop;
            }
            if (request.op == OP_NS_LS)
            {
                // write logic here
                continue;
            }
            ret = sock_get_ack(sock_fd);
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

            if (!ss_addr)
            {
                /* For create/delete/copy, we are done here */
                goto end_loop;
            }

            if (strcasecmp(op, "READ") == 0)
            {
                FILE *outfile = arg2 ? fopen(arg2, "w") : stdout;
                if (!outfile)
                {
                    perror("[SELF] Could not open local file");
                }
                else
                {
                    int sock_server = sock_connect_addr(&ss_addr->addr);
                    request.op = OP_SS_READ;
                    ret = path_sock_getfile(sock_server, (Message *)&request, outfile);
                    if (outfile != stdout)
                    {
                        fclose(outfile);
                    }
                    close(sock_server);
                }
            }
            else if (strcasecmp(op, "WRITE") == 0)
            {
                FILE *infile = arg2 ? fopen(arg2, "r") : stdin;
                if (!infile)
                {
                    perror("[SELF] Could not open local file");
                }
                else
                {
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
                        ret = path_sock_sendfile(sock_server, infile);
                    }
                    if (infile != stdin)
                    {
                        fclose(infile);
                    }
                    close(sock_server);
                }
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
                MessageInt *port_msg = (MessageInt *)sock_get(sock_server);
                if (port_msg->op != OP_ACK)
                {
                    // error
                }
                sleep(1);
                stream_music(ip, port_msg->info);
                close(sock_server);
            }
            else if (strcasecmp(op, "INFO") == 0)
            {
                FILE *outfile = arg2 ? fopen(arg2, "w") : stdout;
                if (!outfile)
                {
                    perror("[SELF] Could not open local file");
                }
                else
                {
                    int sock_server = sock_connect_addr(&ss_addr->addr);
                    request.op = OP_SS_INFO;
                    printf("%s ", request.file);
                    ret = path_sock_getfile(sock_server, (Message *)&request, outfile);
                    if (outfile != stdout)
                    {
                        fclose(outfile);
                    }
                    close(sock_server);
                }
            }
            else
            {
                ret = ERR_REQ;
            }
            free(ss_addr);
        end_loop:
            if (ret == ERR_NONE)
            {
                printf("[SELF] Operation succeeded\n");
            }
            else
            {
                printf("[SELF] Operation failed: %s\n", errcode_to_str(ret));
                continue;
            }
        }
    }

    close(sock_fd);
    return 0;
}
