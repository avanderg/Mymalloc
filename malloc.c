/* Aaron VanderGraaff 6/03/2020 */

#include "malloc.h"

/* TODO: Preliminary tests are looking pretty good.
         Need to do:
            * More intensive tests
                * Pretty intensive already done, have to think about other 
                  tests that could break it
            * README
                * Written but needs a proofread
*/

static void *heap_bot = NULL;
static void *heap_top = NULL;
static void *heap_cur = NULL;
static header *head_list = NULL;
static header *end_list = NULL;
static bool debug = false;
static bool debug_verbose = false;

void *calloc(size_t nmemb, size_t size) {
/* calloc() takes a size_t nmemb and a size_t size. It then sets
   all nmemb elements of size size to 0 on the heap and returns 
   a pointer to the beginning of the allocated memory.
*/

    void *ptr = NULL; /* Pointer to the hunk */

    /* Setup the heap and debug params */
    if (setup() == -1) {
        return NULL;
    }

    /* If 0 members or 0 size is passed, return NULL */
    if (!nmemb || !size) {
        if (debug_verbose) {
            myprint("Returning NULL\n");
        }
        return NULL;
    }

    /* Allocate the hunk */
    ptr = alloc(nmemb*size);

    /* If the hunk was allocated successfully allocated, memset to 0 */
    if (ptr) {
        memset(ptr, 0, nmemb*size);
    }

    if (debug) {
        print_debug(CALLOC, ptr, size*nmemb, nmemb, size, 0);
        myprint("\nprinting list:\n");
        print_list();
    }

    /* Return pointer to the beginning of the hunk */
    return ptr;
}

void *malloc(size_t size) {
/* malloc() allocates size bytes on the heap then returns
   a pointer to the beginning of the alocated memory. malloc() gives no
   guarantee of the contents of memory.
*/

    void *ptr = NULL; /* Pointer to the hunk */

    /* Setup the heap and debug params */
    if (setup() == -1) {
        return NULL;
    }

    /* If 0 size is passed, return NULL */
    if (!size) {
        if (debug) {
            print_debug(MALLOC, ptr, size, 0, 0, 0);
        }
        return NULL;
    }

    /* Allocate the hunk */
    ptr = alloc(size);
    if (debug) {
        print_debug(MALLOC, ptr, size, 0, 0, 0);
    }

    if (debug_verbose) {
        myprint("\nprinting list:\n");
        print_list();
    }

    /* Return pointer to the beginning of the hunk */
    return ptr;
}

