/**
 * @file        hashmap.h
 * @brief       Implementation of a HashMap.
*/

#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************************************/
/*                              CUSTOMIZABLE                             */
/*************************************************************************/

#include <stdlib.h>

/* Memory allocation function equivalent to malloc (3) or a malloc (3) wrapper. May not initialize the allocated zone. */
#define HASHMAP_MALLOC(size) malloc((size))

/* Memory allocation function equivalent to calloc (3) or a calloc (3) wrapper. Must initialize the allocated zone to 0. */
#define HASHMAP_CALLOC(qty, el_size) calloc((qty), (el_size))

/* Memory freeing function equivalent to free (3) or a free (3) wrapper. */
#define HASHMAP_FREE(ptr) free((ptr))

/* Data type used for the Keys. */
/* This type must be trivially copyable (via assignment). I.e. it cannot be an array type like char[], use char* instead. */
#define HASHMAP_KEY_DATA_TYPE char *

/* Data type used for the Values. */
/* This type must be trivially copyable (via assignment). I.e. it cannot be an array type like char[], use char* instead. */
#define HASHMAP_VAL_DATA_TYPE void *

/*************************************************************************/
/*                           NON - CUSTOMIZABLE                          */
/*************************************************************************/

/**
 * @typedef Data type of the Keys.
*/
typedef HASHMAP_KEY_DATA_TYPE HashMapKey;

/**
 * @typedef Data type of the stored Values.
*/
typedef HASHMAP_VAL_DATA_TYPE HashMapValue;

/**
 * @typedef HashMap main Abstract Data Type
*/
typedef struct _HashMap_t * HashMap;

/**
 * @typedef Hashing function callback
*/
typedef uint32_t (* HashMapHashFunction) (const HashMapKey key, const size_t hash_table_size);

/**
 * @typedef Callback function used to check if two HashMapKeys are equal
*/
typedef bool (* HashMapKeyEqualsCallback) (const HashMapKey key1, const HashMapKey key2);

/**
 * @typedef Callback function used to free the memory allocated for the Values when calling HashMap_cleanup
*/
typedef void (* HashMapCleanupCallback) (HashMapValue val);

/**
 * @enum Enumeration of possible errors that can occur when calling HashMap functions.
*/
typedef enum {
    HASHMAP_OK                  =  0,   // 0: No error
    HASHMAP_NO_MEMORY           = -1,   // 1: Not enough memory
    HASHMAP_DUPLICATED_KEY      = -2,   // 2: Key was already present when performing a put() operation
    HASHMAP_KEY_NOT_FOUND       = -3,   // 3: Key was not found when performing a peek() or pop() operation
    HASHMAP_INVALID_STATE       = -4,   // 4: HashMap is in an invalid state (it may not have been created successfully)
    HASHMAP_BAD_HASH_FUNCTION   = -5    // 5: The provided HashMapHashFunction returned an invalid value. This happens when
                                        //    said function returns a value that is greater or equal to the internal hash
                                        //    table size. Verify that your function uses the hash_table_size parameter and
                                        //    that the number it returns is never greater nor equal to that parameter.
} HashMapErrors;

/*************************************************************************/

/**
 * @brief       Initializes a new HashMap using the default hash table size (100).
 * 
 * @param[in]  hash_fn   Function used to perform hashing operations according to the HashMapKey data type.
 * @param[in]  equals_fn Function used to indicate whether 2 elements of type HashMapKey are equal or not.
 * 
 * @return      A new HashMap on success, NULL on failure (memory not available or NULL passed as a function).
 */
HashMap HashMap_create(HashMapHashFunction hash_fn, HashMapKeyEqualsCallback equals_fn);

/**
 * @brief       Initializes a new HashMap indicating a hash table size.
 * 
 * @param[in]  hash_fn   Function used to perform hashing operations according to the HashMapKey data type.
 * @param[in]  equals_fn Function used to indicate whether 2 elements of type HashMapKey are equal or not.
 * @param[in]  size      Size of the internal hash table. A bigger size typically improves performance, but also 
 *                       increases memory usage.
 * 
 * @return      A new HashMap on success, NULL on failure (memory not available or NULL passed as a function).
 */
