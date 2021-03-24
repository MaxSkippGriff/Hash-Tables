/*
   Cuckoo hasing, which resolves the issue of hash collisions
   by using two hash tables. If both indexes are occupied, kick
   one index away from its nest, like the world's most evil
   bird, the cuckoo.
*/

#include "specific.h"
#include "../assoc.h"

/*
   Private functions:
*/

/* Creates copy of the 'original' string given as argument */
static char* clone_string(char* original);

/* Doubles the length of the internal arrays */
static void expand_assoc(assoc* assocs);

/* Two hashes required to overcome collisions */
static int hash_key_a(int keysize, void* key);

/* Two hashes required to overcome collisions */
static int hash_key_b(int keysize, void* key);
/*
   Inserts one key-value pair into the assocs object.
   Uses flag "count" to decide whether or not
   a call to this will affect assocs->count.
*/
static void insert_one(assoc* assocs, void* key, void* value,
                       int count_this);

/* Looks to see if equal key found */
static void* lookup_strings(assoc* assocs, char* key);

/* Looks to see if equal key found */
static void* lookup_general(assoc* assocs, void* key);

/* Fills memory with value and passes to insert_one */
static void expand_strings(assoc* assocs, void **old_values,
                           char **old_keys, int old_length);

/* Fills memory with value and passes to insert_one */
static void expand_general(assoc* assocs, void **old_values,
                           void *old_keys, int old_length);

/* Inserts one key-value pair into the assocs object. */
static void insert_string(assoc* assocs, char* key,
                          int index, int copy_this);

/* Inserts one key-value pair into the assocs object. */
static void insert_general(assoc* assocs, void* key, int index);

/* Passes key-value pairs to insert_string and insert_general */
static void insert_one_index(assoc* assocs, void* key, void* value,
                             int index, int count_this);
/*
   Function responsible for kicking other keys away
   until every key has its own 'nest'.
*/
static void kick_out(assoc* assocs, int index);

/* Function responsible for 'kicking' other keys away */
static void kick_out_general(assoc* assocs, int index_m);

/* Function responsible for 'kicking' other keys away */
static void kick_out_string(assoc* assocs, int index_m);

/*
    Removes repeated values but failed to work properly.
    I've kept it and commented it out to show that an
    attempt was made.
*/
/*static int repeated_insert(assoc* assocs, void *key,
                              void *value, int index);*/

/* Testing on the private functions */
void assoc_test();

assoc* assoc_init(int keysize)
{
    int full_keysize;
    assoc* assocs;

    assocs = malloc(sizeof(*assocs));
    assocs->keysize = keysize;
    assocs->length = SIZE;
    assocs->count = 0;

    if(keysize == 0){ /* Special case of (char *) */
        full_keysize = sizeof(char *) * assocs->length;
    }
    else{
        full_keysize = keysize * assocs->length;
    }
    assocs->keys = malloc(full_keysize);
    memset(assocs->keys, 0, full_keysize);

    assocs->values = malloc(sizeof(void *) * assocs->length);
    memset(assocs->values, 0, sizeof(*assocs->values) * assocs->length);

    return assocs;
}

void assoc_insert(assoc** a, void* key, void* data)
{
    assoc* assocs;
    assocs = *a;
    /* If the array is 25% filled, resize it (log2(16))*/
    if(assocs->count == assocs->length / RESIZE){
        expand_assoc(assocs);
    }
    /* count_this = true */
    insert_one(assocs, key, data, true);
}

unsigned int assoc_count(assoc* assocs)
{
    return assocs->count;
}

static void* lookup_strings(assoc* assocs, char* key)
{
    char **string_keys;
    int index_a, index_b;

    string_keys = (char **) assocs->keys;
    index_a = hash_key_a(assocs->keysize, key) % assocs->length;

    if(string_keys[index_a] != NULL){
        if(strcmp(string_keys[index_a], key) == 0){
            return assocs->values[index_a];
        }
    }
    index_b = hash_key_b(assocs->keysize, key) % assocs->length;

    if(string_keys[index_b] != NULL){
        if(strcmp(string_keys[index_b], key) == 0){
            return assocs->values[index_b];
        }
    }
    return NULL;
}

static void* lookup_general(assoc* assocs, void* key)
{
    char *buffer;
    int memory_index, index_a, index_b, comparison;

    buffer = (char *) assocs->keys;
    index_a = hash_key_a(assocs->keysize, key) % assocs->length;

    memory_index = index_a * assocs->keysize;
    comparison = memcmp(&buffer[memory_index], key, assocs->keysize);

    if(comparison == 0){
        return assocs->values[index_a];
    }

    index_b = hash_key_b(assocs->keysize, key) % assocs->length;
    memory_index = index_b * assocs->keysize;

    comparison = memcmp(&buffer[memory_index], key, assocs->keysize);

    if(comparison == 0){
        return assocs->values[index_b];
    }

    return NULL;
}

