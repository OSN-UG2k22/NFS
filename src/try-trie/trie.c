#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
/*
initialize trie
insert into trie
find in trie    // return hashind if found else -1 if not found
delete from trie
*/

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
        return 1;
    }
    else
    {
        return -1;
    }
}

int main()
{
    trienode *root = NULL;
    initialize_trie(&root);

    trieinsert(root, "home", 1);
    trieinsert(root, "home/abc", 2);
    trieinsert(root, "home/abc/def/ghi", 4);
    trieinsert(root, "home/abc/def/ghi/jkl/mno", 6);
    trieinsert(root, "home/abc/def", 3);
    trieinsert(root, "home/abc/def/ghi/jkl", 5);

    trieinsert(root, "abhi", 7);
    trieinsert(root, "abhiram", 8);
    trieinsert(root, "abhiram/abc", 9);
    trieinsert(root, "abhijeet", 10);

    debug_print_trie(root);

    int x = find_in_trie(root, "home");
    if (x != -1)
    {
        printf("find home ouput %d\n", x);
        printf("HOME found\n");
    }
    else
    {
        printf("Not found\n");
    }

    x = find_in_trie(root, "akshara");
    if (x != -1)
    {
        printf("find akshara ouput %d\n", x);
        printf("Akshara found\n");
    }
    else
    {
        printf("Akshara Not found\n");
    }

    x = find_in_trie(root, "abhiram");
    if (x != -1)
    {
        printf("find abhiram ouput %d\n", x);
        printf("Abhiram found\n");
    }
    else
    {
        printf("Abhiram Not found\n");
    }

    delete_from_trie(root, "abhiram");

    x = find_in_trie(root, "abhiram");
    if (x != -1)
    {
        printf("find abhiram ouput %d\n", x);
        printf("Abhiram found\n");
    }
    else
    {
        printf("Abhiram Not found\n");
    }

    return 0;
}