void free(void *ptr) {
/* free() frees a pointer previously allocated by malloc to the system.
   The freed hunk can be used by subsequent calls to malloc and is no
   longer safe to use in its unnallocated state.
*/

    header *hptr; /* The header of the hunk to free */
    header *hptr2;
    void *tmp_ptr; /* Temporarily holds end of list for comparison */
    char buf[50]; /* Print buffer */

    /* If the pointer is NULL, nothing to do, just return */
    if (!ptr) { 
        return;
    }

    /* Find the header corresponding to this hunk */
    hptr = find_header(ptr);
    /*
    hptr2 = (header *) ((uintptr_t) ptr - round_up(sizeof(header)));
    snprintf(buf, 50, "sizeof(header): %lu\n", sizeof(header));
    fputs(buf, stderr);
    if (strcmp(hptr2->magic, MAGIC)) {
        snprintf(buf, 50, "did not find hptr: %p\n", hptr);
        fputs(buf, stderr);
        exit(EXIT_FAILURE);
    }
    snprintf(buf, 50, "found hptr: %p\n", hptr2);
    fputs(buf, stderr);
    snprintf(buf, 50, "magic: %s\n", hptr2->magic);
    fputs(buf, stderr);
    if (hptr != hptr2) {
        snprintf(buf, 50, "hptr != hptr2 .. hptr: %p\n", hptr);
        fputs(buf, stderr);
        print_debug(FREE, ptr, 0, 0, 0, 0);
        exit(EXIT_FAILURE);
    }
    */

    /* find_header returns NULL if the pointer wasn't allocated in the heap 
    */
    if (!hptr) {
        snprintf(buf, 40, "free(): Invalid pointer \n");
        fputs(buf, stderr);
        print_debug(FREE, ptr, 0, 0, 0, 0);
        exit(EXIT_FAILURE);
    }

    /* If the header is unallocated, freeing a free hunk .. bad */
    if (!hptr->allocated) {
        snprintf(buf, 32, "free(): Double free on pointer\n");
        fputs(buf, stderr);
        exit(EXIT_FAILURE);
    }

    /* Free the hunk */
    hptr->allocated = false;


    if (debug) {
        print_debug(FREE, ptr, 0, 0, 0, 0);
    }
    if (debug_verbose) {
        snprintf(buf, 20, "Freeing: %p\n", hptr);
        fputs(buf, stderr);
        myprint("Calling check_heap top in free\n");
    }

    /* Return memory to the system if there is a crap ton of memory
       at the top of the heap */
    check_heap_top();

    /* Return memory to system if end node is a large unallocated hunk*/
    if (!end_list->allocated && end_list->size > BLK_SIZE) {

        if (debug) {
            myprint("Returning memory to the system from node\n");
        }
        /* Return the size of the node to the system */
        if ((sbrk(-end_list->size)) == (void *) -1) {
            perror("sbrk");
            exit(EXIT_FAILURE);
        }
        /* Adjust heap_top and heap_cur */
        heap_top = (void *) ((uintptr_t) heap_top - end_list->size);
        heap_cur = (void *) ((uintptr_t) heap_cur - 
                               end_list->size + round_up(sizeof(header)));

        /* Save the end list to check in a moment */
        tmp_ptr = end_list;

        /* Remove the returned node from the list */
        remove_node(end_list);

        if (debug) {
            print_list();
        }

        /* If the header was the end_list that was removed, return here.
           Can't go on to merge b/c the node is gone
        */
        if (hptr == tmp_ptr) {
            return;
        }
    }

    if (debug) {
        myprint("before merge:\n");
        print_list(); 
    }
    /* Merge the freed node to minimize fragmentation */
    
    merge(hptr);

    if (debug) {
        myprint("\nafter merge:\n");
        print_list();
    }

}

