#include "malloc.h"

/* TODO: Preliminary tests are looking pretty good.
         Need to do:
            * Remove (#define) "Magic" numbers
            * More intensive tests
                * Pretty intensive already done, have to think about other 
                  tests that could break it
            * More commenting (and remove print statement commented out)
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
    void *ptr = NULL; /* Pointer to the hunk */

    /* Setup the heap and debug params */
    /* This might be able to be a macro ? */
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
    void *ptr = NULL; /* Pointer to the hunk */

    /* Setup the heap and debug params */
    /* This might be able to be a macro ? */
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
    header *hptr; /* The header of the hunk to free */
    void *tmp_ptr; /* Temporarily holds end of list for comparison */
    char buf[50]; /* Print buffer */

    /* If the pointer is NULL, nothing to do, just return */
    if (!ptr) { 
        return;
    }

    /* Find the header corresponding to this hunk */
    hptr = find_header(ptr);

    /* find_header returns NULL if the pointer wasn't allocated in the heap */
    if (!hptr) {
        snprintf(buf, 40, "free(): Invalid pointer \n");
        fputs(buf, stderr);
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
                               end_list->size + sizeof(header));

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
    
    /* Might not need the if (hptr) here, hptr being NULL handled above */
    merge(hptr);

    if (debug) {
        myprint("\nafter merge:\n");
        print_list();
    }

}

void *realloc(void *ptr, size_t size) {
    void *out_ptr; /* Pointer to the hunk */
    void *new_heap_bot; 
    int sbrk_counter; /* Counts sbrk loops to grow amount sbrkd each call */
    header *hptr; /* Header of the hunk to to realloc */
    char buf[50];

    /* Setup the heap and debug params */
    /* This might be able to be a macro ? */
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

    /* Findheader corresponding to this hunk */
    hptr = find_header(ptr);

    /* find_header returns NULL if the pointer wasn't allocated in the heap */
    if (!hptr) {
        snprintf(buf, 40, "realloc(): Invalid pointer\n");
        fputs(buf, stderr);
        exit(EXIT_FAILURE);
    }

    merge(hptr);

    /* If reallocing to a smaller size, shrink the hunk */
    if (hptr->size > size && hptr->size - size > 2*MALLOC_ALIGN ) {
        if (debug) {
            myprint("Shrinking node\n");
        }
        insert_node(hptr, size);
        if (debug) {
            print_debug(REALLOC, ptr, size, 0, 0, ptr);
            print_list();
        }
        return ptr;
    }

    /* Try in place expansion, first merge if possible.
       Even if in place expansion won't work, still defregmenting,
       which is good. Don't have to worry about multiple unallocated
       blocks being in a row, because free would merge those. Only can
       have 1 after the current allocated block.
    */



    /* If the node is the last in the list, just grab more memory 
       from the heap */
   if (!hptr->next) { 
        if (debug) {
            myprint("next is null\n");
        }

        sbrk_counter = 1;
        while ((uintptr_t) heap_cur + size > (uintptr_t) heap_top) { 
            /* Need to call sbrk */
            if ((new_heap_bot = sbrk(BLK_SIZE*sbrk_counter)) 
                    == (void *) -1) {

                errno = ENOMEM;
                if (debug) {
                    print_debug(REALLOC, ptr, size, 0, 0, ptr);
                }

                myprint("calling check heap top in realloc\n");
                check_heap_top();
                return NULL;
            }
            heap_top = (void *) ((uintptr_t) new_heap_bot 
                    + BLK_SIZE*sbrk_counter++);
        }

        heap_cur = (void *) round_up((uintptr_t) heap_cur) +
                            round_up(size) - round_up(hptr->size);
        hptr->size = round_up(size);

        if (debug) {
            myprint("Doing an in place expansion using more heap\n");
            print_list();
            print_debug(REALLOC, ptr, size, 0, 0, ptr);
        }

        return ptr;

    }

    /* Else, need to copy to new hunk */
    /* First, allocate a new hunk */
    if (!(out_ptr = alloc(size))) {
        return NULL;
    }

    /* Now copy */
    memcpy(out_ptr, ptr, hptr->size);
    /* Free the old hunk */
    hptr->allocated = 0;
    /* Now there's a free node, merge it */
    merge(hptr);

    if (debug) {
        print_debug(REALLOC, out_ptr, size, 0, 0, ptr);
        myprint("Making a new node and copying\n");
        print_list();
    }
    return out_ptr;
}

