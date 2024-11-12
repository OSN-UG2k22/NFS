#include "lru_cache.h"

// typedef struct node
// {
//     int hashind;
//     char name[100];
//     struct node *next;
//     struct node *prev;
// } node;

// typedef struct LRU_Cache
// {
//     node *head;
//     int size;
// } LRU_Cache;

// void initialize_cache(LRU_Cache *cache)
// {
//     __global_lru = (LRU_Cache *)malloc(sizeof(LRU_Cache));
//         __global_lru->head = NULL;
//         __global_lru->head->hashind = main_server;
//         strcpy(__global_lru->head->name, str);
//         __global_lru->head->next = NULL;
//         __global_lru->size = 1;
//     cache->head = NULL;
//     cache->size = 0;
// }

#define MAX_CACHE_SIZE 10

void insert(LRU_Cache *cache, int ind, char *str)
{
    struct node *temp = (struct node *)malloc(sizeof(struct node));
    temp->hashind = ind;
    strcpy(temp->name, str);
    temp->next = NULL;
    temp->prev = NULL;
    cache->size++;

    if (cache->head == NULL)
    {
        cache->head = temp;
    }
    else
    {
        temp->next = cache->head;
        cache->head->prev = temp;
        cache->head = temp;
    }
}

void debug_print_cache(LRU_Cache *cache)
{
    struct node *temp = cache->head;
    while (temp != NULL)
    {
        printf("%s : %d\n", temp->name, temp->hashind);
        temp = temp->next;
    }
    // printf("\n");
}

void delete_from_cache(LRU_Cache *cache, char *str)
{
    struct node *temp = cache->head;
    int ind = 0;
    while (temp != NULL && ind < MAX_CACHE_SIZE + 1)
    {
        if (strcmp(str, temp->name) == 0)
        {
            if (temp->prev == NULL)
            {
                cache->head = temp->next;
                temp->next->prev = NULL;
            }
            else if (temp->next == NULL)
            {
                temp->prev->next = NULL;
            }
            else
            {
                temp->prev->next = temp->next;
                temp->next->prev = temp->prev;
            }
            free(temp);
            return;
        }
        temp = temp->next;
        ind++;
    }
}

int find_in_cache(LRU_Cache *lru, char *temp)
{
    node *current = lru->head;
    int ind = 0;
    // while (current != NULL)
    while (current != NULL && ind < MAX_CACHE_SIZE)
    {
        if (strcmp(current->name, temp) == 0)
        {
            return current->hashind;
        }
        current = current->next;
        ind++;
    }
    if (MAX_CACHE_SIZE == ind)
    {
        current->next = NULL;
    }
    return -1;
}

int find_and_update(LRU_Cache *cache, char *str)
{
    int x = find_in_cache(cache, str);
    if (x != -1)
    {
        delete_from_cache(cache, str);
        insert(cache, x, str);
        return x;
    }
    return -1;
    // find_in_trie(str); logic
}