void* assoc_lookup(assoc* assocs, void* key)
{
    if(assocs->keysize == 0){
        return lookup_strings(assocs, (char *) key);
    }
    else{
        return lookup_general(assocs, key);
    }
}

void assoc_free(assoc* assocs)
{
    int index;
    char **string_keys;

    if(assocs->keysize == 0){
        string_keys = (char **) assocs->keys;

        for(index = 0; index < assocs->length; index += 1){
            if (string_keys[index] != NULL){
                free(string_keys[index]);
            }
        }
    }
    free(assocs->keys);
    free(assocs->values);
    free(assocs);
}

static char* clone_string(char* original)
{
    char *clone;
    int length;

    length = strlen(original) + ADDONE;
    clone = malloc(length);

    strcpy(clone, original);

    return clone;
}

static void expand_strings(assoc* assocs, void **old_values,
                                 char **old_keys, int old_length)
{
    int index;
    memset(assocs->keys, 0, sizeof(char *) * assocs->length);

    for(index = 0; index < old_length; index += 1){
        if(old_values[index] != NULL){
            insert_one(
                assocs,
                old_keys[index],
                old_values[index],
                /* Count_this = false */
                false
            );
        }
    }
}

static void expand_general(assoc* assocs, void **old_values,
                                 void *old_keys, int old_length)
{
    char *buffer;
    int index, memory_index, full_keysize;

    memset(assocs->keys, 0, assocs->keysize * assocs->length);
    full_keysize = assocs->keysize * assocs->length;
    memset(assocs->keys, 0, full_keysize);

    buffer = (char *) old_keys;

    for(index = 0; index < old_length; index += 1){
        if(old_values[index] != NULL){
            memory_index = index * assocs->keysize;
            insert_one(
                assocs,
                &buffer[memory_index],
                old_values[index],
                /* Count_this = false */
                false
            );
        }
    }
}
/*
   If 50% of the spaces are guaranteed to be empty,
   there's ~50% chance of no hash collisions when
   trying to insert an element.
*/
static void expand_assoc(assoc* assocs)
{
    void **old_values;
    void *old_keys;
    int old_length;

    old_values = assocs->values;
    old_keys = assocs->keys;
    old_length = assocs->length;

    assocs->length = assocs->length * 2;
    assocs->values = malloc(sizeof(*assocs->values) * assocs->length);
    memset(assocs->values, 0, sizeof(*assocs->values) * assocs->length);

    if(assocs->keysize == 0){
        assocs->keys = malloc(sizeof(char *) * assocs->length);
        expand_strings(assocs, old_values, (char **) old_keys, old_length);
    }
    else{
        assocs->keys = malloc(assocs->keysize * assocs->length);
        expand_general(assocs, old_values, old_keys, old_length);
    }
    free(old_values);
    free(old_keys);
}

static int hash_key_a(int keysize, void* key)
{
    unsigned char* buffer;
    int hash_a, hash_b, hash_c, index;
    unsigned long value_a, value_b;

    if(keysize == 0){
        keysize = strlen((char *) key);
    }
    hash_a = HASHVALUE; hash_b = 0; keysize -= keysize & 1;

    buffer = (unsigned char *) key;
    for(index = 0; index < keysize; index += 1){
        value_a = ((unsigned int) buffer[index]) + ADDBUFFER;
        value_b = ((int) buffer[index]);
        hash_a *= value_a;
        hash_b -= value_b;
    }
    hash_c = (hash_a - hash_b);
    return hash_c < 0 ? -hash_c : hash_c;
}

static int hash_key_b(int keysize, void* key)
{
    char* buffer;
    int hash_a, hash_b, hash_c, index;
    unsigned long value_a, value_b;

    if(keysize == 0){
        keysize = strlen((char *) key);
    }
    hash_a = HASHVALUE; hash_b = 0; keysize -= keysize & HASHVALUE;

    buffer = (char *) key;
    for(index = 0; index < keysize; index += 2){
        value_a = (int) (buffer[index + 0] ^ buffer[index + ADDONE]);
        value_b = (int) (buffer[index + 0] - buffer[index + ADDONE]);
        hash_a *= value_a;
        hash_b -= value_b;
    }
    hash_c = (hash_a ^ hash_b);
    return hash_c < 0 ? -hash_c : hash_c;
}

static void insert_string(assoc* assocs, char* key,
                              int index, int copy_this)
{
    char **string_keys;
    string_keys = (char **) assocs->keys;

    if(copy_this){
        string_keys[index] = clone_string(key);
    }
    else{
        string_keys[index] = key;
    }
}

