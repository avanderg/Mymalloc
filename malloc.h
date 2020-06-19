/* Aaron VanderGraaff 6/03/2020 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define BLK_SIZE 65535 /* Block size to sbrk */
#define MALLOC_ALIGN 16 /* Align all addresses to 16 */

/* This works on both ARM and x86. But they use different typedefs, so 
   the prints in the debug functions need to be changed for each */
#if defined (__x86_64)
    #define x86
#endif

/* For checking which thing to print_debug() */
#define MALLOC 1 
#define FREE 2
#define CALLOC 3
#define REALLOC 4

/* Boolean data type for my eyes */
typedef enum bool {false, true} bool;

/* The header struct stores metadata for each data hunk */
typedef struct header header;
struct header {
    size_t size; /* Size of block */
    bool allocated; /* Is the block in use? */
    header *next; /* Next and prev for linked list */ 
    header *prev;
};


/*********** The Alloc functions **************/

void *calloc(size_t nmemb, size_t size); 
/* calloc() takes a size_t nmemb and a size_t size. It then sets
   all nmemb elements of size size to 0 on the heap and returns 
   a pointer to the beginning of the allocated memory.
*/

void *malloc(size_t size); 
/* malloc() allocates size bytes on the heap then returns
   a pointer to the beginning of the alocated memory. malloc() gives no
   guarantee of the contents of memory.
*/

void free(void *ptr); 
/* free() frees a pointer previously allocated by malloc to the system.
   The freed hunk can be used by subsequent calls to malloc and is no
   longer safe to use in its unnallocated state.
*/

void *realloc(void *ptr, size_t size); 
/* realloc() takes a void *ptr previously allocated by a malloc 
   function and a size_t size. If ptr is NULL, realloc(NULL, size) 
   becomes malloc(size). Otherwise, realloc() grows the memory hunk 
   pointed to by ptr to size size and returns a pointer to the new 
   memory hunk. realloc() preserves the contents of ptr from before the 
   call.
*/


/*********** Helper functions **************/

header *find_header(void *ptr);
/* find_header() finds the header metadata that describes the ptr's
   data section. If no header is found (ie ptr is not in the heap),
   returns NULL. 
*/

void merge(header *hptr);
/* merge() attempts to merge hptr with its adjacent nodes.
   If the next node is free, it is merged into hptr. 
   If the previous node is free, it checks if hptr is unallocated first 
   (since realloc calls this funcion, it is very possible hptr is 
    allocated), then merges hptr into its previous node. 
*/

void remove_node(header *hptr); 
/* remove_node() removes hptr from the header list */

int setup(void); 
/* setup() sets up heap if need be with initial call to sbrk(). It 
   initializes the global variables: heap_bot, heap_cur, and heap_top
   used for heap bookkeeping. It also sets the global debug variables
   used to tell the program how much information to print as it runs 
*/

void check_heap_top(void); 
/* check_heap_top() checks for large amounts of memory sbrkd at the top of 
   the heap but not allocated to nodes and returns it to the system. 
   This would only happen in a failed sbrk for a large amount of memory 
*/

void *alloc(size_t size); 
/* alloc() does the heavy lifting for the calloc(), malloc, and 
   realloc() functions. Given a size, it allocates memory on the heap,
   creates a header metadata, and returns a pointer to the beginning of
   the data.
*/

void insert_node(header *ptr, size_t size); 
/* insert_node() takes a header and a size and splits the given
   node if its size is bigger than the size by 2 alignment blocks 
   (if it's smaller than that, there isn't room for a new node and 
   its data. An alignment block is the smallest unit that can be
   placed on the heap, a node can't take up less than 2 (1 for
   the header, and 1 data alignment block))
*/

void *create_node(header *ptr, size_t size); 
/* create_node() creates a new node at the end of the header list with 
   the size given. It assumes hptr is the end of the list. If hptr is NULL,
   it assumes there is no list and starts one.
*/

uintptr_t round_up(uintptr_t addr);
/* Forces addr to be divisible by MALLOC_ALIGN */


/*********** Debug printing functions **************/
void myprint(char *s); 
/* Simple print function that uses snprintf instead of printf to avoid 
   calls to malloc
*/

void print_list(void); 
/* Prints the header list and each node's data members */

void print_debug(int kind, void *ptr, size_t total_size, size_t nmemb,
        size_t size, void *old_ptr); 
/* Each time an Alloc function is called: print what was called, 
   the parameters, and the result
*/
