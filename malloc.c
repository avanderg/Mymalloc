#include "malloc.h"

/* TODO: Preliminary tests are looking pretty good.
         Need to do:
            * check_heap_top() in calloc, malloc, realloc
            * More intensive tests
            * More commenting
            * README
*/

static void *heap_bot = NULL;
static void *heap_top = NULL;
static void *heap_cur = NULL;
static header *head_list = NULL;

void *calloc(size_t nmemb, size_t size) {
    void *outptr;
    bool debug;
    if ((debug = setup()) == -1) {
        return NULL;
    }
    if (!nmemb || !size) {
        if (debug) {
            myprint("Returning NULL\n", debug);
        }
        return NULL;
    }
    outptr = alloc(nmemb*size, debug);
    if (outptr) {
        memset(outptr, 0, nmemb*size);
    }
    if (debug) {
        print_debug(CALLOC, outptr, size*nmemb, nmemb, size, 0);
        myprint("\nprinting list:\n", debug);
        print_list();
    }
    myprint("calling check heap top in calloc\n", debug);
    check_heap_top(debug);
    return outptr;
    
}
void *malloc(size_t size) {
    bool debug;
    void *ptr = NULL;

    if ((debug = setup()) == -1) {
        return NULL;
    }
    myprint("calling malloc\n", debug);
    if (!size) {
        if (debug) {
            print_debug(MALLOC, ptr, size, 0, 0, 0);
        }
        return NULL;
    }

    ptr = alloc(size, debug);
    if (debug) {
        print_debug(MALLOC, ptr, size, 0, 0, 0);
    }
    /*
    myprint("\nprinting list:\n", debug);
    print_list();
    */
    myprint("calling check heap top in malloc\n", debug);
    check_heap_top(debug);
    return ptr;

}
void free(void *ptr) {
    header *hptr;
    bool debug = false;
    char buf[50];
    if (getenv("DEBUG_MALLOC")) {
        debug = true;
    }
    if (!ptr) {
        return;
    }

    hptr = find_header(ptr, debug);
    if (!hptr) {
        snprintf(buf, 40, "free(): Invalid pointer \n");
        fputs(buf, stderr);
        exit(EXIT_FAILURE);
    }
    if (!hptr->allocated) {
        snprintf(buf, 32, "free(): Double free on pointer\n");
        fputs(buf, stderr);
        exit(EXIT_FAILURE);
    }
    hptr->allocated = false;


    /* Check for debug */
    if (debug) {
        print_debug(FREE, ptr, 0, 0, 0, 0);
    }
    if (debug && !strcmp(getenv("DEBUG_MALLOC"), "1")) {
        snprintf(buf, 20, "Freeing: %p\n", hptr);
        fputs(buf, stderr);
    }

    /* Return memory to the system if there is a crap ton of memory
       at the top of the heap */
    myprint("Calling check_heap top in free\n", debug);
    check_heap_top(debug);

    /* Return memory to system if at end of list, and is a large chunk*/
    if (!hptr->next && hptr->size > BLK_SIZE) {
        remove_node(hptr);
        if (debug) {
            myprint("Returning memory to the system from node\n", debug);
        }
        if ((sbrk(-hptr->size)) == (void *) -1) {
            perror("sbrk");
            exit(EXIT_FAILURE);
        }
        heap_top -= hptr->size;
        heap_cur -= (hptr->size + sizeof(header));
        if (debug) {
        print_list();
        }
        return;
    }

    if (debug) {
        myprint("before merge:\n", debug);
        print_list(); 
    }
    if (hptr) {
        merge(hptr, debug);
    }
    if (debug) {
        myprint("\nafter merge:\n", debug);
        print_list();
    }

}

