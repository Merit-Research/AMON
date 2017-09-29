#include "bm_structs.h"


unsigned hash(flow_t sd){
    u_int32_t s;
    s = sd.src;
    const unsigned long long ulong_max_plus_one = (unsigned long long)4294967296;
    u_int32_t p = ipow( (unsigned long)(6700417) , 3) % ulong_max_plus_one;

    return ((s*p) % ulong_max_plus_one) % HASHSIZE;
}

struct nlist *lookup(flow_t sd){
    struct nlist *np;

    for (np = hashtab[hash(sd)]; np != NULL; np = np->next)
         if ((sd.src == np->key.src) &&   // sd.src == np->key.src && sd.dst == np->key.dst && ...
            (sd.dst == np->key.dst) &&
            (sd.sport == np->key.sport) &&
             (sd.dport == np->key.dport))
               return np; 


//	if (sd == np->key)
// 	    return np; /* found */
    return NULL;       /* not found */
}

unsigned int hashtab_read(flow_t  key){
    struct nlist *np;
    if ((np = lookup(key)) != NULL) /* found */
	return np->val;
    else{ /* not found */
	fprintf(stderr, "INFO: Key not found.\n");
	return 0;
    }
}

struct nlist *update(flow_t key, unsigned int val){ 
    struct nlist *np;
    unsigned hashval;

    if ((np = lookup(key)) == NULL){ /* not found */
	np = (struct nlist *) malloc(sizeof(*np));
        if (np == NULL)
	    return NULL;
	np->key = key;
    	np->val = val;
	hashval = hash(key);
	np->next = hashtab[hashval];
	hashtab[hashval] = np;
    } else  /* already there */
	np->val += val;
    return np;
}

struct nlist *insert(flow_t key, unsigned int val){ 
    struct nlist *np;
    unsigned hashval;

    if ((np = lookup(key)) == NULL){ /* not found */
	np = (struct nlist *) malloc(sizeof(*np));
        if (np == NULL)
	    return NULL;
	np->key = key;
    	np->val = val;
	hashval = hash(key);
	np->next = hashtab[hashval];
	hashtab[hashval] = np;
    } else{
	fprintf(stderr, "FATAL ERROR. Aborting. Key should not exist here.\n");
	exit(-1);
    } 
    return np;
}

struct nlist *replace(flow_t key, unsigned int val){ 
    struct nlist *np;

    if ((np = lookup(key)) != NULL){ /* already there */
	np->val = val;
    } else{
	fprintf(stderr, "FATAL ERROR. Aborting. Program tries to replace key that does not exist.\n");
	exit(-1);
    } 
    return np;
}

void init_hashtable(struct nlist **hashtab){
    int i;

    if (hashtab == NULL) return;
	
    /* Initialize all to NULL */
    for (i=0; i < HASHSIZE; i++){
        hashtab[i] = NULL;
    }
}

void free_hashtable(struct nlist **hashtab){
    int i;
    struct nlist *list, *temp;

    if (hashtab == NULL) return;
	
    /* Free the memory from every item in the table, including the structure elements */
    for (i=0; i < HASHSIZE; i++){
        list = hashtab[i];
	while (list != NULL){
	    temp = list;
	    list = list->next;
	    //printf("Freeing (%ud, %d)\n", temp->key, temp->val);
	    free(temp);
	}
     }	    
}

/* **************************************************** */

void max_heapify(flow_t *A, unsigned int i, int heapsize){
    unsigned int l = LEFT(i);
    unsigned int r = RIGHT(i);
    unsigned int largest = 0;
    flow_t temp;

    //fprintf(stderr, "Entered max_heapify. i=%u, LEFT=%u, RIGHT=%u, Heapsize-1=%d\n",i, l,r,heapsize-1);
    //if (heapsize==0) return;
    
    if (((int)l <= (int)heapsize-1) && (hashtab_read(A[l]) > hashtab_read(A[i])))
       	largest = l;
    else
        largest = i;

    if (((int)r <= (int)heapsize-1) && (hashtab_read(A[r]) > hashtab_read(A[largest])))
        largest = r;
    if (largest != i){
        temp = A[largest];
        A[largest] = A[i];
        A[i] = temp;
        max_heapify(A, largest, heapsize);
    }
}

void build_max_heap(flow_t *A, int n){
    int i;
    int heapsize = n;
    for (i = n/2; i >= 0; i--)
        max_heapify(A, (unsigned int) i, heapsize);
}

flow_t heap_maximum(flow_t *A){
    return A[0];
}

flow_t heap_extract_max(flow_t *A, int *heapsize_ptr){
    int heapsize = *heapsize_ptr;
    flow_t max = {0,0,0,0};
    if (heapsize < 1){
        fprintf(stderr, "ERROR: Heap undeflow!\n");
        return max;
    }
    max = A[0];
    A[0] = A[heapsize-1];
    heapsize -= 1;
    *heapsize_ptr = heapsize;
    max_heapify(A, (unsigned int) 0, heapsize); 
    return max;
}

void heap_increase_key(flow_t *A, unsigned int i, unsigned int val, int *heapsize_ptr){
    int heapsize = *heapsize_ptr;
         
    if ((i > heapsize) || ((int)i < 0)){
        fprintf(stderr, "FATAL ERROR: Index out of bounds\n");
	exit(-1);
    }

    if (val < hashtab_read(A[i])){
        fprintf(stderr, "FATAL ERROR: New key is smaller than current key\n");
	exit(-1);
    }
    else{
        flow_t temp;
	assert(replace(A[i], val) != NULL);
        while ((i > 0) && (hashtab_read(A[PARENT(i)]) < hashtab_read(A[i]))){
            temp = A[i];
            A[i] = A[PARENT(i)];
            A[PARENT(i)] = temp;
            i = PARENT(i);
        }
    }
}

void max_heap_insert(flow_t *A, flow_t key, unsigned int val, int *heapsize_ptr){
    *heapsize_ptr += 1;
    int heapsize = *heapsize_ptr;
    if (heapsize == HEAPSIZE){
        fprintf(stderr, "FATAL ERROR: Trying to increase beyond HEAPSIZE!\n");
        exit(-1);	
    }

    A[heapsize-1] = key;
    //printf("Inserting new...\n");
    assert(insert(key, 1) != NULL); /* inserting  dummy, low value */
    heap_increase_key(A, heapsize-1, val, heapsize_ptr);
}






