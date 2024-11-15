#include "wrapper_opt.h"

trienode *__global_trie = NULL;
LRU_Cache *__global_lru = NULL;

char *handle_slash(char *str)
{
    int len = (int)strlen(str);
    char *newstr = malloc((len + 3) * sizeof(char));
    int i = 0; // for str
    int j = 0; // for newstr
    if (str[i] != '/')
    {
        newstr[j++] = '/';
    }
    for (; i < len - 1; i++)
    {
        newstr[j++] = str[i];
    }
    if (str[len - 1] == '/') // ignore trailing '/'
    {
        newstr[j++] = '\0';
    }
    else
    {
        newstr[j++] = str[len - 1];
    }
    newstr[j] = '\0';
    return newstr;
}

int create(int main_server, char *str) // takes main server and string path inserts in trie updates lru
{
    char *newstr = handle_slash(str);
    if (__global_trie == NULL)
    {
        initialize_trie(&__global_trie);
    }
    int x = trieinsert(__global_trie, newstr, main_server);
    if (x == -1)
    {
        insert(__global_lru, main_server, newstr);
        return x;
    }

    if (__global_lru == NULL)
    {
        // printf("FIRST COMMENT\n");
        __global_lru = (LRU_Cache *)malloc(sizeof(LRU_Cache));
        insert(__global_lru, main_server, newstr);
        // __global_lru->head = NULL;
        // __global_lru->head =
        // __global_lru->head->hashind = main_server;
        // // printf("FIRST COMMENT\n");
        // strcpy(__global_lru->head->name, str);
        // __global_lru->head->next = NULL;
        // __global_lru->size = 1;
        return x;
    }

    insert(__global_lru, main_server, newstr);
    if (x == -1)
    {
        return -1;
    }
    return x;
}

int search(char *str) // searches in lru first, if not found then in trie returns -1 if not found in both
{
    if (!__global_trie)
    {
        return -1;
    }

    // int x = find_in_cache(__global_lru, str);
    char *newstr = handle_slash(str);
    int x = find_and_update(__global_lru, newstr);
    if (x != -1)
    {
        return x;
    }
    x = find_in_trie(__global_trie, newstr);
    if (x != -1)
    {
        return x;
    }
    return -1;
}
// int find_all(int *arr, int max_conn, char *str); // to remove warning

int *delete_file_folder(char *str) // first deletes from LRU, then deletes from trie
{
    if (!__global_trie)
    {
        return NULL;
    }

    char *newstr = handle_slash(str);
    int *arr = calloc(NS_MAX_CONN, sizeof(int));
    if (arr == NULL)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    int x = find_all(__global_trie, arr, NS_MAX_CONN, newstr);
    if (x == -1)
    {
        printf("Wrong Path\n");
        free(arr);
        return NULL;
    }

    delete_from_cache(__global_lru, newstr);
    delete_from_trie(__global_trie, newstr);
    return arr;
}

pthread_mutex_t *what_the_lock(char *str) // returns a pointer to lock for that file
{
    if (!__global_trie)
    {
        return NULL;
    }

    char *newstr = handle_slash(str);
    return lock_in_trie(__global_trie, newstr);
}

int ls(char *str, FILE *fp) // lists all files and subfiles
{
    if (!__global_trie)
    {
        return 0;
    }

    char *newstr = handle_slash(str);
    int result = print_all_childs(__global_trie, newstr, fp);
    return result;
}
// void ls(char* str); //