void *realloc(void *ptr, size_t size) {
/* realloc() takes a void *ptr previously allocated by a malloc 
   function and a size_t size. If ptr is NULL, realloc(NULL, size) 
   becomes malloc(size). Otherwise, realloc() grows the memory hunk 
   pointed to by ptr to size size and returns a pointer to the new 
   memory hunk. realloc() preserves the contents of ptr from before the 
   call.
*/
    void *out_ptr; /* Pointer to the hunk */
    void *new_heap_bot; 
    long unsigned int sbrk_counter; /* Counts sbrk loops to grow amount 
                                       sbrkd each call */
    header *hptr; /* Header of the hunk to to realloc */
    char buf[50]; /* Print buffer */

    /* Setup the heap and debug params */
    if (setup() == -1) {
        return NULL;
    }

    /* If ptr passed is NULL, equivalent to calling malloc(size) */
    if (!ptr) {
        if (debug) {
            myprint("NULL ptr passed, doing a malloc\n");
        }

        /* If 0 size is passed, return NULL */
        if (!size) {
            return NULL;
        }

        /* Allocate the hunk */
        out_ptr = alloc(size);
        if (debug) {
            print_debug(REALLOC, out_ptr, size, 0, 0, ptr);
            print_list();
        }

        /* Return pointer to the beginning of the hunk */
        return out_ptr;
    }

    /* If size passed is 0, equivalent to calling free(ptr) */
    else if (!size) {
        if (debug) {
            myprint("0 size passed, calling free\n");
        }
        /* Call free */
        free(ptr);
        return NULL;
    }

    /* Find header corresponding to this hunk */
    hptr = find_header(ptr);

    /* find_header returns NULL if the pointer wasn't allocated in the heap
     */
    if (!hptr) {
        snprintf(buf, 40, "realloc(): Invalid pointer\n");
        fputs(buf, stderr);
        exit(EXIT_FAILURE);
    }

    /* Merge hunks if possible */
    merge(hptr);

    /* If reallocing to a smaller size, shrink the hunk if needed */
   
    if (hptr->size > size) {
        if (debug) {
            myprint("In place shrinking\n");
        }
        if (hptr->size - size > 2*MALLOC_ALIGN) {
            if (debug) {
                myprint("Shrinking node\n");
            }
            /* Insert a new node */
            insert_node(hptr, size);
        }
        if (debug) {
            print_debug(REALLOC, ptr, size, 0, 0, ptr);
            print_list();
        }
        return ptr;
    }

    /* If the node is the last in the list, just grab more memory 
       from the heap */
   if (!hptr->next) { 
        if (debug) {
            myprint("next is null\n");
        }

        sbrk_counter = 1; /* Counter used to slowly make sbrk hunks bigger 
                           */
        /* If the current heap location + size of hunk to be realloced is 
           greater than the top of the heap, sbrk needs to be called */
        while ((uintptr_t) heap_cur + 
                round_up(sizeof(header)) + round_up(size) > 
                (uintptr_t) heap_top) { 
            /* Need to call sbrk */
            if ((new_heap_bot = sbrk(BLK_SIZE*sbrk_counter)) 
                    == (void *) -1) {
                /* If sbrk fails, set errno to ENOMEM */
                errno = ENOMEM;
                if (debug) {
                    print_debug(REALLOC, ptr, size, 0, 0, ptr);
                }

                myprint("calling check heap top in realloc\n");
                /* check_heap_top() checks for large amounts of memory 
                   sbrkd at the top of the heap but not allocated to nodes
                   and returns it to the system. This would only happen in 
                   a failed sbrk for a large amount of memory */
                check_heap_top();
                /* Realloc failed, return NULL. User has to check errno if 
                   they want to know what happened */
                return NULL;
            }
            /* Sbrk was successful, adjust heap_top to use the new memory */
            heap_top = (void *) ((uintptr_t) new_heap_bot 
                    + BLK_SIZE*sbrk_counter++);
        }

        /* No new node is being made, just adjusting where this node ends.
           Need to adjust heap_cur so a new node isn't created in the middle
           of the newly expanded data section */
        heap_cur = (void *) round_up((uintptr_t) heap_cur) +
                            round_up(size) - round_up(hptr->size);
        /* Update the size */
        hptr->size = round_up(size);

        if (debug) {
            myprint("Doing an in place expansion using more heap\n");
            print_list();
            print_debug(REALLOC, ptr, size, 0, 0, ptr);
        }

        return ptr;

    }

    /* If we get here, need to copy to new hunk :( */

    /* First, allocate a new hunk */
    if (!(out_ptr = alloc(size))) {
        return NULL;
    }

    /* Now copy from old to new (ptr to out_ptr) */
    memcpy(out_ptr, ptr, hptr->size);
    /* "Free" the old hunk */
    /* Could call a proper free here to take advantage of other bookkeeping 
       done by free? */
    hptr->allocated = 0;
    /* Now there's a free node, try to merge it with neighbors */
    merge(hptr);

    if (debug) {
        print_debug(REALLOC, out_ptr, size, 0, 0, ptr);
        myprint("Making a new node and copying\n");
        print_list();
    }
    /* Make sure to return the new pointer, not the original */
    return out_ptr;
}

header *find_header(void *ptr) {
/* find_header() finds the header metadata that describes the ptr's
   data section. If no header is found (ie ptr is not in the heap),
   returns NULL. 
*/
    char buf[50];


    header *hptr;
    hptr = (header *) ((uintptr_t) ptr - round_up(sizeof(header)));
    /*
    snprintf(buf, 50, "sizeof(header): %lu\n", sizeof(header));
    fputs(buf, stderr);
    */
    if (strcmp(hptr->magic, MAGIC)) {
        snprintf(buf, 50, "did not find hptr: %p\n", hptr);
        fputs(buf, stderr);
        exit(EXIT_FAILURE);
        return NULL;

    }
    /*
    snprintf(buf, 50, "found hptr: %p\n", hptr);
    fputs(buf, stderr);
    snprintf(buf, 50, "magic: %s\n", hptr->magic);
    fputs(buf, stderr);
    */
    return hptr;
#ifdef OLD


    header *hptr;
    hptr = head_list;
    while(hptr) {
        /* If the address of the pointer is smaller than the address of 
           the header + the size of the hunk, then it must be in this hunk
       */
        if ((uintptr_t) ptr < (uintptr_t) hptr + 
                hptr->size + round_up(sizeof(header))) {
            return hptr;
        }
        hptr = hptr->next;
    }
    /* No header found, was passed a bogus ptr not in the heap */
    return NULL;
#endif
}

