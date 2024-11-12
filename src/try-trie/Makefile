CC = gcc
CFLAGS = -Wall -Wextra -g

all: simplewrapper simpletrie simplelru

simplewrapper: test_wrapper.c wrapper_opt.c trie.c lru_cache.c
	$(CC) $(CFLAGS) -o simplewrapper test_wrapper.c wrapper_opt.c trie.c lru_cache.c

simpletrie: test_trie.c trie.c
	$(CC) $(CFLAGS) -o simpletrie test_trie.c trie.c

simplelru: test_lru.c lru_cache.c
	$(CC) $(CFLAGS) -o simplelru test_lru.c lru_cache.c

clean:
	rm -f simplewrapper simpletrie simplelru


# LRU_SRC = lru_cache.c
# TRIE_SRC = trie.c
# WRAPPER_SRC = wrapper.c

# HEADERS = lru_cache.h trie.h wrapper_opt.h

# TEST_LRU = test_lru.c
# TEST_TRIE = test_try.c
# TEST_WRAPPER = test_wrapper.c

# LRU_EXEC = test_lru
# TRIE_EXEC = test_trie
# WRAPPER_EXEC = test_wrapper

# all: $(WRAPPER_EXEC)
# trie: $(TRIE_EXEC)
# lru: $(LRU_EXEC)
# wrapper: $(WRAPPER_EXEC)

# $(WRAPPER_EXEC): $(WRAPPER_SRC) $(TEST_WRAPPER) $(TRIE_SRC) $(LRU_SRC)
# 	$(CC) $(CFLAGS) -o wrapper $(TEST_WRAPPER) $(WRAPPER_SRC) $(LRU_SRC) $(TRIE_SRC)

# clean:
# 	rm -f $(LRU_EXEC) $(TRIE_EXEC) $(WRAPPER_EXEC)

