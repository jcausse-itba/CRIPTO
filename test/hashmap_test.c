#include "hashmap.h"
#include "custom_assert.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Standard string hash function (djb2 algorithm) */
static uint32_t string_hash(const HashMapKey key, const size_t hash_table_size) {
    if (key == nullptr || hash_table_size == 0) {
        return 0;
    }
    uint32_t hash = 5381;
    const unsigned char *str = (const unsigned char *)key;
    while (*str) {
        hash = ((hash << 5) + hash) + *str; /* hash * 33 + c */
        str++;
    }
    return hash % (uint32_t)hash_table_size;
}

/* Key equality callback for string keys */
static bool string_equals(const HashMapKey key1, const HashMapKey key2) {
    if (key1 == key2) {
        return true;
    }
    if (key1 == nullptr || key2 == nullptr) {
        return false;
    }
    return strcmp(key1, key2) == 0;
}

/* Hash function that always returns 0 to force collisions */
static uint32_t colliding_hash(const HashMapKey key, const size_t hash_table_size) {
    (void)key;
    (void)hash_table_size;
    return 0;
}

/* Hash function that returns an invalid index (>= hash_table_size) */
static uint32_t bad_hash(const HashMapKey key, const size_t hash_table_size) {
    (void)key;
    return (uint32_t)hash_table_size;
}

/* 1. Test creation with NULL callbacks and cleanup */
static void test_creation_and_cleanup() {
    /* Creation with NULL callbacks must fail */
    ASSERT_MSG(HashMap_create(nullptr, string_equals) == nullptr,
               "HashMap_create should fail when hash_fn is NULL");
    ASSERT_MSG(HashMap_create(string_hash, nullptr) == nullptr,
               "HashMap_create should fail when equals_fn is NULL");
    ASSERT_MSG(HashMap_create(nullptr, nullptr) == nullptr,
               "HashMap_create should fail when both callbacks are NULL");

    ASSERT_MSG(HashMap_create_sized(nullptr, string_equals, 10) == nullptr,
               "HashMap_create_sized should fail when hash_fn is NULL");
    ASSERT_MSG(HashMap_create_sized(string_hash, nullptr, 10) == nullptr,
               "HashMap_create_sized should fail when equals_fn is NULL");
    ASSERT_MSG(HashMap_create_sized(nullptr, nullptr, 10) == nullptr,
               "HashMap_create_sized should fail when both callbacks are NULL");

    /* Creation with valid callbacks should succeed */
    HashMap map = HashMap_create(string_hash, string_equals);
    ASSERT_MSG(map != nullptr, "HashMap_create failed with valid callbacks");
    ASSERT_MSG(HashMap_size(map) == 0, "Initial size of hashmap should be 0");
    HashMap_cleanup(map, nullptr);

    /* Sized creation should also succeed */
    HashMap sized_map = HashMap_create_sized(string_hash, string_equals, 50);
    ASSERT_MSG(sized_map != nullptr, "HashMap_create_sized failed with valid parameters");
    ASSERT_MSG(HashMap_size(sized_map) == 0, "Initial size of sized hashmap should be 0");
    HashMap_cleanup(sized_map, nullptr);

    /* Cleanup on NULL pointer must not crash */
    HashMap_cleanup(nullptr, nullptr);
}

/* 2. Test operations on NULL map (invalid state) */
static void test_invalid_state() {
    HashMapValue val = (HashMapValue)0x1234;

    ASSERT_MSG(HashMap_size(nullptr) == HASHMAP_INVALID_STATE,
               "HashMap_size(NULL) must return HASHMAP_INVALID_STATE");
    ASSERT_MSG(HashMap_put(nullptr, "key", "val") == HASHMAP_INVALID_STATE,
               "HashMap_put(NULL, ...) must return HASHMAP_INVALID_STATE");

    ASSERT_MSG(HashMap_peek(nullptr, "key", &val) == HASHMAP_INVALID_STATE,
               "HashMap_peek(NULL, ...) must return HASHMAP_INVALID_STATE");
    ASSERT_MSG(val == (HashMapValue)0x1234,
               "val out-param must not be modified when HashMap_peek fails on NULL map");

    ASSERT_MSG(HashMap_pop(nullptr, "key", &val) == HASHMAP_INVALID_STATE,
               "HashMap_pop(NULL, ...) must return HASHMAP_INVALID_STATE");
    ASSERT_MSG(val == (HashMapValue)0x1234,
               "val out-param must not be modified when HashMap_pop fails on NULL map");

    ASSERT_MSG(HashMap_contains(nullptr, "key") == false,
               "HashMap_contains(NULL, ...) must return false");
}

