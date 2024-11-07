#include "common.h"

int main(int argc, char *argv[])
{
    char *storage_path = NULL;
    char *nserver_host = DEFAULT_HOST;
    char *nserver_port = NS_DEFAULT_PORT;
    char *sserver_port = SS_DEFAULT_PORT;
    switch (argc)
    {
    case 5:
        nserver_port = argv[4];
        /* Fall through */
    case 4:
        nserver_host = argv[3];
        /* Fall through */
    case 3:
        sserver_port = argv[2];
        /* Fall through */
    case 2:
        storage_path = argv[1];
        break;
    default:
        fprintf(
            stderr,
            "usage: ./sserver <storage_path> [sserver_port] [nserver_host] [nserver_port]\n");
        return 1;
    }

    int sserver_fd = sock_init(sserver_port);
    if (sserver_fd < 0)
    {
        return 1;
    }

    printf("[SERVER] Started storage server on %s\n", storage_path);
    int nserver_fd = sock_connect(nserver_host, nserver_port, sserver_port);
    if (nserver_fd >= 0)
    {
        while (1)
        {
            struct sockaddr_in sock_addr, ss_sock_addr = {0};
            int conn_fd = sock_accept(sserver_fd, &sock_addr, &ss_sock_addr);
            if (conn_fd < 0)
            {
                break;
            }
        }
    }
    close(sserver_fd);
    close(nserver_fd);
    return 0;
}