header *find_header(void *ptr) {
    header *hptr;
    hptr = head_list;
    while(hptr) {
        /* If the address of the pointer is smaller than the address of 
           the header + the size of the hunk, then it must be in this hunk
       */
        if ((uintptr_t) ptr < (uintptr_t) hptr + hptr->size + sizeof(header)) {
            return hptr;
        }
        hptr = hptr->next;
    }
    return NULL;
}

void merge(header *hptr) {
    if (hptr->next && !hptr->next->allocated) {
        hptr->size += hptr->next->size + round_up(sizeof(header));
        remove_node(hptr->next);
    }
    /* Make sure hptr is unallocated before removing it.
       This is for calls to realloc 
    */
    if (!hptr->allocated && hptr->prev && !hptr->prev->allocated) {
        hptr->prev->size += hptr->size + round_up(sizeof(header));
        remove_node(hptr);
    }
}

void remove_node(header *hptr) {

    if (!hptr->next && hptr == head_list) {
        head_list = NULL;
        return;
    }
    if (hptr->prev) {
        hptr->prev->next = hptr->next;
    }
    if (hptr->next) {
        hptr->next->prev = hptr->prev;
    }
    else {
        end_list = hptr->prev;
    }
}


/* Sets up heap if need be, checks for debug */
bool setup() {
    char *debug_malloc;
    /* If the heap hasn't been allocated, allocate it */

    /* Check for debug */
    
    if ((debug_malloc = getenv("DEBUG_MALLOC"))) {
        debug = true;
        if (!strcmp(debug_malloc, "1")) {
            debug_verbose = true;
        }
    }
    
    if (!heap_bot) {
        if (debug_verbose) {
            myprint("Setup heap\n");
        }
        if ((heap_bot = sbrk(BLK_SIZE)) == (void *) -1) {
            /*
            perror("sbrk");
            */
            errno = ENOMEM;
            return -1;
        }
        heap_top = (void * ) (uintptr_t) heap_bot + BLK_SIZE;
        heap_cur = heap_bot;

    }
    
    return debug;
}

void check_heap_top(void) {
    uintptr_t heap_dif;
    void *out;
    if ((heap_top - heap_cur) > 2*BLK_SIZE) {
        if (debug) {
            myprint("Returning memory to system without node\n");
            print_list();
        }

        heap_dif = (uintptr_t) heap_top - (uintptr_t) heap_cur;
        if ((out = sbrk(-round_up(heap_dif 
                        - BLK_SIZE))) == (void *) -1) {

            perror("sbrk");
            exit(EXIT_FAILURE);
        }
        heap_top = (void *) round_up((uintptr_t) heap_cur +
                                    BLK_SIZE);
    }
    if (debug) {
        print_list();
    }
}

void *alloc(size_t size) {
    header *ptr = NULL;
    void *new_heap_bot = NULL;
    void *out_ptr = NULL;
    long unsigned int sbrk_counter;

    ptr = head_list;
    myprint("Starting to run alloc\n");
    while (ptr) {
        if (ptr->allocated == false && ptr->size >= size) {
            /* Found an appopriate node, use it */
            myprint("Found a free node to use\n");
            /*ptr->allocated = true;*/
            insert_node(ptr, size);
            out_ptr =  (void *) round_up((uintptr_t) ptr + sizeof(header));
            /*
            if (debug) {
                print_debug(MALLOC, out_ptr, size);
            }
            */
            return out_ptr;
        }
        if (!ptr->next) {
            break;
        }
        ptr = ptr->next;
    }
    myprint("Did not find an already created node, going to make one\n");
    
    /* If we got here, made it to the end of list without an appropriate
       node to reuse, need to make a new one.
    */
    
    /* Check if heap is large enough */
    /* Keep sbrk-ing blocks until it's big enough */
    sbrk_counter = 1;


    while ((uintptr_t) heap_cur + size > (uintptr_t) heap_top) { 
        /* Need to call sbrk */
        /*
        if (debug) {
        }
        */
        if ((new_heap_bot = sbrk(BLK_SIZE*sbrk_counter)) == (void *) -1) {
            errno = ENOMEM;
            check_heap_top();
            return NULL;
        }
        heap_top = (void *) ((uintptr_t) new_heap_bot + 
                BLK_SIZE*sbrk_counter++);
    
    }

    /* ptr should point to end of list */
    out_ptr = create_node(ptr, size);
    return out_ptr;
}

