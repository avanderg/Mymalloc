#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define BLK_SIZE 65536
#define MALLOC_ALIGN 16

#if defined (__x86_64)
    #define x86
#endif

#define MALLOC 1
#define FREE 2
#define CALLOC 3
#define REALLOC 4



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
header *find_header(void *ptr);
void merge(header *hptr);
void remove_node(header *hptr); 
void *realloc(void *ptr, size_t size); 
bool setup(void); 
void check_heap_top(void); 
void *alloc(size_t size); 
void insert_node(header *ptr, size_t size); 
void *create_node(header *ptr, size_t size); 
void myprint(char *s); 
void print_list(void); 
uintptr_t round_up(uintptr_t addr);
void print_debug(int kind, void *ptr, size_t total_size, size_t nmemb,
        size_t size, void *old_ptr); 
