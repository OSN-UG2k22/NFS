#include "trie.h"

trienode *newnode()
{
    // printf("DEBUG MESSAGE : AA GYA YHN M\n");
    trienode *node = (trienode *)malloc(sizeof(trienode));
    // printf("DEBUG MESSAGE : YHN NHI AAYA M\n");
    node->lastnode = false;
    if (pthread_mutex_init(&node->lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return NULL;
    }
    node->hashind = -1;
    for (int i = 0; i < 256; i++)
    {
        node->child[i] = NULL;
    }
    return node;
}

int trieinsert(trienode *temp, char *str, int hashind)
{
    for (int i = 0; i < (int)strlen(str); i++)
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
    return;
}

int find_in_trie(trienode *root, char *str)
{
    for (int i = 0; i < (int)strlen(str); i++)
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
    for (int i = 0; i < (int)strlen(str); i++)
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

int find_all(trienode *root, int *arr, int NS_MAX_CONN, char *str)
{
    if (root == NULL)
    {
        return -1;
    }

    for (int i = 0; i < (int)strlen(str); i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return -1;
        }
        root = root->child[(unsigned char)str[i]];
    }
    mark_subtree(root, arr, NS_MAX_CONN);
    return 1;
}

void mark_subtree(trienode *node, int *arr, int NS_MAX_CONN)
{
    if (node == NULL)
    {
        return;
    }
    if (node->hashind != -1 && node->hashind < NS_MAX_CONN && node->hashind >= 0)
    {
        arr[node->hashind] = 1;
    }
    for (int i = 0; i < 256; i++)
    {
        if (node->child[i] != NULL)
        {
            mark_subtree(node->child[i], arr, NS_MAX_CONN);
        }
    }
}

pthread_mutex_t *lock_in_trie(trienode *root, char *str)
{
    for (int i = 0; i < (int)strlen(str); i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return NULL;
        }
        root = root->child[(unsigned char)str[i]];
    }
    if (root->hashind != -1)
    {
        return &root->lock;
    }
    else
    {
        return NULL;
    }
}

void print_all_subtree(trienode *node, char *path, int level, FILE *fp)
{
    if (node == NULL)
    {
        return;
    }

    if (node->hashind != -1 || node->child[(int)'/'] != NULL)
    {
        path[level] = '\0';
        // printf("%s (hashind: %d)\n", path, node->hashind);
        fprintf(fp, "%s\n", path);
    }

    for (int i = 0; i < 256; i++)
    {
        char c = (char)i;
        if (c == '/')
        {
            continue;
            // printf("%s\n", str);
            // return;
        }
        if (node->child[i] != NULL)
        {
            path[level] = (char)i; // Append the character to the current path
            print_all_subtree(node->child[i], path, level + 1, fp);
        }
    }
}

int print_all_childs(trienode *root, char *str, FILE *fp)
{
    int len = (int)strlen(str);

    for (int i = 0; i < len; i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return 0;
        }
        root = root->child[(unsigned char)str[i]];
    }
    char path[256];
    if (root->child[(int)'/'] == NULL)
    {
        if (root->hashind != -1)
        {
            fprintf(fp, "%s\n", str);
        }
        else
        {
            return 0;
        }
        return 1;
    }
    root = root->child[(int)'/'];

    print_all_subtree(root, path, 0, fp);
    return 1;
}