/* 3. Test basic put and contains */
static void test_put_and_contains() {
    HashMap map = HashMap_create(string_hash, string_equals);
    ASSERT(map != nullptr);

    ASSERT_MSG(!HashMap_contains(map, "user1"), "Key should not exist in empty map");

    ASSERT_MSG(HashMap_put(map, "user1", "Alice") == HASHMAP_OK, "Failed to insert user1");
    ASSERT_MSG(HashMap_size(map) == 1, "Size should be 1 after one insertion");
    ASSERT_MSG(HashMap_contains(map, "user1"), "Key user1 should exist after insertion");
    ASSERT_MSG(!HashMap_contains(map, "user2"), "Key user2 should not exist yet");

    ASSERT_MSG(HashMap_put(map, "user2", "Bob") == HASHMAP_OK, "Failed to insert user2");
    ASSERT_MSG(HashMap_size(map) == 2, "Size should be 2 after two insertions");
    ASSERT_MSG(HashMap_contains(map, "user2"), "Key user2 should exist after insertion");

    ASSERT_MSG(HashMap_put(map, "user3", "Charlie") == HASHMAP_OK, "Failed to insert user3");
    ASSERT_MSG(HashMap_size(map) == 3, "Size should be 3 after three insertions");
    ASSERT_MSG(HashMap_contains(map, "user3"), "Key user3 should exist after insertion");

    HashMap_cleanup(map, nullptr);
}

/* 4. Test duplicate key insertion */
static void test_duplicate_key() {
    HashMap map = HashMap_create(string_hash, string_equals);
    ASSERT(map != nullptr);

    ASSERT_MSG(HashMap_put(map, "keyA", "original_value") == HASHMAP_OK,
               "Failed initial insertion of keyA");
    ASSERT_MSG(HashMap_size(map) == 1, "Size should be 1");

    ASSERT_MSG(HashMap_put(map, "keyA", "new_value") == HASHMAP_DUPLICATED_KEY,
               "Expected HASHMAP_DUPLICATED_KEY when inserting duplicate key");
    ASSERT_MSG(HashMap_size(map) == 1, "Size should not increase on duplicate insertion");

    HashMapValue val = nullptr;
    ASSERT_MSG(HashMap_peek(map, "keyA", &val) == HASHMAP_OK, "Failed to peek keyA");
    ASSERT_MSG(strcmp((const char *)val, "original_value") == 0,
               "Value should remain original_value after failed duplicate put");

    HashMap_cleanup(map, nullptr);
}

/* 5. Test peek operations and unmodified out parameter on failure */
static void test_peek() {
    HashMap map = HashMap_create(string_hash, string_equals);
    ASSERT(map != nullptr);

    HashMap_put(map, "k1", "v1");
    HashMap_put(map, "k2", "v2");

    HashMapValue val = nullptr;
    ASSERT_MSG(HashMap_peek(map, "k1", &val) == HASHMAP_OK, "Failed to peek k1");
    ASSERT_MSG(strcmp((const char *)val, "v1") == 0, "Peeked value for k1 mismatch");

    ASSERT_MSG(HashMap_peek(map, "k2", &val) == HASHMAP_OK, "Failed to peek k2");
    ASSERT_MSG(strcmp((const char *)val, "v2") == 0, "Peeked value for k2 mismatch");

    /* Peek on non-existent key must return HASHMAP_KEY_NOT_FOUND and NOT modify val */
    val = (HashMapValue)0x5678;
    ASSERT_MSG(HashMap_peek(map, "non_existent", &val) == HASHMAP_KEY_NOT_FOUND,
               "Expected HASHMAP_KEY_NOT_FOUND for non-existent key");
    ASSERT_MSG(val == (HashMapValue)0x5678,
               "val out-param must remain untouched when key is not found");

    /* Size must remain 2 after peeks */
    ASSERT_MSG(HashMap_size(map) == 2, "Size should remain 2 after peek operations");

    HashMap_cleanup(map, nullptr);
}

