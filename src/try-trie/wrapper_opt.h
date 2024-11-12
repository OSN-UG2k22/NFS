#ifndef WRAPPER_OPT_H
#define WRAPPER_OPT_H

#include "trie.h"
#include "lru_cache.h"

trienode *__global_trie = NULL;
LRU_Cache *__global_lru = NULL;

int create(int main_server, char *str); // takes main server and string path inserts in trie updates lru
int search(char *str);                  // searches in lru first, if not found then in trie returns -1 if not found in both
int* delete_file_folder(char *str); // first deletes from lru then deletes from from trie
pthread_mutex_t *what_the_lock(char *str); // returns a pointer to lock for that file
void ls(char* str); // lists all files and subfiles
// lock(char *str)
// unlock(char *str)
// lock_status(char *str)

#endif