void *realloc(void *ptr, size_t size) {
    void *out_ptr;
    void *new_heap_bot;
    bool debug;
    int sbrk_counter;
    /*char buf[50];*/
    /*
    errno = 0;
    */
    header *hptr;
    if ((debug = setup()) == -1) {
        return NULL;
    }
    /*
    if (errno == ENOMEM) {
        return NULL;
    }
    */
    /* Return memory to the system if there is a crap ton of memory
       at the top of the heap */
    /*
    check_heap_top(debug);
    */


    /* If ptr passes is NULL, equivalent to calling malloc(size) */
    if (!ptr) {
        if (debug) {
            myprint("NULL ptr passed, doing a malloc\n", debug);
        }
        if (!size) {
            return NULL;
        }
        out_ptr = alloc(size, debug);
        if (debug) {
            print_debug(REALLOC, out_ptr, size, 0, 0, ptr);
            print_list();
        }
        myprint("calling check heap top in realloc1\n", debug);
        check_heap_top(debug);
        return out_ptr;
    }
    else if (!size) {
        if (debug) {
            myprint("0 size passed, calling free\n", debug);
        }
        free(ptr);
        return NULL;
    }

    hptr = find_header(ptr, debug);
    merge(hptr, debug);

    /* If reallocing to a smaller size, shrink the hunk */
    if (hptr->size > size && hptr->size - size > 32 ) {
        if (debug) {
            myprint("Shrinking node\n", debug);
        }
        insert_node(hptr, size, debug);
        /*
        out_ptr =  (void *) round_up((uintptr_t) ptr + sizeof(header));
        */
        if (debug) {
            print_debug(REALLOC, ptr, size, 0, 0, ptr);
            print_list();
        }
        myprint("calling check heap top in realloc2\n", debug);
        check_heap_top(debug);
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
            myprint("next is null\n", debug);
        }

        sbrk_counter = 1;
        while ((uintptr_t) heap_cur + size > (uintptr_t) heap_top) { 
            /* Need to call sbrk */
            if ((new_heap_bot = sbrk(BLK_SIZE*sbrk_counter++)) 
                    == (void *) -1) {
                /*
                perror("sbrk");
                */
                errno = ENOMEM;
                if (debug) {
                    print_debug(REALLOC, ptr, size, 0, 0, ptr);
                }
                myprint("calling check heap top in realloc3\n", debug);
                check_heap_top(debug);
                return NULL;
            }
            heap_top = (void *) ((uintptr_t) new_heap_bot 
                    + BLK_SIZE*sbrk_counter);
        }

        heap_cur = (void *) round_up((uintptr_t) heap_cur) +
                            round_up(size) - round_up(hptr->size);
        hptr->size = round_up(size);
        /*
        check_heap_top(debug, hptr->size);
        */
        if (debug) {
            myprint("Doing an in place expansion using more heap\n", debug);
            print_list();
            print_debug(REALLOC, ptr, size, 0, 0, ptr);
        }

        myprint("calling check heap top in realloc4\n", debug);
        check_heap_top(debug);
        return ptr;

    }

    /* Else, need to copy to new hunk */
    /* First, allocate a new hunk */
    if (!(out_ptr = alloc(size, debug))) {
        return NULL;
    }

    /* Now copy */
    memcpy(out_ptr, ptr, hptr->size);
    /* Free the old hunk */
    hptr->allocated = 0;
    /* Now there's a free node, merge it */
    merge(hptr, debug);

    if (debug) {
        print_debug(REALLOC, out_ptr, size, 0, 0, ptr);
        myprint("Making a new node and copying\n", debug);
        print_list();
    }
    check_heap_top(debug);
    return out_ptr;
}