void merge(header *hptr) {
/* merge() attempts to merge hptr with its adjacent nodes.
   If the next node is free, it is merged into hptr. 
   If the previous node is free, it checks if hptr is unallocated first 
   (since realloc calls this funcion, it is very possible hptr is 
    allocated), then merges hptr into its previous node. 
*/

    /* Make sure hptr->next exists then check if its allocated */
    if (hptr->next && !hptr->next->allocated) {
        /* If hptr->next is not allocated, adjust hptr's size */
        hptr->size += hptr->next->size + round_up(sizeof(header));
        /* Remove hptr->next. All of it is now abosorbed by hptr */
        remove_node(hptr->next);
    }
    /* Make sure hptr is unallocated before removing it.
       This is for calls to realloc 
    */
    if (!hptr->allocated && hptr->prev && !hptr->prev->allocated) {
        /* Similar to above, but for hptr->prev instead of hptr */
        /* Adjust hptr->prev's size */
        hptr->prev->size += hptr->size + round_up(sizeof(header));
        /* Remove hptr. It is now absorbed by hptr->prev */
        remove_node(hptr);
    }
}

void remove_node(header *hptr) {
/* remove_node() removes hptr from the header list */

    /* If the head is getting removed without a next, there
       will be no list after the removal */
    if (!hptr->next && hptr == head_list) {
        head_list = NULL;
        end_list = NULL;
        return;
    }
    /* If hptr has a previous, change its next to hptr->next */
    if (hptr->prev) {
        hptr->prev->next = hptr->next;
    }
    /* If hptr has a next, change its previous to hptr->prev */
    if (hptr->next) {
        hptr->next->prev = hptr->prev;
    }
    /* If hptr doesn't have a next, it was the end of the list.
       hptr->prev will now be the end */
    else {
        end_list = hptr->prev;
    }
}


int setup() {
/* setup() sets up heap if need be with initial call to sbrk(). It 
   initializes the global variables: heap_bot, heap_cur, and heap_top
   used for heap bookkeeping. It also sets the global debug variables
   used to tell the program how much information to print as it runs 
*/

    char *debug_malloc;

    /* Check for debug */
    if ((debug_malloc = getenv("DEBUG_MALLOC"))) {
        debug = true; 
        if (!strcmp(debug_malloc, "1")) {
            debug_verbose = true;
        }
    }
    
    /* If the heap hasn't been allocated, allocate it */
    if (!heap_bot) {
        if (debug_verbose) {
            myprint("Setup heap\n");
        }
        /* sbrk() returns the start of the allocated section of memory,
           so set heap_bot to its result */
        if ((heap_bot = sbrk(BLK_SIZE)) == (void *) -1) {
            /* If sbrk fails, report ENOMEM and return -1 so 
               the calling function knows something failed */
            errno = ENOMEM;
            return -1;
        }
        /* heap_top is the end of the allocated section of memory, 
           set it to heap_bot + the amount carved with sbrk() */
        heap_top = (void * ) (uintptr_t) heap_bot + BLK_SIZE;
        /* heap_cur points to the end of used space on the heap, 
           nothing has been used yet, so set it to the bottom  */
        heap_cur = heap_bot;

    }
    
    return 0; /* Everything executed fine, return 0 */
}

void check_heap_top(void) {
/* check_heap_top() checks for large amounts of memory sbrkd at the top of 
   the heap but not allocated to nodes and returns it to the system. 
   This would only happen in a failed sbrk for a large amount of memory 
*/

    uintptr_t heap_dif; /* temp variable for saving difference between
                           heap_top and heap_cur */
    void *out; /* output from sbrk() */

    /* If the difference between the top of the heap and the
       current position in the heap is bigger than 2 block
       sizes, shrink the heap */
    if ((heap_top - heap_cur) > 2*BLK_SIZE) {
        if (debug) {
            myprint("Returning memory to system without node\n");
            print_list();
        }

        /* Shrink the heap by the difference of heap_top and heap_cur
           + a block */
        heap_dif = (uintptr_t) heap_top - (uintptr_t) heap_cur;
        if ((out = sbrk(-round_up(heap_dif 
                        - BLK_SIZE))) == (void *) -1) {

            /* If it fails to shrink, that's not an ENOMEM, 
               something else is wrong. Report and bail */
            perror("sbrk");
            exit(EXIT_FAILURE);
        }
        /* Adjust heap_top */
        heap_top = (void *) round_up((uintptr_t) heap_cur +
                                    BLK_SIZE);
    }
    if (debug) {
        print_list();
    }
}

