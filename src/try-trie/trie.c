#include "trie.h"


int is_file(trienode *root, char *str)
{
    for (int i = 0; i < (int)strlen(str); i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return -1; // invalid path
        }
        root = root->child[(unsigned char)str[i]];
    }
    if (root->child[(int)'/'] == NULL)
    {
        if (root->hashind != -1){//there is a server for this file so it is a file
            return 1; // is file
        }
        return -1; // is not a file
    }
    return 0; // is directory
}

void print_all_subtree_complete(trienode *node, char *path, int level, FILE *fp, char* str)
{
    if (node == NULL)
    {
        return;
    }

    if (node->hashind != -1)
    {
        path[level] = '\0';
        fprintf(fp, "%s", str);
        fprintf(fp, "%s\n", path);
    }

    for (int i = 0; i < 256; i++)
    {
        // char c = (char)i;
        if (node->child[i] != NULL)
        {
            path[level] = (char)i; // Append the character to the current path
            print_all_subtree_complete(node->child[i], path, level + 1, fp, str);
        }
    }
}

int print_all_childs_v2(trienode *root, char *str, FILE *fp)
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

    print_all_subtree_complete(root, path, 0, fp, str);
    return 1;
}

int find_subtree_new(trienode *node) // returns what server it is in
{
    if (node == NULL)
    {
        return -1;
    }
    if (node->hashind != -1)
    {
        return node->hashind;
    }
    for (int i = 0; i < 256; i++)
    {
        if (node->child[i] != NULL)
        {
            int y = find_subtree_new(node->child[i]);
            if (y != -1)
            {
                return y;
            }
        }
    }
    return -1;
}

int find_new(trienode *root, char *str, int *is_partial)
{
    if (root == NULL)
    {
        return -1;
    }
    int i = 0;
    trienode *lastslash = root;
    trienode *rootc = root;
    *is_partial = 1;
    for (; i < (int)strlen(str); i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            *is_partial = 1;
            break;
        }
        if (str[i] == '/')
        {
            lastslash = root;
        }
        root = root->child[(unsigned char)str[i]];
    }

    if (i == (int)strlen(str) && root->child[(int)'/'] == NULL)
    {
        // is file or incomplete match
        *is_partial = 0;
        return root->hashind;
    }
    if (i == (int)strlen(str) && root->child[(int)'/'] != NULL)
    {
        // is directory
        *is_partial = 0;
        return find_subtree_new(root);
    }
    *is_partial = 1;
    if (lastslash != rootc)
    {
        return find_subtree_new(lastslash);
    }
    return -1;
}
trienode *newnode()
{
    trienode *node = (trienode *)malloc(sizeof(trienode));
    node->lastnode = false;
    if (pthread_mutex_init(&node->lock, NULL) != 0)
    {
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

void initialize_trie(trienode **root)
{
    *root = newnode();
    return;
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

void print_all_subtree(trienode *node, char *path, int level, FILE *fp)
{
    if (node == NULL)
    {
        return;
    }

    if (node->hashind != -1 || node->child[(int)'/'] != NULL)
    {
        path[level] = '\0';
        fprintf(fp, "%s\n", path);
    }

    for (int i = 0; i < 256; i++)
    {
        char c = (char)i;
        if (c == '/')
        {
            continue;
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
