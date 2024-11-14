#include "wrapper_opt.h"

trienode *__global_trie;
LRU_Cache *__global_lru;

int create(int main_server, char *str) // takes main server and string path inserts in trie updates lru
{
    if (__global_trie == NULL)
    {
        initialize_trie(&__global_trie);
    }
    int x = trieinsert(__global_trie, str, main_server);
    if (x == -1)
    {
        insert(__global_lru, main_server, str);
        return x;
    }

    if (__global_lru == NULL)
    {
        // printf("FIRST COMMENT\n");
        __global_lru = (LRU_Cache *)malloc(sizeof(LRU_Cache));
        insert(__global_lru, main_server, str);
        // __global_lru->head = NULL;
        // __global_lru->head =
        // __global_lru->head->hashind = main_server;
        // // printf("FIRST COMMENT\n");
        // strcpy(__global_lru->head->name, str);
        // __global_lru->head->next = NULL;
        // __global_lru->size = 1;
        return x;
    }

    insert(__global_lru, main_server, str);
    if (x == -1)
    {
        return -1;
    }
    return x;
}

int search(char *str) // searches in lru first, if not found then in trie returns -1 if not found in both
{
    // int x = find_in_cache(__global_lru, str);
    int x = find_and_update(__global_lru, str);
    if (x != -1)
    {
        return x;
    }
    x = find_in_trie(__global_trie, str);
    if (x != -1)
    {
        return x;
    }
    return -1;
}
// int find_all(int *arr, int max_conn, char *str); // to remove warning

int *delete_file_folder(char *str) // first deletes from LRU, then deletes from trie
{
    int *arr = calloc(NS_MAX_CONN, sizeof(int));
    if (arr == NULL) {
        perror("Failed to allocate memory");
        return NULL;
    }
    
    int x = find_all(__global_trie, arr, NS_MAX_CONN, str);
    if (x == -1)
    {
        printf("Wrong Path\n");
        free(arr);
        return NULL;
    }

    delete_from_cache(__global_lru, str);
    delete_from_trie(__global_trie, str);
    return arr;
}

pthread_mutex_t *what_the_lock(char *str) // returns a pointer to lock for that file
{
    return lock_in_trie(__global_trie, str);
}

void ls(char *str) // lists all files and subfiles
{
    print_all_childs(__global_trie, str);
    return;
}
// void ls(char* str); //