/* 6. Test pop operations and removal semantics */
static void test_pop() {
    HashMap map = HashMap_create(string_hash, string_equals);
    ASSERT(map != nullptr);

    HashMap_put(map, "itemA", "valA");
    HashMap_put(map, "itemB", "valB");
    HashMap_put(map, "itemC", "valC");
    ASSERT_MSG(HashMap_size(map) == 3, "Size should be 3");

    /* Pop non-existent key */
    HashMapValue val = (HashMapValue)0xbeef;
    ASSERT_MSG(HashMap_pop(map, "non_existent", &val) == HASHMAP_KEY_NOT_FOUND,
               "Pop on non-existent key should return HASHMAP_KEY_NOT_FOUND");
    ASSERT_MSG(val == (HashMapValue)0xbeef,
               "val out-param must remain untouched when key to pop is not found");
    ASSERT_MSG(HashMap_size(map) == 3, "Size should remain 3 after failed pop");

    /* Pop middle item */
    val = nullptr;
    ASSERT_MSG(HashMap_pop(map, "itemB", &val) == HASHMAP_OK, "Pop itemB failed");
    ASSERT_MSG(strcmp((const char *)val, "valB") == 0, "Popped value mismatch for itemB");
    ASSERT_MSG(HashMap_size(map) == 2, "Size should be 2 after popping one element");
    ASSERT_MSG(!HashMap_contains(map, "itemB"), "itemB should no longer exist after pop");

    /* Subsequent peek or pop on itemB should fail */
    val = (HashMapValue)0xcafe;
    ASSERT_MSG(HashMap_peek(map, "itemB", &val) == HASHMAP_KEY_NOT_FOUND,
               "Peek on popped key should return HASHMAP_KEY_NOT_FOUND");
    ASSERT_MSG(val == (HashMapValue)0xcafe, "val out-param must remain untouched");
    ASSERT_MSG(HashMap_pop(map, "itemB", &val) == HASHMAP_KEY_NOT_FOUND,
               "Second pop on popped key should return HASHMAP_KEY_NOT_FOUND");
    ASSERT_MSG(val == (HashMapValue)0xcafe, "val out-param must remain untouched");

    /* Other items should remain intact */
    ASSERT_MSG(HashMap_peek(map, "itemA", &val) == HASHMAP_OK, "itemA should still be peekable");
    ASSERT_MSG(strcmp((const char *)val, "valA") == 0, "itemA value mismatch");
    ASSERT_MSG(HashMap_peek(map, "itemC", &val) == HASHMAP_OK, "itemC should still be peekable");
    ASSERT_MSG(strcmp((const char *)val, "valC") == 0, "itemC value mismatch");

    /* Pop remaining elements */
    ASSERT_MSG(HashMap_pop(map, "itemA", &val) == HASHMAP_OK, "Pop itemA failed");
    ASSERT_MSG(HashMap_pop(map, "itemC", &val) == HASHMAP_OK, "Pop itemC failed");
    ASSERT_MSG(HashMap_size(map) == 0, "Size should be 0 when all items are popped");

    /* Pop on empty map */
    val = (HashMapValue)0x1111;
    ASSERT_MSG(HashMap_pop(map, "itemA", &val) == HASHMAP_KEY_NOT_FOUND,
               "Pop on empty map should return HASHMAP_KEY_NOT_FOUND");
    ASSERT_MSG(val == (HashMapValue)0x1111, "val out-param must remain untouched");

    HashMap_cleanup(map, nullptr);
}

