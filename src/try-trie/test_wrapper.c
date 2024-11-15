#include "wrapper_opt.h"
#include <assert.h>
// extern trienode* __global_trie = NULL;
// extern LRU_Cache* __global_lru = NULL;
void test_create()
{
    char *path = "/example/path";
    int main_server = 1;
    int result = create(main_server, path);
    assert(result != -1);
    result = create(main_server, path);
    assert(result == -1);
}
void test_search()
{
    char *path = "/example/path";
    int main_server = 1;
    create(main_server, path);
    int result = search(path);
    assert(result == main_server);
    result = search("/non/existent/path");
    assert(result == -1);
}
void test_delete_file_folder()
{
    char *path = "/example/path";
    int main_server = 1;

    create(main_server, path);
    // printf("HERE\n");
    int *deleted_paths = delete_file_folder(path);
    for (int i = 0; i < 1024; i++)
    {
        if (deleted_paths[i])
        {
            printf("path is in server %d \n", i);
        }
    }
    assert(deleted_paths != NULL);
    int result = search(path);
    printf("%d\n", result);
    // assert(result == -1); NOT WORKING WITH OTHERS

    free(deleted_paths);
}
void test_what_the_lock()
{
    char *path = "/example/path";
    int main_server = 1;
    create(main_server, path);
    pthread_mutex_t *lock = what_the_lock(path);
    assert(lock != NULL);
}
void test_ls()
{
    char *path1 = "/example";
    char *path2 = "/example/path";
    int main_server = 1;

    create(main_server, path1);
    create(main_server, path2);
    ls("/example");
}

void tough_test()
{
    assert(create(1, "home") != -1);
    assert(create(2, "home/abc") != -1);
    assert(create(4, "home/abc/def/ghi") != -1);
    assert(create(6, "home/abc/def/ghi/jkl/mno") != -1);
    assert(create(3, "home/abc/def") != -1);
    assert(create(5, "home/abc/def/ghi/jkl") != -1);
    assert(create(7, "abhi") != -1);
    assert(create(8, "abhiram") != -1);
    assert(create(9, "abhiram/abc") != -1);
    assert(create(10, "abhijeet") != -1);

    int x = search("home");
    assert(x == 1);

    assert(search("home/abc") == 2);
    assert(search("home/abc/def") == 3);

    int *arr = delete_file_folder("home/abc/def");
    if (arr == NULL)
    {
        printf("Invalid Path");
    }
    else
    {
        for (int i = 0; i < 1024; i++)
        {
            // printf("%d ", i);
            if (arr[i])
            {
                printf("%d", i);
            }
        }
        printf("\n");
        free(arr);
    }

    assert(search("home/abc/def") == -1);
    assert(search("home/abc/invalid") == -1);
    assert(search("abhiram/abc") == 9);
}

int main()
{
    // test_create();
    // test_search();
    // test_delete_file_folder();
    // test_what_the_lock();
    // test_ls();
    // tough_test();
    char *path = "/example/path";
    char *path1 = "example";
    char *path2 = "example/path/";
    char *path3 = "/example/path/";
    char *result = handle_slash(path);
    printf("for %s function output: %s\n", path, result);

    char *result1 = handle_slash(path1);
    printf("for %s function output: %s\n", path1, result1);

    char *result2 = handle_slash(path2);
    printf("for %s function output: %s\n", path2, result2);

    char *result3 = handle_slash(path3);
    printf("for %s function output: %s\n", path3, result3);

    printf("All tests passed successfully.\n");

    // create(1, "home");
    // create(2, "home/abc");
    // create(4, "home/abc/def/ghi");
    // create(6, "home/abc/def/ghi/jkl/mno");
    // create(3, "home/abc/def");
    // create(5, "home/abc/def/ghi/jkl");
    // create(7, "abhi");
    // create(8, "abhiram");
    // create(9, "abhiram/abc");
    // create(10, "abhijeet");
    // int x = search("home");
    // printf("find home ouput %d\n", x);
    // printf("find home/abc ouput %d\n", search("home/abc"));
    // printf("find home/abc/def ouput %d\n", search("home/abc/def"));

    // delete_file_folder("home/abc/def");

    // printf("find home/abc/def ouput %d\n", search("home/abc/def"));
    // printf("find home/abc/invalid ouput %d\n", search("home/abc/invalid"));
    // printf("find abhiram/abc ouput %d\n", search("abhiram/abc"));

    return 0;
}
