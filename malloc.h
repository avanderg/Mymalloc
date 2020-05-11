#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define BLK_SIZE 65536

#if defined(__x86_64)
    #define x86
#endif

#define MALLOC 1
#define FREE 2
#define CALLOC 3
#define REALLOC 4


#define NEXT 1
#define PREV 2

typedef enum bool {false, true} bool;
typedef struct header header;

struct header {
    size_t size; /* Size of block */
    bool allocated; /* Is the block in use? */
    header *next; /* Next and prev for linked list */ 
    header *prev;
};

void *calloc(size_t nmemb, size_t size); 
void *malloc(size_t size); 
void free(void *ptr); 
header *find_header(void *ptr, bool debug);
void merge(header *hptr, bool debug);
void remove_node(header *hptr); 
void *realloc(void *ptr, size_t size); 
bool setup(void); 
void check_heap_top(bool debug); 
void *alloc(size_t size, bool debug); 
void insert_node(header *ptr, size_t size, bool debug); 
void *create_node(header *ptr, size_t size, bool debug); 
void myprint(char *s, bool debug); 
void print_list(void); 
uintptr_t round_up(uintptr_t addr);
void print_debug(int kind, void *ptr, size_t total_size, size_t nmemb,
        size_t size, void *old_ptr); 
