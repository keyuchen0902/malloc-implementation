#include "my_malloc.h"
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#define NodeSize sizeof(struct my_node)
pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
__thread Node * head_nolock = NULL;
__thread Node * tail_nolock = NULL;

void merge(Node* node,Node* nextN){
    if(node == NULL) return;

    if(nextN!=NULL){
        if((void *)node + node->datasize + NodeSize != (void *)nextN) return;
        if(nextN->next!=NULL){
            nextN->next->prev = node;
        }
        node->next = nextN->next;
        node->datasize += NodeSize + nextN->datasize;
    }
}

void InsertToFreeList(Node* node,Node** head,Node **tail){
    Node* findAdd = *head;
    if(findAdd == NULL){
        *head = node;
        *tail = node;
        return;
    }

    while(findAdd != NULL){
        if(findAdd >= node){
            break;
        }
        findAdd = findAdd->next;
    }

    if(findAdd != *head && findAdd != NULL){//insert into freelist
        node->next = findAdd;
        node->prev = findAdd->prev;
        node->prev->next = node;
        findAdd->prev = node;
    }else if(findAdd == *head){//insert into the front of freelist
            node->next = findAdd;
            findAdd->prev = node;
            *head = node;
    }else if(findAdd == NULL){//insert into the end of freelist
            (*tail)->next = node;
            node->prev = *tail;
            *tail = node;
    }
    

}
Node* find_free_node_bf(Node* node,size_t size,Node** head){//this function is used to find a free node whose datasize is closest to the required size.
    node = *head;
    Node* ans=NULL;
    size_t min = ULONG_MAX;
    while (node!=NULL)
    {
        if(node->datasize == size){
            ans = node;
            break;
        }
        if(node->datasize > size){
            if(node->datasize - size < min){
                min = node->datasize - size;
                ans = node;
            }
        }
        node = node->next;
    }
    if(ans == NULL){
        return NULL;
    }
    return ans;
}

void *split(Node* node,size_t size,Node** head, Node** tail,int flag){
        Node* newnode = (Node*)((void*)node+NodeSize+size);//create a new node to spilt the original
        newnode->next = node->next;
        newnode->prev = node->prev;
        newnode->datasize = node->datasize - NodeSize - size;
        if(newnode->next != NULL){
            newnode->next->prev = newnode;
        }else{
            *tail = newnode;
        }

        if(newnode->prev !=NULL){
            newnode->prev->next = newnode;
        }else{
            *head = newnode;
        }
        //the original node's datasize will be exactly same with size,delete the node
        //newnode->prev = node;
        node->datasize = size;
        node->prev = NULL;
        node->next = NULL;

        
        if(flag != 1){
            pthread_mutex_unlock(&lock);
        }
        return node+1;
}

void *removeNode(Node* node,size_t size,Node** head, Node** tail,int flag){
    if(node->prev == NULL){
        *head = node->next;
            }else{
                node->prev->next = node->next;
            }
    if(node->next==NULL){
        *tail = node->prev;
        }else{
            node->next->prev = node->prev;
            }
    node->prev = NULL;
    node->next = NULL;

    if(flag != 1){
        pthread_mutex_unlock(&lock);
        }
    return node+1;
}

void *ts_malloc_lock(size_t size){//this function is used to implement malloc
    pthread_mutex_lock(&lock);
    Node *node =NULL;
    node = find_free_node_bf(node,size,&head);
    
    if(node != NULL){//If find a free node
        if (node->datasize > NodeSize + size){
            return split(node,size,&head,&tail,0);
        }else{
            return removeNode(node,size,&head,&tail,0);
        }
        
        
    }else{//unable to find a free node => node=NULL
        node = (Node*)sbrk(size+NodeSize);
        node->next = NULL;
        node->datasize = size;
        node->prev = NULL;
    }
    pthread_mutex_unlock(&lock);
    return node + 1;
}


void ts_free_lock(void* ptr){
    pthread_mutex_lock(&lock);
    if(ptr == NULL){
        return;
    }
    Node *node = (Node*)(ptr-NodeSize);
    InsertToFreeList(node,&head,&tail);
    Node* prevN = node->prev;
    Node* nextN = node->next;
    if(prevN==NULL){
        merge(node,nextN);
    }else{
        merge(prevN,node);
        if(prevN->next != NULL){
            merge(prevN,nextN);
        }
    }
    pthread_mutex_unlock(&lock);
}


void *ts_malloc_nolock(size_t size){
    Node *node =NULL;
    node = find_free_node_bf(node,size,&head_nolock);
    
   if(node != NULL){//If find a free node
        if (node->datasize > NodeSize + size){
            return split(node,size,&head_nolock,&tail_nolock,1);
        }else{
            return removeNode(node,size,&head_nolock,&tail_nolock,1);
        }
        
    }else{//unable to find a free node => node=NULL
        pthread_mutex_lock(&lock);
        node = (Node*)sbrk(size+NodeSize);
        pthread_mutex_unlock(&lock);
        node->next = NULL;
        node->datasize = size;
        node->prev = NULL;
    }
    return node + 1;
}
void ts_free_nolock(void *ptr){
    if(ptr == NULL){
        return;
    }
    Node *node = (Node*)(ptr-NodeSize);
    InsertToFreeList(node,&head_nolock,&tail_nolock);
    Node* prevN = node->prev;
    Node* nextN = node->next;
    if(prevN==NULL){
        merge(node,nextN);
    }else{
        merge(prevN,node);
        if(prevN->next != NULL){
            merge(prevN,nextN);
        }
    }
}