void *alloc(size_t size) {
/* alloc() does the heavy lifting for the calloc(), malloc, and 
   realloc() functions. Given a size, it allocates memory on the heap,
   creates a header metadata, and returns a pointer to the beginning of
   the data.
*/

    header *ptr = NULL; /* Pointer to the header metadata */
    void *new_heap_bot = NULL; /* bottom of newly sbrkd memory */ 
    void *out_ptr = NULL; /*Pointer to the beggining of alloced memory */
    long unsigned int sbrk_counter; /* Counts sbrk loops to grow amount 
                                       sbrkd each call */


    /* Start by looking for an unallocated node big enough to use */
    ptr = head_list;
    myprint("Starting to run alloc\n");
    /* Loop through all nodes in the header list */
    while (ptr) {
        if (ptr->allocated == false && ptr->size >= size) {
            /* Found an appopriate node, use it */
            if (debug) {
                myprint("Found a free node to use\n");
            }
            /* Insert a new node with leftover space in the node */
            insert_node(ptr, size);
            /* Point out_ptr to the start of data, so as to not clobber
               the header metadata */
            out_ptr =  (void *) round_up((uintptr_t) ptr) +
               round_up( sizeof(header));
            return out_ptr;
        }
        /* If there is no next, ptr is end of list. Break and go on to next 
           section */
        if (!ptr->next) {
            break;
        }
        ptr = ptr->next;
    }
    myprint("Did not find an already created node, going to make one\n");
    

    /* If we got here, made it to the end of list without an appropriate
       node to reuse, need to make a new one.
    */
    
    sbrk_counter = 1; /* Counter used to slowly make sbrk hunks bigger 
                       */
    /* If the current heap location + size of hunk to be realloced is 
       greater than the top of the heap, sbrk needs to be called */
    while ((uintptr_t) heap_cur + 
            round_up(sizeof(header)) + round_up(size) > 
            (uintptr_t) heap_top) { 
        /* Need to call sbrk */
        if ((new_heap_bot = sbrk(BLK_SIZE*sbrk_counter)) == (void *) -1) {
            /* If sbrk fails, set errno to ENOMEM */
            errno = ENOMEM;
            /* check_heap_top() checks for large amounts of memory 
               sbrkd at the top of the heap but not allocated to nodes
               and returns it to the system. This would only happen in 
               a failed sbrk for a large amount of memory */
            check_heap_top();
            /* alloc failed, return NULL. User has to check errno if 
               they want to know what happened */
            return NULL;
        }
        myprint("Called sbrk\n");
        /* Sbrk was successful, adjust heap_top to use the new memory */
        heap_top = (void *) ((uintptr_t) new_heap_bot + 
                BLK_SIZE*sbrk_counter++);
    
    }

    /* Now, create a new node using ptr as the previous node.
       ptr should point to the end of the header list */ 
    out_ptr = create_node(ptr, size);
    /* create_node() returns a pointer to the beginning of usable memory 
       for the memory hunk 
    */
    return out_ptr;
}

