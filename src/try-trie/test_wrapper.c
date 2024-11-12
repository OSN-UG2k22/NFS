#include "wrapper_opt.h"

// extern trienode* __global_trie = NULL;
// extern LRU_Cache* __global_lru = NULL;

int main()
{
    create(1, "home");
    create(2, "home/abc");
    create(4, "home/abc/def/ghi");
    create(6, "home/abc/def/ghi/jkl/mno");
    create(3, "home/abc/def");
    create(5, "home/abc/def/ghi/jkl");
    create(7, "abhi");
    create(8, "abhiram");
    create(9, "abhiram/abc");
    create(10, "abhijeet");
    int x = search("home");
    printf("find home ouput %d\n", x);
    printf("find home/abc ouput %d\n", search("home/abc"));
    printf("find home/abc/def ouput %d\n", search("home/abc/def"));

    delete_file_folder("home/abc/def");
    
    printf("find home/abc/def ouput %d\n", search("home/abc/def"));
    printf("find home/abc/invalid ouput %d\n", search("home/abc/invalid"));
    printf("find abhiram/abc ouput %d\n", search("abhiram/abc"));

    return 0;
}