/* 7. Test re-insertion after pop */
static void test_reinsertion_after_pop() {
    HashMap map = HashMap_create(string_hash, string_equals);
    ASSERT(map != nullptr);

    ASSERT_MSG(HashMap_put(map, "key", "first_val") == HASHMAP_OK, "Initial put failed");
    HashMapValue val = nullptr;
    ASSERT_MSG(HashMap_pop(map, "key", &val) == HASHMAP_OK, "Pop failed");
    ASSERT_MSG(strcmp((const char *)val, "first_val") == 0, "Popped value mismatch");
    ASSERT_MSG(HashMap_size(map) == 0, "Size should be 0");

    /* Re-insert same key with new value */
    ASSERT_MSG(HashMap_put(map, "key", "second_val") == HASHMAP_OK,
               "Re-insertion of popped key should succeed");
    ASSERT_MSG(HashMap_size(map) == 1, "Size should be 1 after re-insertion");
    ASSERT_MSG(HashMap_peek(map, "key", &val) == HASHMAP_OK, "Peek after re-insertion failed");
    ASSERT_MSG(strcmp((const char *)val, "second_val") == 0,
               "Peeked value should match second_val");

    HashMap_cleanup(map, nullptr);
}

/* 8. Test collision handling */
static void test_collisions() {
    /* Using colliding_hash, all keys map to bucket 0 */
    HashMap map = HashMap_create_sized(colliding_hash, string_equals, 10);
    ASSERT(map != nullptr);

    ASSERT_MSG(HashMap_put(map, "colA", "valA") == HASHMAP_OK, "Put colA failed");
    ASSERT_MSG(HashMap_put(map, "colB", "valB") == HASHMAP_OK, "Put colB failed");
    ASSERT_MSG(HashMap_put(map, "colC", "valC") == HASHMAP_OK, "Put colC failed");
    ASSERT_MSG(HashMap_put(map, "colD", "valD") == HASHMAP_OK, "Put colD failed");
    ASSERT_MSG(HashMap_size(map) == 4, "Size should be 4 with 4 colliding elements");

    /* Duplicate key detection with collisions */
    ASSERT_MSG(HashMap_put(map, "colB", "duplicate") == HASHMAP_DUPLICATED_KEY,
               "Duplicate put should fail even in collision chain");
    ASSERT_MSG(HashMap_size(map) == 4, "Size should not increase");

    /* All elements must be retrievable */
    HashMapValue val = nullptr;
    ASSERT_MSG(HashMap_peek(map, "colA", &val) == HASHMAP_OK && strcmp((const char *)val, "valA") == 0,
               "Peek colA mismatch");
    ASSERT_MSG(HashMap_peek(map, "colB", &val) == HASHMAP_OK && strcmp((const char *)val, "valB") == 0,
               "Peek colB mismatch");
    ASSERT_MSG(HashMap_peek(map, "colC", &val) == HASHMAP_OK && strcmp((const char *)val, "valC") == 0,
               "Peek colC mismatch");
    ASSERT_MSG(HashMap_peek(map, "colD", &val) == HASHMAP_OK && strcmp((const char *)val, "valD") == 0,
               "Peek colD mismatch");

    /* Check contains */
    ASSERT_MSG(HashMap_contains(map, "colA"), "Contains colA failed");
    ASSERT_MSG(HashMap_contains(map, "colC"), "Contains colC failed");
    ASSERT_MSG(!HashMap_contains(map, "colNonExistent"), "Contains non-existent returned true");

    /* Pop middle element in collision list */
    ASSERT_MSG(HashMap_pop(map, "colB", &val) == HASHMAP_OK, "Pop colB failed");
    ASSERT_MSG(strcmp((const char *)val, "valB") == 0, "Popped value mismatch");
    ASSERT_MSG(HashMap_size(map) == 3, "Size should be 3");
    ASSERT_MSG(!HashMap_contains(map, "colB"), "colB should no longer exist");

    /* Ensure remaining elements in collision list are still accessible */
    ASSERT_MSG(HashMap_peek(map, "colA", &val) == HASHMAP_OK && strcmp((const char *)val, "valA") == 0,
               "colA should still be accessible after colB pop");
    ASSERT_MSG(HashMap_peek(map, "colC", &val) == HASHMAP_OK && strcmp((const char *)val, "valC") == 0,
               "colC should still be accessible after colB pop");
    ASSERT_MSG(HashMap_peek(map, "colD", &val) == HASHMAP_OK && strcmp((const char *)val, "valD") == 0,
               "colD should still be accessible after colB pop");

    /* Pop head/tail elements */
    ASSERT_MSG(HashMap_pop(map, "colA", &val) == HASHMAP_OK, "Pop colA failed");
    ASSERT_MSG(HashMap_pop(map, "colD", &val) == HASHMAP_OK, "Pop colD failed");
    ASSERT_MSG(HashMap_pop(map, "colC", &val) == HASHMAP_OK, "Pop colC failed");
    ASSERT_MSG(HashMap_size(map) == 0, "Size should be 0 after all popped");

    HashMap_cleanup(map, nullptr);
}

