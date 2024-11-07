CC=gcc

# release build, with max optimization flag
rel: CFLAGS=-O3
rel: client nserver sserver

# debug build, with flags to catch issues
dev: CFLAGS=-Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer
dev: client nserver sserver

client: src/client.c src/sockutils.c
	$(CC) $(CFLAGS) $^ -o $@

nserver: src/nserver.c src/sockutils.c
	$(CC) $(CFLAGS) $^ -o $@

sserver: src/sserver.c src/sockutils.c
	$(CC) $(CFLAGS) $^ -o $@

# clean command
clean:
	rm client nserver sserver
