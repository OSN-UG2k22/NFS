#include "common.h"
#define CHUNK_SIZE 1024

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
        printf("- LS [path]\n");
        printf("- LA .\n"); // list all paths available
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
            
            // printf("Enter the path: ");
            char *input = (char *)malloc(sizeof(char) * 256);
            if (scanf("%255s\n", input) != 1)
            {
                return 1;
            }
            if (strcasecmp(op, "LA") == 0)
            {
                
            }

            MessageFile path;
            path.op = OP_NS_GET_SS;
            strcpy(path.file, input);
            path.size = strlen(input);

            sock_send(sock_fd, (Message *)&path);
            MessageAddr *reply = (MessageAddr *)sock_get(sock_fd);
            struct sockaddr_in *ss_addr;
            if (reply->op != OP_NS_REPLY_SS)
            {
                // error
            }
            ss_addr = (struct sockaddr_in *)&reply->addr;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ss_addr->sin_addr), ip, INET_ADDRSTRLEN);
            char port_str[6];
            sprintf(port_str, "%d", ntohs(ss_addr->sin_port));
            int sock_server = sock_connect(ip, port_str, NULL);
            int port = ntohs(ss_addr->sin_port);

            if (strcasecmp(op, "READ") == 0)
            {
                MessageFile request;
                request.op = OP_SS_READ;
                strcpy(request.file, input);
                request.size = strlen(input);
                sock_send(sock_server, (Message *)&request);

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
                    else
                    {
                        printf("%.*s", size_indic->size, read_data->file);
                    }
                }
            }
            if (strcasecmp(op, "WRITE") == 0)
            {
                char filepath[256];
                if (scanf("%255s\n", filepath) != 1)
                {
                    return 1;
                }
                FILE *file = fopen(filepath, "r");
                if (file == NULL)
                {
                    // error
                }
                MessageFile request;
                request.op = OP_SS_WRITE;
                strcpy(request.file, input);
                request.size = strlen(input);
                sock_send(sock_server, (Message *)&request);

                // If someone else is writing to the file, you cant write so wait till server sends an ACK
                Message *ack = sock_get(sock_server);
                if (ack->op != OP_ACK)
                {
                    // error
                }
                // read from file and send 1024 bytes at a time
                // find the size of the file preemptively and send it first
                fseek(file, 0, SEEK_END);
                int file_size = ftell(file);
                rewind(file);
                MessageFile size_indic;
                size_indic.op = OP_RAW;
                size_indic.size = file_size;
                sock_send(sock_server, (Message *)&size_indic);

                char buffer[CHUNK_SIZE];
                int read_bytes;
                while ((read_bytes = fread(buffer, 1, CHUNK_SIZE, file)) > 0)
                {
                    MessageFile chunk;
                    chunk.op = OP_SS_WRITE;
                    chunk.size = read_bytes;
                    memcpy(chunk.file, buffer, read_bytes);
                    sock_send(sock_server, (Message *)&chunk);
                }
            }
            if (strcasecmp(op, "STREAM") == 0)
            {
                MessageFile request;
                request.op = OP_SS_STREAM;
                strcpy(request.file, input);
                request.size = strlen(input);
                sock_send(sock_server, (Message *)&request);
                stream_music(ip, port);
                
            }
            if (strcasecmp(op, "INFO") == 0)
            {
                MessageFile request;
                request.op = OP_SS_INFO;
                strcpy(request.file, input);
                request.size = strlen(input);
                sock_send(sock_server, (Message *)&request);

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
                    if (read_data->op != OP_SS_INFO)
                    {
                        // error;
                    }
                    recv_chunks++;
                    if (size_indic->size > 1024)
                    {
                        size_indic->size -= 1024;
                        printf("%s", read_data->file);
                    }
                    else
                    {
                        printf("%.*s", size_indic->size, read_data->file);
                    }
                }
            }
            if (strcasecmp(op, "COPY") == 0)
            {
                // Format - COPY Destination Source
                char filepath[256];
                scanf("%255s\n", filepath);
                MessageFile2 request;
                request.op = OP_NS_COPY;
                strcpy(request.file, input);
                strcpy(request.file2, filepath);
                request.size = strlen(input) + strlen(filepath);
                sock_send(sock_fd, (Message *)&request);
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

    close(sock_fd);
    return 0;
}