/* 9. Test bad hash function returning value >= hash_table_size */
static void test_bad_hash_function() {
    HashMap map = HashMap_create(bad_hash, string_equals);
    ASSERT(map != nullptr);

    HashMapValue val = (HashMapValue)0x2222;

    ASSERT_MSG(HashMap_put(map, "key", "val") == HASHMAP_BAD_HASH_FUNCTION,
               "HashMap_put must return HASHMAP_BAD_HASH_FUNCTION with bad hash function");

    ASSERT_MSG(HashMap_peek(map, "key", &val) == HASHMAP_BAD_HASH_FUNCTION,
               "HashMap_peek must return HASHMAP_BAD_HASH_FUNCTION with bad hash function");
    ASSERT_MSG(val == (HashMapValue)0x2222,
               "val out-param must not be modified when HashMap_peek fails");

    ASSERT_MSG(HashMap_pop(map, "key", &val) == HASHMAP_BAD_HASH_FUNCTION,
               "HashMap_pop must return HASHMAP_BAD_HASH_FUNCTION with bad hash function");
    ASSERT_MSG(val == (HashMapValue)0x2222,
               "val out-param must not be modified when HashMap_pop fails");

    ASSERT_MSG(HashMap_contains(map, "key") == false,
               "HashMap_contains must return false when hash function is bad");

    HashMap_cleanup(map, nullptr);
}

/* 10. Test cleanup callback */
static int g_cleanup_call_count = 0;
static void counting_cleanup(HashMapValue val) {
    if (val != nullptr) {
        g_cleanup_call_count++;
        free(val);
    }
}

static void test_cleanup_callback() {
    HashMap map = HashMap_create(string_hash, string_equals);
    ASSERT(map != nullptr);

    g_cleanup_call_count = 0;
    const int count = 10;
    char key_buf[10][16];

    for (int i = 0; i < count; i++) {
        snprintf(key_buf[i], sizeof(key_buf[i]), "k_%d", i);
        int *val = (int *)malloc(sizeof(int));
        ASSERT(val != nullptr);
        *val = i * 10;
        ASSERT(HashMap_put(map, key_buf[i], val) == HASHMAP_OK);
    }

    ASSERT_MSG(HashMap_size(map) == count, "Size should be 10 before cleanup");

    /* Cleanup with custom callback that frees memory and increments counter */
    HashMap_cleanup(map, counting_cleanup);
    ASSERT_MSG(g_cleanup_call_count == count,
               "Cleanup callback should be called for every stored value");
}