void insert_node(header *hptr, size_t size) {
/* insert_node() takes a header and a size and splits the given
   node if its size is bigger than the size by 2 alignment blocks 
   (if it's smaller than that, there isn't room for a new node and 
   its data. An alignment block is the smallest unit that can be
   placed on the heap, a node can't take up less than 2 (1 for
   the header, and 1 data alignment block))
*/
    
    header *new_node; /* The new_node to be created from the given header */
    
    /* If the given hunk isn't big enough to split into 2, don't */
    if ((hptr->size - size) < round_up(sizeof(header)) + MALLOC_ALIGN) {
        hptr->allocated = true;
        return;
    }

    
    /* Now split the hunk into 2 */

    if (debug) {
        myprint("Splitting hunk\n");
    }

    /* Put the new_node after the ptr's data section */
    new_node = (void *) (round_up((uintptr_t) hptr) + round_up(size) + 
                                   round_up(sizeof(header)));

    /* Make the new_node's size the left over space */
    new_node->size = round_up(hptr->size) - round_up(size) - 
                     round_up(sizeof(header));

    /* Adjust hptr */
    hptr->size = round_up(size);
    hptr->allocated = true;

    /* Initialize new_ptr */
    new_node->allocated = false;
    strcpy(new_node->magic, MAGIC);
    new_node->next = NULL;
    new_node->prev = hptr;

    /* Place new_node right after hptr */
    if (hptr->next) {
        new_node->next = hptr->next;
        hptr->next->prev = new_node;
    }

    /* Otherwise, hptr was the end of the list, make new_node end_list */
    /*
    if (hptr == end_list) {
    */
    else {
       end_list = new_node; 
    }

    /* Change hptr->next to new_node*/
    hptr->next = new_node;

}

void *create_node(header *hptr, size_t size) {
/* create_node() creates a new node at the end of the header list with 
   the size given. It assumes hptr is the end of the list. If hptr is NULL,
   it assumes there is no list and starts one.
*/

    header *new_node; /* The new_node to be created from the given header*/ 
    char buf[50]; /* Print buffer */

    new_node = heap_cur; /* new_node is saved starting at heap_cur */ 
    /* Initialize new_node */
    new_node->size = round_up(size);
    new_node->allocated = true;
    strcpy(new_node->magic, MAGIC);
    new_node->next = NULL;
    /* If the node passed is NULL, new_node will be the only node */
    if (!hptr) {
        new_node->prev = NULL;
        head_list = new_node;
        end_list = new_node;
    }
    /* Otherwise, new_node is going in at the end of the list */
    else {
        hptr->next = new_node;
        new_node->prev = hptr;
        end_list = new_node;
    }

    /* Move the current heap ptr up the size of a header and 
       the size of the memory chunk. 
    */
    heap_cur = (void *) round_up((uintptr_t) heap_cur) + 
                        round_up(sizeof(header)) +
                        round_up(new_node->size);

    if (debug_verbose) {
        snprintf(buf, 25, "header: %p\n", new_node);
        fputs(buf, stderr);
        snprintf(buf, 28, "start of data: %p\n", 
                (void *) round_up((uintptr_t) new_node) + 
                         round_up(sizeof(header)));
        fputs(buf, stderr);
    }

    /* Now that the data structure is setup, return the ptr to 
       the data section so as to not clobber the header 
    */
    return (void *) round_up((uintptr_t) new_node) +
                    round_up(sizeof(header));

}

uintptr_t round_up(uintptr_t addr) {
/* Forces addr to be divisible by MALLOC_ALIGN */

    if (addr % MALLOC_ALIGN) {
        addr += (MALLOC_ALIGN - addr%MALLOC_ALIGN);
    }
    return addr;
}


void myprint(char *s) {
/* Simple print function that uses snprintf instead of printf to avoid 
   calls to malloc
*/

    char buf[100];
    if (debug_verbose) {
        snprintf(buf, strlen(s)+1, "%s", s);
        fputs(buf, stderr);
    }
}

