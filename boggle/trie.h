
#include <stdbool.h>
// Alphabet size (# of symbols)
#define ALPHABET_SIZE (26)
// trie node
struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];

    // isEndOfWord is true if the node represents
    // end of a word
    bool isEndOfWord;
};
struct TrieNode *getNode(void);
void insert(struct TrieNode *root, const char *key);
bool search(struct TrieNode *root, const char *key);
void clear(struct TrieNode *root);