/* 11. Test NULL value storage and empty string keys */
static void test_special_keys_and_values() {
    HashMap map = HashMap_create(string_hash, string_equals);
    ASSERT(map != nullptr);

    /* Empty string as key */
    ASSERT_MSG(HashMap_put(map, "", "empty_key_val") == HASHMAP_OK,
               "Failed to insert empty string key");
    ASSERT_MSG(HashMap_contains(map, ""), "Contains failed for empty string key");

    HashMapValue val = nullptr;
    ASSERT_MSG(HashMap_peek(map, "", &val) == HASHMAP_OK, "Peek failed for empty string key");
    ASSERT_MSG(strcmp((const char *)val, "empty_key_val") == 0, "Value mismatch for empty string key");

    /* NULL value storage */
    ASSERT_MSG(HashMap_put(map, "null_val_key", nullptr) == HASHMAP_OK,
               "Failed to insert NULL value");
    ASSERT_MSG(HashMap_contains(map, "null_val_key"),
               "Contains should return true for key associated with NULL value");

    val = (HashMapValue)0x1234;
    ASSERT_MSG(HashMap_peek(map, "null_val_key", &val) == HASHMAP_OK,
               "Peek should succeed for key with NULL value");
    ASSERT_MSG(val == nullptr, "Peeked value should be NULL");

    val = (HashMapValue)0x5678;
    ASSERT_MSG(HashMap_pop(map, "null_val_key", &val) == HASHMAP_OK,
               "Pop should succeed for key with NULL value");
    ASSERT_MSG(val == nullptr, "Popped value should be NULL");

    ASSERT_MSG(HashMap_pop(map, "", &val) == HASHMAP_OK, "Pop failed for empty string key");
    ASSERT_MSG(strcmp((const char *)val, "empty_key_val") == 0, "Popped empty string value mismatch");

    ASSERT_MSG(HashMap_size(map) == 0, "Map size should be 0");
    HashMap_cleanup(map, nullptr);
}

/* 12. Test many elements (stress test) */
static void test_large_dataset() {
    HashMap map = HashMap_create_sized(string_hash, string_equals, 64);
    ASSERT(map != nullptr);

    const int total = 300;
    static char keys[300][32];

    for (int i = 0; i < total; i++) {
        snprintf(keys[i], sizeof(keys[i]), "large_key_%04d", i);
        /* Store integer value cast to pointer */
        HashMapErrors err = HashMap_put(map, keys[i], (HashMapValue)(uintptr_t)(i + 100));
        ASSERT_MSG(err == HASHMAP_OK, "Put failed in large dataset test");
    }

    ASSERT_MSG(HashMap_size(map) == total, "Size mismatch after large insertions");

    /* Verify all elements */
    for (int i = 0; i < total; i++) {
        ASSERT_MSG(HashMap_contains(map, keys[i]), "Contains failed for inserted key");
        HashMapValue val = nullptr;
        ASSERT_MSG(HashMap_peek(map, keys[i], &val) == HASHMAP_OK, "Peek failed in large test");
        ASSERT_MSG((uintptr_t)val == (uintptr_t)(i + 100), "Value mismatch in large test");
    }

    /* Pop half of the elements (even indexes) */
    for (int i = 0; i < total; i += 2) {
        HashMapValue val = nullptr;
        ASSERT_MSG(HashMap_pop(map, keys[i], &val) == HASHMAP_OK, "Pop failed in large test");
        ASSERT_MSG((uintptr_t)val == (uintptr_t)(i + 100), "Popped value mismatch in large test");
    }

    ASSERT_MSG(HashMap_size(map) == total / 2, "Size mismatch after popping half elements");

    /* Verify popped elements are missing and remaining elements are intact */
    for (int i = 0; i < total; i++) {
        if (i % 2 == 0) {
            ASSERT_MSG(!HashMap_contains(map, keys[i]), "Popped key still exists");
            HashMapValue val = nullptr;
            ASSERT_MSG(HashMap_peek(map, keys[i], &val) == HASHMAP_KEY_NOT_FOUND,
                       "Popped key should not be peekable");
        } else {
            ASSERT_MSG(HashMap_contains(map, keys[i]), "Remaining key not found");
            HashMapValue val = nullptr;
            ASSERT_MSG(HashMap_peek(map, keys[i], &val) == HASHMAP_OK, "Peek failed on remaining key");
            ASSERT_MSG((uintptr_t)val == (uintptr_t)(i + 100), "Value mismatch on remaining key");
        }
    }

    HashMap_cleanup(map, nullptr);
}

int main(void) {
    test_creation_and_cleanup();
    test_invalid_state();
    test_put_and_contains();
    test_duplicate_key();
    test_peek();
    test_pop();
    test_reinsertion_after_pop();
    test_collisions();
    test_bad_hash_function();
    test_cleanup_callback();
    test_special_keys_and_values();
    test_large_dataset();

    return ASSERT_REPORT();
}
