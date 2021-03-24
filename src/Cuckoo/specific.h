#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>

/* Buffer for hashing */
#define ADDBUFFER 3
/* Resize is equivalent to log2(16) */
#define RESIZE 4
/* Comparison required for hashing */
#define COMPARE -1
/* Increase value by one */
#define ADDONE 1
/* Initial value for hash_a */
#define HASHVALUE 1
/*
   This pointer is used to indicate places that had
   elements, but are now vacant. Places that never
   had elements have NULL pointers.
*/
#define VACANT ((void *) -1)
/* The assignment requires this size */
#define SIZE 16

typedef enum bool {false, true} bool;
/* Structure for hashing */
typedef struct assoc {

    void **values;
    void *keys;

    int keysize;
    int length;
    int count;

} assoc;
