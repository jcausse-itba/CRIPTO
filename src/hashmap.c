/**
 * @file        hashmap.c
 * @brief       Implementation of a HashMap.
*/

#include "hashmap.h"

#define HASHMAP_DEFAULT_HASH_TABLE_SIZE 100

typedef struct _HashMap_Node_t{
    HashMapKey                  key;
    HashMapValue                val;
    struct _HashMap_Node_t *    next;
} _HashMap_Node_t;

typedef struct _HashMap_t{
    struct _HashMap_Node_t **   table;
    size_t                      table_size;
    HashMapHashFunction         hash_fn;
    HashMapKeyEqualsCallback    equals_fn;
    int32_t elem_count;
} _HashMap_t;

static HashMapErrors HashMap_find(HashMap map, HashMapKey key, HashMapValue * val, bool remove_on_found){
    // Find position in the hash table
    uint32_t position = map->hash_fn(key, map->table_size);
    if (position >= map->table_size){ // As hash_fn is customizable, check if the returned position is within the hash table bounds
        return HASHMAP_BAD_HASH_FUNCTION;
    }

    // Find the node inside the linked list for the calculated position in the hash table
    _HashMap_Node_t * previous_node = NULL;
    _HashMap_Node_t * current_node = map->table[position];
    while(current_node != NULL && ! map->equals_fn(current_node->key, key)){
        previous_node = current_node;
        current_node = current_node->next;
    }
    if (current_node == NULL){
        return HASHMAP_KEY_NOT_FOUND;
    }

    // Store the value
    if (val != NULL){
        * val = current_node->val;
    }
    
    // Remove the value if asked to do so
    if (remove_on_found){
        if (previous_node == NULL){ // The key was found in the first node of the linked list
            map->table[position] = current_node->next;
        }
        else{
            previous_node->next = current_node->next;
        }
        HASHMAP_FREE(current_node);
    }

    // Found!
    return HASHMAP_OK;
}

HashMap HashMap_create(HashMapHashFunction hash_fn, HashMapKeyEqualsCallback equals_fn){
    return HashMap_create_sized(hash_fn, equals_fn, HASHMAP_DEFAULT_HASH_TABLE_SIZE);
}

HashMap HashMap_create_sized(HashMapHashFunction hash_fn, HashMapKeyEqualsCallback equals_fn, const size_t table_size){
    // Check parameters
    if (hash_fn == NULL || equals_fn == NULL){
        return NULL;
    }

    // Create HashMap structure
    HashMap map = HASHMAP_MALLOC(sizeof(_HashMap_t));
    if (map  == NULL){
        return NULL;
    }
    map->table_size = table_size;
    map->hash_fn = hash_fn;
    map->equals_fn = equals_fn;
    map->elem_count = 0;
    
    // Create table
    map->table = HASHMAP_CALLOC(map->table_size, sizeof(_HashMap_Node_t *)); // Each element is initialized to 0 (NULL)
    if (map->table == NULL){
        HASHMAP_FREE(map);
        return NULL;
    }

    // HashMap created an initialized
    return map;
}

HashMapErrors HashMap_put(HashMap const map, const HashMapKey key, const HashMapValue val){
    if (map == NULL || map->table == NULL || map->hash_fn == NULL || map->equals_fn == NULL){
        return HASHMAP_INVALID_STATE;
    }
    // Find position in the hash table
    uint32_t position = map->hash_fn(key, map->table_size);
    if (position >= map->table_size){ // As hash_fn is customizable, check if the returned position is within the hash table bounds
        return HASHMAP_BAD_HASH_FUNCTION;
    }
    // Traverse the linked list for the calculated position
    _HashMap_Node_t * previous_node = NULL;
    _HashMap_Node_t * current_node = map->table[position];
    while (current_node != NULL){
        // Make sure that the provided key is not already present in the Hash Map
        if (map->equals_fn(current_node->key, key)){
            return HASHMAP_DUPLICATED_KEY;
        }
        previous_node = current_node;
        current_node = current_node->next;
    }
    // Create a new node
    _HashMap_Node_t * new_node = HASHMAP_MALLOC(sizeof(_HashMap_Node_t));
    if (new_node == NULL){
        return HASHMAP_NO_MEMORY;
    }
    new_node->key = key;
    new_node->val = val;
    new_node->next = NULL;
    // Insert the new node into the list
    if (previous_node == NULL){ // No node in that table position
        map->table[position] = new_node;
    }
    else {
        previous_node->next = new_node;
    }
    // Increment the size
    map->elem_count++;
    return HASHMAP_OK;
}

HashMapErrors HashMap_peek(HashMap const map, const HashMapKey key, HashMapValue * val){
    if (map == NULL || map->table == NULL || map->hash_fn == NULL || map->equals_fn == NULL){
        return HASHMAP_INVALID_STATE;
    }
    HashMapValue value;
    switch (HashMap_find(map, key, &value, false)){
        case HASHMAP_KEY_NOT_FOUND:
            return HASHMAP_KEY_NOT_FOUND;
        case HASHMAP_BAD_HASH_FUNCTION:
            return HASHMAP_BAD_HASH_FUNCTION;
        default:
            break;
    }
    if (val != NULL){
        * val = value;
    }
    return HASHMAP_OK;
}

HashMapErrors HashMap_pop(HashMap const map, const HashMapKey key, HashMapValue * val){
    if (map == NULL || map->table == NULL || map->hash_fn == NULL || map->equals_fn == NULL){
        return HASHMAP_INVALID_STATE;
    }
    HashMapValue value;
    switch (HashMap_find(map, key, &value, true)){
        case HASHMAP_KEY_NOT_FOUND:
            return HASHMAP_KEY_NOT_FOUND;
        case HASHMAP_BAD_HASH_FUNCTION:
            return HASHMAP_BAD_HASH_FUNCTION;
        default:
            break;
    }
    if (val != NULL){
        * val = value;
    }
    // Decrement the size
    map->elem_count--;
    return HASHMAP_OK;
}   

bool HashMap_contains(HashMap const map, const HashMapKey key){
    if (map == NULL || map->table == NULL || map->hash_fn == NULL || map->equals_fn == NULL){
        return false;
    }
    return HashMap_find(map, key, NULL, false) == HASHMAP_OK;
}

void HashMap_cleanup(HashMap const map, HashMapCleanupCallback cb){
    if (map == NULL){
        return;
    }
    if (map->table != NULL){
        for (unsigned int i = 0; i < map->table_size; i++){
            _HashMap_Node_t * current_node = map->table[i];
            _HashMap_Node_t * next_node = NULL;
            while (current_node != NULL){
                if (cb != NULL){
                    cb(current_node->val);
                }
                next_node = current_node->next;
                HASHMAP_FREE(current_node);
                current_node = next_node;
            }
        }
        HASHMAP_FREE(map->table);
    }
    HASHMAP_FREE(map);
}

int32_t HashMap_size(HashMap const map){
    if (map == NULL || map->table == NULL || map->hash_fn == NULL || map->equals_fn == NULL){
        return HASHMAP_INVALID_STATE;
    }
    return map->elem_count;
}
