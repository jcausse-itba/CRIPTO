#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>

int main() {
    const char *message = "Hello, OpenSSL!";
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;

    // Create a context for the digest operation
    EVP_MD_CTX *context = EVP_MD_CTX_new();
    
    // Initialize the context with SHA-256
    EVP_DigestInit_ex(context, EVP_sha256(), NULL);
    
    // Pass the message data
    EVP_DigestUpdate(context, message, strlen(message));
    
    // Finalize the hash and fetch the results
    EVP_DigestFinal_ex(context, hash, &hash_len);
    
    // Clean up memory
    EVP_MD_CTX_free(context);

    // Print the SHA-256 hash in hexadecimal format
    printf("Message: %s\n", message);
    printf("SHA-256 Hash: ");
    for (unsigned int i = 0; i < hash_len; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");

    return 0;
}
