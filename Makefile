CC = gcc
CFLAGS = -Wall -g -fpic 
OBJS = malloc64.o malloc32.o\
	   tests/test.o tests/test_smallmalloc.o tests/test3.o\
	   tests/test_splitnode.o tests/test_calloc.o\
	   tests/test_merge.o tests/test_realloc.o tests/test_enomem.o\
		tests/test_freebadptr.o tests/test_nulls.o tests/test_reallocbadptr.o\
		tests/test_realloc2.o

PROGS = test test_smallmalloc test3 test_splitnode test_calloc test_merge\
		test_realloc test_enomem test_freebadptr test_nulls test_reallocbadptr\
		test_realloc2
LIBS = lib64/libmalloc.so lib/libmalloc.so lib64_s/libmalloc.a
LIB_PATH=~/Mymalloc/lib64

all: lib64/libmalloc.so lib64_s/libmalloc.a

tests: test test_smallmalloc test3 test_splitnode test_calloc test_merge\
	   test_realloc test_enomem test_freebadptr test_nulls test_reallocbadptr\
	   test_realloc2

test_s: lib64_s/libmalloc.a tests/test.o
	$(CC) -o test tests/test.o -Llib64_s -lmalloc
test: lib64/libmalloc.so tests/test.o
	$(CC) -L $(LIB_PATH) -o test tests/test.o -lmalloc
tests/test.o: tests/test.c
	$(CC) $(CFLAGS) -c -o tests/test.o tests/test.c

test_smallmalloc: lib64/libmalloc.so tests/test_smallmalloc.o
	$(CC) -L $(LIB_PATH) -o test_smallmalloc tests/test_smallmalloc.o\
	   	-lmalloc
tests/test_smallmalloc.o: tests/test_smallmalloc.c
	$(CC) $(CFLAGS) -c -o tests/test_smallmalloc.o tests/test_smallmalloc.c

test3: lib64/libmalloc.so tests/test3.o
	$(CC) -L $(LIB_PATH) -o test3 tests/test3.o -lmalloc
tests/test3.o: tests/test3.c
	$(CC) $(CFLAGS) -c -o tests/test3.o tests/test3.c

test_splitnode: lib64/libmalloc.so tests/test_splitnode.o
	$(CC) -L $(LIB_PATH) -o test_splitnode tests/test_splitnode.o -lmalloc
tests/test_splitnode.o: tests/test_splitnode.c
	$(CC) $(CFLAGS) -c -o tests/test_splitnode.o tests/test_splitnode.c

test_calloc: lib64/libmalloc.so tests/test_calloc.o
	$(CC) -L $(LIB_PATH) -o test_calloc tests/test_calloc.o -lmalloc
tests/test_calloc.o: tests/test_calloc.c
	$(CC) $(CFLAGS) -c -o tests/test_calloc.o tests/test_calloc.c

test_merge: lib64/libmalloc.so tests/test_merge.o
	$(CC) -L $(LIB_PATH) -o test_merge tests/test_merge.o -lmalloc
tests/test_merge.o: tests/test_merge.c
	$(CC) $(CFLAGS) -c -o tests/test_merge.o tests/test_merge.c

test_realloc: lib64/libmalloc.so tests/test_realloc.o
	$(CC) -L $(LIB_PATH) -o test_realloc tests/test_realloc.o -lmalloc
tests/test_realloc.o: tests/test_realloc.c
	$(CC) $(CFLAGS) -c -o tests/test_realloc.o tests/test_realloc.c

test_realloc2: lib64/libmalloc.so tests/test_realloc2.o
	$(CC) -L $(LIB_PATH) -o test_realloc2 tests/test_realloc2.o -lmalloc
tests/test_realloc2.o: tests/test_realloc2.c
	$(CC) $(CFLAGS) -c -o tests/test_realloc2.o tests/test_realloc2.c

test_enomem: lib64/libmalloc.so tests/test_enomem.o
	$(CC) -L $(LIB_PATH) -o test_enomem tests/test_enomem.o -lmalloc
tests/test_enomem.o: tests/test_enomem.c
	$(CC) $(CFLAGS) -c -o tests/test_enomem.o tests/test_enomem.c

test_freebadptr: lib64/libmalloc.so tests/test_freebadptr.o
	$(CC) -L $(LIB_PATH) -o test_freebadptr tests/test_freebadptr.o -lmalloc
tests/test_freebadptr.o: tests/test_freebadptr.c
	$(CC) $(CFLAGS) -c -o tests/test_freebadptr.o tests/test_freebadptr.c

test_nulls: lib64/libmalloc.so tests/test_nulls.o
	$(CC) -L $(LIB_PATH) -o test_nulls tests/test_nulls.o -lmalloc
tests/test_nulls.o: tests/test_nulls.c
	$(CC) $(CFLAGS) -c -o tests/test_nulls.o tests/test_nulls.c

test_reallocbadptr: lib64/libmalloc.so tests/test_reallocbadptr.o
	$(CC) -L $(LIB_PATH) -o test_reallocbadptr tests/test_reallocbadptr.o \
		-lmalloc
tests/test_reallocbadptr.o: tests/test_reallocbadptr.c
	$(CC) $(CFLAGS) -c -o tests/test_reallocbadptr.o tests/test_reallocbadptr.c

lib/libmalloc.so: lib malloc32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o

lib64/libmalloc.so: lib64 malloc64.o malloc.h
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o malloc.h

lib64_s/libmalloc.a: lib64_s malloc64.o malloc.h
	ar r $@ malloc64.o malloc.h
	ranlib $@

lib: 
	mkdir lib

lib64: 
	mkdir lib64

lib64_s:
	mkdir lib64_s

malloc32.o: malloc.c
	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	$(CC) $(CFLAGS) -c -o malloc64.o malloc.c 
clean: 
	rm -rf lib64 lib lib64_s $(LIBS) $(OBJS) $(PROGS) 