header *find_header(void *ptr, bool debug) {
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

void merge(header *hptr, bool debug) {
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
}


/* Sets up heap if need be, checks for debug */
bool setup() {
    bool debug = false;
    /* If the heap hasn't been allocated, allocate it */

    /* Check for debug */
    if (getenv("DEBUG_MALLOC")) {
        debug = true;
    }
    
    if (!heap_bot) {
        if (debug) {
            myprint("Setup heap\n", debug);
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

void check_heap_top(bool debug) {
    char buf[50];
    snprintf(buf, 40, "heap_top: %p\n", heap_top);
    fputs(buf, stderr);
    snprintf(buf, 40, "heap_cur: %p\n", heap_cur);
    fputs(buf, stderr);
    if ((heap_top - heap_cur) > 2*BLK_SIZE) {
        if (debug) {
            myprint("Returning memory to system without node\n", debug);
            print_list();
        }
        if (sbrk(-round_up((-(uintptr_t) heap_top - 
                         (uintptr_t) heap_cur) 
                        - BLK_SIZE)) == (void *) -1) {

            perror("sbrk");
        }
        myprint("\nAltering heap_top\n", debug);
        snprintf(buf, 40, "heap_cur again: %p\n", heap_cur);
        fputs(buf, stderr);
        snprintf(buf, 40, "heap_top + BLK_SIZE: %p\n", 
                (void *) round_up((uintptr_t) heap_cur + BLK_SIZE));
        fputs(buf, stderr);
        heap_top = (void *) round_up((uintptr_t) heap_cur + BLK_SIZE);
    }
    snprintf(buf, 40, "heap_top after: %p\n", heap_top);
    fputs(buf, stderr);
}

void *alloc(size_t size, bool debug) {
    header *ptr = NULL;
    void *new_heap_bot = NULL;
    void *out_ptr = NULL;
    long unsigned int sbrk_counter;
    char buf[50];

    ptr = head_list;
    myprint("Starting to run alloc\n", debug);
    while (ptr) {
        if (ptr->allocated == false && ptr->size >= size) {
            /* Found an appopriate node, use it */
            myprint("Found a free node to use\n", debug);
            /*ptr->allocated = true;*/
            insert_node(ptr, size, debug);
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
    myprint("Did not find an already created node, going to make one\n",
            debug);
    
    /* If we got here, made it to the end of list without an appropriate
       node to reuse, need to make a new one.
    */
    
    /* Check if heap is large enough */
    /* Keep sbrk-ing blocks until it's big enough */
    sbrk_counter = 1;


    while ((uintptr_t) heap_cur + size > (uintptr_t) heap_top) { 
        /* Need to call sbrk */
        /*
        myprint("sbrk for more heap space\n", debug);
        */
        if (debug) {
            snprintf(buf, 40, "sbrk(%lu) for more heap space\n", 
                    BLK_SIZE*sbrk_counter);
            fputs(buf, stderr);
        }
        if ((new_heap_bot = sbrk(BLK_SIZE*sbrk_counter++)) == (void *) -1) {
            errno = ENOMEM;
            check_heap_top(debug);
            return NULL;
        }
        heap_top = (void *) ((uintptr_t) new_heap_bot + sbrk_counter*BLK_SIZE);
    
    }

    /* ptr should point to end of list */
    out_ptr = create_node(ptr, size, debug);
    return out_ptr;
}

void insert_node(header *ptr, size_t size, bool debug) {
    header *new_node;
    
    if ((ptr->size - size) < 32) {
        ptr->allocated = true;
        return;
    }
    myprint("Splitting hunk\n", debug);

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
    ptr->next = new_node;

}

void *create_node(header *ptr, size_t size, bool debug) {
    header *new_node;
    char buf[50];

    new_node = heap_cur;
    new_node->size = round_up(size);
    new_node->allocated = true;
    new_node->next = NULL;
    if (!ptr) {
        new_node->prev = NULL;
        head_list = new_node;
    }
    else {
        ptr->next = new_node;
        new_node->prev = ptr;
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
    if (debug && !strcmp(getenv("DEBUG_MALLOC"), "1")) {
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


void myprint(char *s, bool debug) {
    char buf[100];
    if (debug && !strcmp(getenv("DEBUG_MALLOC"), "1")) {
        snprintf(buf, strlen(s)+1, "%s", s);
        fputs(buf, stderr);
    }
}

void print_list(void) {
    header *tmp;
    char buf[100];
    bool debug;
    tmp = head_list;
    debug = false;
    if (getenv("DEBUG_MALLOC")) {
        debug = true;
    }
    if (!tmp) {
        myprint("No head of list :(\n", debug);
    }
    if (debug && !strcmp(getenv("DEBUG_MALLOC"), "1")) {
        while (tmp) {
            #ifdef x86
            snprintf(buf, 19, "size: %lu\n", tmp->size);
            #else
            snprintf(buf, 19, "size: %d\n", tmp->size);
            #endif

            fputs(buf, stderr);
            
            snprintf(buf, 24, "allocated: %d\n", tmp->allocated);
            fputs(buf, stderr);

            snprintf(buf, 20, "current: %p\n", tmp);
            fputs(buf, stderr);

            snprintf(buf, 18, "next: %p\n", tmp->next);
            fputs(buf, stderr);

            snprintf(buf, 18, "prev: %p\n\n", tmp->prev);
            fputs(buf, stderr);

            tmp = tmp->next;
        }
        myprint("\n", debug);
    }
}


uintptr_t round_up(uintptr_t addr) {
    if (addr % 16) {
        addr += (16 - addr%16);
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