void insert_node(header *ptr, size_t size) {
    header *new_node;
    
    if ((ptr->size - size) < 2*MALLOC_ALIGN) {
        ptr->allocated = true;
        return;
    }

    if (debug) {
        myprint("Splitting hunk\n");
    }

    new_node = (void *) (round_up((uintptr_t) ptr) + round_up(size) + 
                                   round_up(sizeof(header)));
    new_node->size = round_up(ptr->size) - round_up(size) - 
                     round_up(sizeof(header));
    ptr->size = round_up(size);
    ptr->allocated = true;
    new_node->allocated = false;
    new_node->next = NULL;
    
    new_node->prev = ptr;
    if (ptr->next) {
        new_node->next = ptr->next;
        ptr->next->prev = new_node;
    }
    if (ptr == end_list) {
       end_list = new_node; 
    }


    ptr->next = new_node;

}

void *create_node(header *ptr, size_t size) {
    header *new_node;
    char buf[50];

    new_node = heap_cur;
    new_node->size = round_up(size);
    new_node->allocated = true;
    new_node->next = NULL;
    if (!ptr) {
        new_node->prev = NULL;
        head_list = new_node;
        end_list = new_node;
    }
    else {
        ptr->next = new_node;
        new_node->prev = ptr;
        end_list = new_node;
    }

    /* Move the current heap ptr up the size of a header and 
       the size of the memory chunk. Round to a value divisible by 16
    */
    heap_cur = (void *) round_up((uintptr_t) heap_cur) + 
                        round_up(sizeof(header)) +
                        round_up(new_node->size);

    /* Now that the data structure is setup, move the ptr to 
       the data section so as to not clobber the header 
    */
    /*
    if (debug && !strcmp(getenv("DEBUG_MALLOC"), "1")) {
    */
    if (debug_verbose) {
        snprintf(buf, 25, "header: %p\n", new_node);
        fputs(buf, stderr);
        snprintf(buf, 28, "start of data: %p\n", 
                (void *) round_up((uintptr_t) new_node) + 
                         round_up(sizeof(header)));
        fputs(buf, stderr);
    }
    return (void *) round_up((uintptr_t) new_node) +
                    round_up(sizeof(header));

}


void myprint(char *s) {
    char buf[100];
    /*
    if (debug && !strcmp(getenv("DEBUG_MALLOC"), "1")) {
    */
    if (debug_verbose) {
        snprintf(buf, strlen(s)+1, "%s", s);
        fputs(buf, stderr);
    }
}

void print_list(void) {
    header *tmp;
    char buf[100];
    tmp = head_list;

    /*
    debug = false;
    if (getenv("DEBUG_MALLOC")) {
        debug = true;
    }
    */
    if (!tmp) {
        myprint("No head of list :(\n");
    }
    /*
    if (debug && !strcmp(getenv("DEBUG_MALLOC"), "1")) {
    */
    if (debug_verbose) {
        while (tmp) {
            #ifdef x86
            snprintf(buf, 19, "size: %lu\n", tmp->size);
            #else
            snprintf(buf, 19, "size: %d\n", tmp->size);
            #endif

            fputs(buf, stderr);
            
            snprintf(buf, 24, "allocated: %d\n", tmp->allocated);
            fputs(buf, stderr);

            snprintf(buf, 30, "current: %p\n", tmp);
            fputs(buf, stderr);

            snprintf(buf, 18, "next: %p\n", tmp->next);
            fputs(buf, stderr);

            snprintf(buf, 18, "prev: %p\n\n", tmp->prev);
            fputs(buf, stderr);

            tmp = tmp->next;
        }
        myprint("\n");
        myprint("End list:\n");
            #ifdef x86
            snprintf(buf, 19, "size: %lu\n", end_list->size);
            #else
            snprintf(buf, 19, "size: %d\n", end_list->size);
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
        myprint("\n");
    }

}


uintptr_t round_up(uintptr_t addr) {
    if (addr % MALLOC_ALIGN) {
        addr += (MALLOC_ALIGN - addr%MALLOC_ALIGN);
    }
    return addr;
}

void print_debug(int kind, void *ptr, size_t total_size, size_t nmemb,
        size_t size, void *old_ptr) {
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