void print_list(void) {
/* Prints the header list and each node's data members */

    header *tmp;
    char buf[100];
    tmp = head_list;

    if (!tmp) {
        myprint("No head of list :(\n");
    }
    if (debug_verbose) {
        while (tmp) {
            #ifdef x86
            snprintf(buf, 49, "size: %lu\n", tmp->size);
            #else
            snprintf(buf, 40, "size: %d\n", tmp->size);
            #endif

            fputs(buf, stderr);
            
            snprintf(buf, 24, "allocated: %d\n", tmp->allocated);
            fputs(buf, stderr);

            snprintf(buf, 35, "current: %p\n", tmp);
            fputs(buf, stderr);

            snprintf(buf, 35, "next: %p\n", tmp->next);
            fputs(buf, stderr);

            snprintf(buf, 35, "prev: %p\n\n", tmp->prev);
            fputs(buf, stderr);

            tmp = tmp->next;
        }
        myprint("\n");
        myprint("End list:\n");
        if (!end_list) {
            myprint("No end of list :(\n");
        }
        else {

            #ifdef x86
            snprintf(buf, 49, "size: %lu\n", end_list->size);
            #else
            snprintf(buf, 49, "size: %d\n", end_list->size);
            #endif

            fputs(buf, stderr);
            
            snprintf(buf, 24, "allocated: %d\n", end_list->allocated);
            fputs(buf, stderr);

            snprintf(buf, 30, "current: %p\n", end_list);
            fputs(buf, stderr);

            snprintf(buf, 18, "next: %p\n", end_list->next);
            fputs(buf, stderr);

            snprintf(buf, 18, "prev: %p\n\n", end_list->prev);
            fputs(buf, stderr);
        }
        myprint("\n");
    }

}


void print_debug(int kind, void *ptr, size_t total_size, size_t nmemb,
        size_t size, void *old_ptr) {
/* Each time an Alloc function is called: print what was called, 
   the parameters, and the result
*/
    char buf[160];
    char buf2[150];
    snprintf(buf, 9, "MALLOC: ");
    if (kind == MALLOC) {
        snprintf(buf+8, 17, "malloc");
        fputs(buf, stderr);
        #ifdef x86
        snprintf(buf2, 100, "(%lu)", total_size);
        #else
        snprintf(buf2, 100, "(%d)", total_size);
        #endif
        snprintf(buf, 160,"%-10s", buf2);
        fputs(buf, stderr);
        snprintf(buf, 50, "%14s   (ptr=%p, size", "=>", ptr);
        fputs(buf, stderr);
        #ifdef x86
        snprintf(buf2, 100, "=%lu)", round_up(total_size));
        #else
        snprintf(buf2, 100, "=%d)", round_up(total_size));
        #endif
        snprintf(buf, 161,"%-11s\n", buf2);
        fputs(buf, stderr);
    }
    else if (kind == FREE) {
        snprintf(buf+8, 25, "free(%p)\n", ptr);
        fputs(buf, stderr);
    }
    else if (kind == CALLOC) {
        /*
        snprintf(buf+8, 50, "calloc(%8d, %8d)%12s   (ptr=%p, size=%8d)\n",
                nmemb, size, "=>", ptr, total_size);
                */
        snprintf(buf+8, 20, "calloc");
        fputs(buf, stderr);
        #ifdef x86
        snprintf(buf2, 100, "(%lu,", nmemb);
        #else
        snprintf(buf2, 100, "(%d,", nmemb);
        #endif
        snprintf(buf, 160,"%5s ", buf2);
        fputs(buf, stderr);
        #ifdef x86
        snprintf(buf2, 100, "%lu)", size);
        #else
        snprintf(buf2, 100, "%d)", size);
        #endif
        snprintf(buf, 160,"%-10s", buf2);
        fputs(buf, stderr);
        snprintf(buf, 50, "%8s    (ptr=%p, size", "=>", ptr);
        fputs(buf, stderr);
        #ifdef x86
        snprintf(buf2, 100, "=%lu)", round_up(total_size));
        #else
        snprintf(buf2, 100, "=%d)", round_up(total_size));
        #endif
        snprintf(buf, 160,"%-9s\n", buf2);
        fputs(buf, stderr);

    }
    else if (kind == REALLOC) {
        snprintf(buf+8, 77, "realloc(%p, ", old_ptr);
        fputs(buf, stderr);

        #ifdef x86
        snprintf(buf2, 100, "%lu)", total_size);
        #else
        snprintf(buf2, 100, "%d)", total_size);
        #endif
        snprintf(buf, 160,"%-10s", buf2);
        fputs(buf, stderr);

        snprintf(buf, 40, "%2s   (ptr=%p, size", "=>", ptr);
        fputs(buf, stderr);

        #ifdef x86
        snprintf(buf2, 100, "=%lu)", round_up(total_size));
        #else
        snprintf(buf2, 100, "=%d)", round_up(total_size));
        #endif
        snprintf(buf, 160,"%-10s\n", buf2);
        fputs(buf, stderr);
    }

    
    return;
}