static void insert_general(assoc* assocs, void* key, int index)
{
    char *buffer;
    int memory_index;

    buffer = (char *) assocs->keys;
    memory_index = index * assocs->keysize;

    memcpy(&buffer[memory_index], key, assocs->keysize);
}

static void insert_one_index(assoc* assocs, void* key,
                             void* value, int index, int count_this)
{
    if(count_this){
        assocs->count += ADDONE;
    }
    assocs->values[index] = value;

    if(assocs->keysize == 0){
        insert_string(assocs, key, index, count_this);
    }
    else{
        insert_general(assocs, key, index);
    }
}
/*
   "Count_this" is also used in "expand_assoc", because when
   the array is expanded,  everything is hashed again in the
   newer arrays. These insertions in "expand_assoc" should
   not be counted.
*/
static void insert_one(assoc* assocs, void* key, void* value,
                       int count_this)
{
    int index_a, index_b;
    index_a = hash_key_a(assocs->keysize, key) % assocs->length;

    if(assocs->values[index_a] == NULL){
        insert_one_index(assocs, key, value, index_a, count_this);
        return;
    }

    index_b = hash_key_b(assocs->keysize, key) % assocs->length;

    if(assocs->values[index_b] == NULL){
        insert_one_index(assocs, key, value, index_b, count_this);
        return;
    }
    /* At this point, both indexes are occupied. Kick one
       index away from its nest, like a cuckoo does */
    kick_out(assocs, index_b);
    insert_one_index(assocs, key, value, index_b, count_this);
}

static void kick_out_string(assoc* assocs, int index_m)
{
    char **string_keys;
    int index_a, index_b, index_n;

    string_keys = (char **) assocs->keys;

    index_a = hash_key_a(assocs->keysize,
                         string_keys[index_m]) % assocs->length;
    index_b = hash_key_b(assocs->keysize,
                         string_keys[index_m]) % assocs->length;

    index_n = index_a == index_m ? index_b : index_a;
    /* The other index is also occupied! */
    if(assocs->values[index_n] != NULL){
    /* Commenting this out leads to a memory leak. This
       was leading to an infinite but not anymore.
       kick_out_string(assocs, index_n) */
    }
    /* Kicks the value out of its nest into somewhere else */
    string_keys[index_n] = string_keys[index_m];
    assocs->values[index_n] = assocs->values[index_m];

    string_keys[index_m] = NULL;
    assocs->values[index_m] = NULL;
}

static void kick_out_general(assoc* assocs, int index_m)
{
    char *buffer;
    int index_a, index_b, index_n, memory_index_m, memory_index_n;

    buffer = (char *) assocs->keys;
    memory_index_m = assocs->keysize * index_m;

    index_a = hash_key_a(assocs->keysize,
                         &buffer[memory_index_m]) % assocs->length;
    index_b = hash_key_b(assocs->keysize,
                         &buffer[memory_index_m]) % assocs->length;

    index_n = index_a == index_m ? index_b : index_a;
    memory_index_n = assocs->keysize * index_n;
    /* The other index is also occupied */
    if(assocs->values[index_n] != NULL){
        kick_out_general(assocs, index_n);
    }
    /* Kicks the value out of its nest into somewhere else */
    memcpy(&buffer[memory_index_n], &buffer[memory_index_m],
           assocs->keysize);
    assocs->values[index_n] = assocs->values[index_m];

    memset(&buffer[memory_index_m], 0, assocs->keysize);
    assocs->values[index_m] = NULL;
}

static void kick_out(assoc *assocs, int index)
{
    if(assocs->keysize == 0){
        kick_out_string(assocs, index);
    }
    else{
        kick_out_general(assocs, index);
    }
}
/*
static int repeated_insert(assoc* assocs, void *key,
                           void *value, int index)
{
    char *buffer;
    int memory_index;

    buffer = (char *) assocs->keys;
    memory_index = index * assocs->keysize;

    if(memcmp(&buffer[memory_index], key, assocs->keysize) == 0){
        assocs->values[index] = value;
        return true;
    }
    return false;
}*/

/* Testing on clone_string, insert_one and expand_assoc */

