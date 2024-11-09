#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// #define MAX_CACHE_SIZE 10

// ********************************TRIE****************************************

typedef struct trienode
{
    bool lastnode;
    int hashind;
    struct trienode *child[256];
} trienode;

trienode *newnode()
{
    trienode *node = (trienode *)malloc(sizeof(trienode));
    node->lastnode = false;
    node->hashind = -1;
    for (int i = 0; i < 256; i++)
    {
        node->child[i] = NULL;
    }
    return node;
}

int trieinsert(trienode *temp, char *str, int hashind)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if (temp->child[(unsigned char)str[i]] == NULL)
        {
            temp->child[(unsigned char)str[i]] = newnode();
        }
        temp = temp->child[(unsigned char)str[i]];
    }
    if (temp->hashind != -1)
    {
        return -1; // Duplicate entry
    }
    else
    {
        temp->hashind = hashind;
        temp->lastnode = true;
        return temp->hashind;
    }
}

void debug_print(trienode *root, char *str, int level)
{
    if (root == NULL)
    {
        return;
    }
    if (root->hashind != -1)
    {
        str[level] = '\0';
        printf("%s %d\n", str, root->hashind);
    }
    for (int i = 0; i < 256; i++)
    {
        if (root->child[i] != NULL)
        {
            str[level] = (char)i;
            debug_print(root->child[i], str, level + 1);
        }
    }
}

void debug_print_trie(trienode *root)
{
    if (root == NULL)
    {
        printf("Trie is empty\n");
        return;
    }
    char str[256];
    debug_print(root, str, 0);
}

void initialize_trie(trienode **root)
{
    *root = newnode();
}

int find_in_trie(trienode *root, char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return -1;
        }
        root = root->child[(unsigned char)str[i]];
    }
    if (root->hashind != -1)
    {
        return root->hashind;
    }
    else
    {
        return -1;
    }
}

int delete_from_trie(trienode *root, char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return -1;
        }
        root = root->child[(unsigned char)str[i]];
    }
    if (root->hashind != -1)
    {
        root->hashind = -1;
        // found
        // also can free its children but who cares for storage
        return 1;
    }
    else
    {
        // not found
        return -1;
    }
}

// ********************************TRIE****************************************

typedef struct node
{
    int hashind;
    char name[100];
    struct node *next;
    struct node *prev;
} node;

typedef struct LRU_Cache
{
    node *head;
    trienode *root;
    int size;
} LRU_Cache;

void initialize_cache(LRU_Cache *cache)
{
    cache->head = NULL;
    initialize_trie(&cache->root);
    cache->size = 0;
}

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

void delete(LRU_Cache *cache, char *str)
{
    struct node *temp = cache->head;
    while (temp != NULL)
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
    }
}

int find_in_cache(LRU_Cache *lru, char *temp)
{
    node *current = lru->head;
    // int ind = 0;
    // while(current!=NULL&&ind<MAX_CACHE_SIZE)
    while (current != NULL)
    {
        if (strcmp(current->name, temp) == 0)
        {
            return current->hashind;
        }
        current = current->next;
        // ind++;
    }
    return -1;
}

int find_and_update(LRU_Cache *cache, char *str)
{
    int x = find_in_cache(cache, str);
    if (x != -1)
    {
        delete (cache, str);
        insert(cache, str, x);
        return x;
    }
    return -1;
    // find_in_trie(str); logic
}

int main()
{
    LRU_Cache mycash;
    initialize_cache(&mycash);
    insert(&mycash, 1, "Abhi");
    insert(&mycash, 2, "Abhiram");
    insert(&mycash, 3, "Abhijeet");
    insert(&mycash, 4, "Abhilaksh");
    insert(&mycash, 5, "Abhinav");
    debug_print_cache(&mycash);
    printf("\n");
    printf("Abhiram: %d\n", find_in_cache(&mycash, "Abhiram"));
    printf("Akshara: %d\n", find_in_cache(&mycash, "Akshara"));

    delete (&mycash, "Abhiram");
    printf("Abhiram: %d\n", find_in_cache(&mycash, "Abhiram"));

    return 0;
}
