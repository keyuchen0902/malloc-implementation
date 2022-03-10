#ifndef my_malloc_h
#define my_malloc_h

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdbool.h>
#include<limits.h>

struct my_node
{
    size_t datasize;
    //bool free;
    struct my_node *prev;
    struct my_node *next;
;
};
typedef struct my_node Node;

//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

Node *head = NULL;
Node* tail = NULL;


#endif