void assoc_test()
{
    assoc* assocs;
    char *string_a, *string_b, *string_c;
    char *string_d, *string_e, *string_f;
    char *string_g, *string_h, *string_i;
    char *string_j, *string_k, *string_l;
    char *string_m, *string_n, *string_o;
    char *string_p, *string_q, *string_r;
    char *string_s, *string_t, *string_v;

    int old_length;

    string_a = clone_string("abc");
    string_b = clone_string("def");
    string_c = clone_string("ghi");
    string_d = clone_string("jkl");
    string_e = clone_string("mno");
    string_f = clone_string("pqr");
    string_g = clone_string("stv");
    string_h = clone_string("uwx");
    string_i = clone_string("dance");
    string_j = clone_string("woodland");
    string_k = clone_string("wizard");
    string_l = clone_string("lizard");
    string_m = clone_string("fiver");
    string_n = clone_string("household");
    string_o = clone_string("lockdown");
    string_p = clone_string("jumping");
    string_q = clone_string("turkey");
    string_r = clone_string("fireplace");
    string_s = clone_string("snowman");
    string_t = clone_string("dancing");
    string_v = clone_string("whiskey");

    assert(strcmp(string_a, "abc") == 0);
    assert(strcmp(string_b, "def") == 0);
    assert(strcmp(string_c, "ghi") == 0);
    assert(strcmp(string_d, "jkl") == 0);
    assert(strcmp(string_e, "mno") == 0);
    assert(strcmp(string_f, "pqr") == 0);
    assert(strcmp(string_g, "stv") == 0);
    assert(strcmp(string_h, "uwx") == 0);
    assert(strcmp(string_i, "dance") == 0);
    assert(strcmp(string_j, "woodland") == 0);
    assert(strcmp(string_k, "wizard") == 0);
    assert(strcmp(string_l, "lizard") == 0);
    assert(strcmp(string_m, "fiver") == 0);
    assert(strcmp(string_n, "household") == 0);
    assert(strcmp(string_o, "lockdown") == 0);
    assert(strcmp(string_p, "jumping") == 0);
    assert(strcmp(string_q, "turkey") == 0);
    assert(strcmp(string_r, "fireplace") == 0);
    assert(strcmp(string_s, "snowman") == 0);
    assert(strcmp(string_t, "dancing") == 0);
    assert(strcmp(string_v, "whiskey") == 0);

    assocs = assoc_init(0);

    insert_one(assocs, string_a, NULL, 1);
    assert(assocs->count == 1);

    insert_one(assocs, string_b, NULL, 1);
    assert(assocs->count == 2);

    insert_one(assocs, string_c, NULL, 1);
    assert(assocs->count == 3);

    insert_one(assocs, string_d, NULL, 1);
    assert(assocs->count == 4);

    insert_one(assocs, string_e, NULL, 1);
    assert(assocs->count == 5);

    insert_one(assocs, string_f, NULL, 1);
    assert(assocs->count == 6);

    insert_one(assocs, string_g, NULL, 1);
    assert(assocs->count == 7);

    insert_one(assocs, string_h, NULL, 1);
    assert(assocs->count == 8);

    insert_one(assocs, string_i, NULL, 1);
    assert(assocs->count == 9);

    insert_one(assocs, string_j, NULL, 1);
    assert(assocs->count == 10);

    insert_one(assocs, string_k, NULL, 1);
    assert(assocs->count == 11);

    insert_one(assocs, string_l, NULL, 1);
    assert(assocs->count == 12);

    insert_one(assocs, string_m, NULL, 1);
    assert(assocs->count == 13);

    insert_one(assocs, string_n, NULL, 1);
    assert(assocs->count == 14);

    insert_one(assocs, string_o, NULL, 1);
    assert(assocs->count == 15);

    insert_one(assocs, string_p, NULL, 1);
    assert(assocs->count == 16);

    insert_one(assocs, string_q, NULL, 1);
    assert(assocs->count == 17);

    insert_one(assocs, string_r, NULL, 1);
    assert(assocs->count == 18);

    insert_one(assocs, string_s, NULL, 1);
    assert(assocs->count == 19);

    insert_one(assocs, string_t, NULL, 1);
    assert(assocs->count == 20);

    insert_one(assocs, string_v, NULL, 1);
    assert(assocs->count == 21);

    old_length = assocs->length;
    expand_assoc(assocs);

    assert(assocs->length == old_length * 2);
    assert(assocs->length != old_length * 4);
    assert(assocs->length != old_length * 1);
    assert(assocs->length != old_length * 5);
    assert(assocs->length != old_length * 3);
    assert(assocs->length != old_length * 10);

    free(string_a);
    free(string_b);
    free(string_c);
    free(string_d);
    free(string_e);
    free(string_f);
    free(string_g);
    free(string_h);
    free(string_i);
    free(string_j);
    free(string_k);
    free(string_l);
    free(string_m);
    free(string_n);
    free(string_o);
    free(string_p);
    free(string_q);
    free(string_r);
    free(string_s);
    free(string_t);
    free(string_v);

    assoc_free(assocs);
}
