#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct trienode
{
    bool lastnode;
    int hashind;
    pthread_mutex_t lock;
    struct trienode *child[256];
} trienode;

trienode *newnode();                                    // done
int trieinsert(trienode *temp, char *str, int hashind); // returns -1 if duplicate, if success returns hashind
int find_in_trie(trienode *root, char *str);

int find_all(trienode *root, int *arr, int ns_max_conn, char *str);
void mark_subtree(trienode *node, int *arr, int ns_max_conn);

int delete_from_trie(trienode *root, char *str);
void initialize_trie(trienode **root);


int find_subtree_new(trienode *node);
int find_new(trienode *root, char *str, int *is_partial);


void debug_print_trie(trienode *root);
void debug_print(trienode *root, char *str, int level);
pthread_mutex_t *lock_in_trie(trienode *root, char *str);
int print_all_childs(trienode *root, char *str, FILE *fp);

#endif