HashMap HashMap_create_sized(HashMapHashFunction hash_fn, HashMapKeyEqualsCallback equals_fn, const size_t size);

/**
 * @brief       Insert a new (key, value) pair to the HashMap.
 * 
 * @param[in]  map The HashMap itself.
 * @param[in]  key Integer used as Key. Must be unique.
 * @param[in]  val The Value associated to the previous Key.
 * 
 * @return      Returns HASHMAP_OK on success, HASHMAP_NO_MEMORY if no memory was available
 *              to create the new (key, value) pair, or HASHMAP_DUPLICATED_KEY if the
 *              provided Key was already present in the HashMap.
 *              If an invalid or corrupted HashMap is provided, HASHMAP_INVALID_STATE is returned.
 *              If the provided HashMapHashFunction returns an invalid value, HASHMAP_BAD_HASH_FUNCTION is returned.
*/
HashMapErrors HashMap_put(HashMap const map, const HashMapKey key, const HashMapValue val);

/**
 * @brief       Get the Value associated to a Key.
 * 
 * @param[in]  map The HashMap itself.
 * @param[in]  key A key of type HashMapKey.
 * @param[out] val A pointer to HashMapValue. It is only modified if the provided Key is found.
 * 
 * @return      Returns HASHMAP_OK on success, or HASHMAP_KEY_NOT_FOUND if the HashMap
 *              does not contain the provided Key.
 *              If an invalid or corrupted HashMap is provided, HASHMAP_INVALID_STATE is returned.
 *              If the provided HashMapHashFunction returns an invalid value, HASHMAP_BAD_HASH_FUNCTION is returned.
*/
HashMapErrors HashMap_peek(HashMap const map, const HashMapKey key, HashMapValue * val);

/**
 * @brief       Get the Value associated to a Key, and remove the (key, value) pair from the HashMap.
 * 
 * @param[in]  map The HashMap itself.
 * @param[in]  key A key of type HashMapKey.
 * @param[out] val A pointer to HashMapValue. It is only modified if the provided Key is found.
 *                 Note that the caller is responsible for freeing values after a pop operation, as the HashMap will
 *                 no longer keep a reference to it.
 * 
 * @return      Returns HASHMAP_OK on success, or HASHMAP_KEY_NOT_FOUND if the HashMap
 *              does not contain the provided Key.
 *              If an invalid or corrupted HashMap is provided, HASHMAP_INVALID_STATE is returned.
 *              If the provided HashMapHashFunction returns an invalid value, HASHMAP_BAD_HASH_FUNCTION is returned.
*/
HashMapErrors HashMap_pop(HashMap const map, const HashMapKey key, HashMapValue * val);

/**
 * @brief       Check if a Key is present in the HashMap.
 * 
 * @param[in]  map The HashMap itself.
 * @param[in]  key A key of type HashMapKey.
 * 
 * @return      A boolean value indicating whether the Key is present or not.
 *              If an invalid or corrupted HashMap is provided, false is returned.
 *              If the provided HashMapHashFunction returns an invalid value, false is returned.
*/
bool HashMap_contains(HashMap const map, const HashMapKey key);

/**
 * @brief       Free all memory used by the HashMap and destroy it.
 * @details     After calling this function, @map must not be dereferenced again.
 *              If an invalid or corrupted HashMap is provided, any resources that may remain allocated are freed.
 * 
 * @param[in]  map The HashMap itself.
 * @param[in]  cb  A HashMapCleanupCallback function that is used to free the memory allocated for the Values.
 *                 If this parameter is NULL, no Value cleanup will be performed.
*/
void HashMap_cleanup(HashMap const map, HashMapCleanupCallback cb);

/**
 * @brief       Get how many elements are inside the HashMap.
 * 
 * @param[in]  map The HashMap itself.
 * 
 * @return      An integer indicating how many elements the HashMap holds inside. If the provided HashMap
 *              is NULL, HASHMAP_INVALID_STATE is returned.
*/
int32_t HashMap_size(HashMap const map);

#endif // __HASHMAP_H__
