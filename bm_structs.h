#ifndef _BM_STRUCTS_H_
#define _BM_STRUCTS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PARENT(i) (((i-1)>>1)>0? ((i-1)>>1) :0)
#define LEFT(i) ((i<<1)+1)
#define RIGHT(i) ((i<<1)+2)
#define HEAPSIZE 16385 // BRICK_DIMENSION*BRICK_DIMENSION + 1
#define HASHSIZE 512


/* Declare the hash table associated with the priority queue*/
extern struct nlist *hashtab[HASHSIZE]; /* pointer table */

struct Flow{ /* expanded flow key */
 u_int32_t src;
 u_int32_t dst;
 u_int16_t sport;
 u_int16_t dport;
 };
 
typedef struct Flow flow_t;

struct nlist{ /* entry */
    struct nlist *next;
    flow_t key;
    unsigned int val;
};

unsigned hash(flow_t s);

unsigned long ipow(unsigned long base, int exp);

struct nlist *lookup(flow_t s);

unsigned int hashtab_read(flow_t key);

struct nlist *update(flow_t key, unsigned int val); 

struct nlist *insert(flow_t key, unsigned int val); 

struct nlist *replace(flow_t key, unsigned int val); 

void init_hashtable(struct nlist **hashtab);

void free_hashtable(struct nlist **hashtab);

/* **************************************************** */

void max_heapify(flow_t *A, unsigned int i, int heapsize);

void build_max_heap(flow_t *A, int n);

flow_t heap_maximum(flow_t *A);

flow_t heap_extract_max(flow_t *A, int *heapsize_ptr);

void heap_increase_key(flow_t *A, unsigned int i, unsigned int val, int *heapsize_ptr);

void max_heap_insert(flow_t *A, flow_t key, unsigned int val, int *heapsize_ptr);

#endif





