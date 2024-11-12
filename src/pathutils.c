#include "common.h"

/* Returns a path with prefix removed if prefix matches, else returns NULL.
 * The returned buffer is simply a pointer to memory within 'self' argument */
char *path_remove_prefix(char *self, char *op)
{
    if (!self || !op)
    {
        return self;
    }

    ssize_t op_len = strlen(op);
    if (op[op_len - 1] == '/')
    {
        op_len--;
    }
    if (!op_len)
    {
        /* "/" is a prefix for everything */
        return self;
    }

    if (!strncmp(self, op, op_len))
    {
        /* found match */
        char *ret = self + op_len;
        if (ret[0] != '/' && ret[0])
        {
            /* not a perfect match */
            return NULL;
        }
        return ret;
    }
    return NULL;
}

/* Returns a char buffer that should be freed by caller, or NULL on failure */
char *path_concat(char *first, char *second)
{
    ssize_t first_len = strlen(first);
    ssize_t second_len = strlen(second);
    while (first_len && first[first_len - 1] == '/')
    {
        /* Ignore trailing slashes */
        first_len--;
    }

    while (second[0] == '/')
    {
        /* Ignore leading slashes */
        second++;
        second_len--;
    }

    /* +2 is for / and trailing NULL */
    char *ret = malloc(first_len + second_len + 2);
    if (ret)
    {
        memcpy(ret, first, first_len);
        if (second_len)
        {
            ret[first_len] = '/';
            memcpy(ret + first_len + 1, second, second_len);
            ret[first_len + second_len + 1] = '\0';
        }
        else
        {
            /* Avoid trailing slash */
            ret[first_len] = '\0';
        }
    }
    return ret;
}
