#include <string.h>
#include "common.hpp"
#include "ParseUtil.hpp"


// --------------------------------------------------------------------------
// 
//                               HashMap
// 
// --------------------------------------------------------------------------

int VoidHashMap::calc_hash_impl(const char *key, int keyLength, size_t max)
{
    assert(key != nullptr);
    assert(keyLength >= 0);
    assert(max > 0);

    // this is a simple hash function for string keys,
    // it may cause more hash collision than more complex hash functions,
    // but it is faster and the performance is acceptable for our use case.
    unsigned int sum = keyLength;
    int border = 128;

    int salt = 0; // prevent same result from only order different of letters
    for (int i = 0; i < keyLength && i < border; i++) {
        unsigned char unsignedValue = key[i];
        sum += unsignedValue;
        // use different salt for different order of letters, e.g. "ab" and "ba"
        salt += i % 2 == 0 ? unsignedValue * i : -unsignedValue * i;
    }

    for (int i = keyLength-1,j=0; i >= border && j < border; i--,j++) {
        unsigned char unsignedValue = key[i];// (key[i] < 0 ? -key[i] : key[i]);
        sum += unsignedValue;
        // use different salt for different order of letters, e.g. "ab" and "ba"
        salt += j % 2 == 0 ? unsignedValue * j : -unsignedValue * j;
    }
    if (salt < 0) {
        salt = -(salt);
    }
    return (sum + salt) % max; // use modulo to fit the hash into the table size
}


static bool isSameKey(VoidHashNode *hashNode, int keyLength, const char *key) {
    if (hashNode->keyLength != keyLength) {
        return false;
    }

    for (int i = 0; i < keyLength; i++) {
        if (hashNode->key[i] != key[i]) {
            return false;
        }
    }

    return true;
}

void VoidHashMap::put(const char *key, int keyLength, void* val)
{
    auto hashInt = this->calc_hash_impl(key, keyLength, this->entries_length);
    VoidHashNode* hashNode = this->entries[hashInt];

    // if the hash node is null, create a new hash node and add it to the entries
    if (hashNode == nullptr) {
        auto *newHashNode = this->memBuffer->newMem<VoidHashNode>(1);
        newHashNode->next = nullptr;
        this->entries[hashInt] = newHashNode;

        newHashNode->key = this->memBuffer->newTextAssign(key, static_cast<unsigned int>(keyLength));
        newHashNode->keyLength = keyLength;
        newHashNode->voidPtrItem = val;
        return;
    }

    //  update the value for the existing key if the key already exists in the hash map
    while (hashNode != nullptr) {
        if (isSameKey(hashNode, keyLength, key)) {
            hashNode->voidPtrItem = val; // update the value for the existing key
            return;
        }

        if (hashNode->next == nullptr) {
            break;
        }

        hashNode = hashNode->next;
    }

    assert(hashNode != nullptr);

    // if the key does not exist, create a new hash node and add it to the end of the linked list
    auto *newHashNode = this->memBuffer->newMem<VoidHashNode>(1);
    newHashNode->key = this->memBuffer->newTextAssign(key, static_cast<unsigned int>(keyLength));
    newHashNode->keyLength = keyLength;
    newHashNode->voidPtrItem = val;
    newHashNode->next = nullptr;

    hashNode->next = newHashNode;
}


void VoidHashMap::init(MemBuffer* membuffer)
{
    this->memBuffer = membuffer;
    this->entries = this->memBuffer->newMemArray<VoidHashNode*>(HashNode_TABLE_SIZE);
    this->entries_length = HashNode_TABLE_SIZE;
    // initialize all entries to nullptr
    memset(this->entries, 0, sizeof(VoidHashNode*) * this->entries_length);
    //// or you can use a loop to set each entry to nullptr
    // for (unsigned int i = 0; i < this->entries_length; i++) {
    //     this->entries[i] = nullptr;
    // }
}

bool VoidHashMap::hasKey(const char * key, int keyLength)
{
    auto hashKey = calc_hash(key, keyLength);
    auto *hashNode = this->entries[hashKey];
    while (hashNode != nullptr) {
        if (isSameKey(hashNode, keyLength, key)) {
            return true;
        }
        hashNode = hashNode->next;
    }
    return false;
}

void VoidHashMap::deleteKey(const char *key, int keyLength)
{
    int hashKey = calc_hash(key, keyLength);
    VoidHashNode *hashNode = this->entries[hashKey];
    VoidHashNode *firstNode = hashNode;
    VoidHashNode *prevNode = nullptr;

    while (hashNode != nullptr) {
        if (isSameKey(hashNode, keyLength, key)) {
            // Remove the node from the linked list
            if (hashNode == firstNode) {
                this->entries[hashKey] = hashNode->next;
            }
            else {
                assert(prevNode != nullptr); // prevNode should not be null if hashNode is not the first node
                prevNode->next = hashNode->next; // link the previous node to the next node, effectively removing the current node from the list
            }
            break;
        }
        prevNode = hashNode;
        hashNode = hashNode->next;
    }
}

void* VoidHashMap::get(const char *key, int keyLength)
{
    auto hashKey = calc_hash(key, keyLength);
    auto *hashNode = this->entries[hashKey];
    if (hashNode != nullptr) {
        while (hashNode != nullptr) {
            if (isSameKey(hashNode, keyLength, key)) {
                return hashNode->voidPtrItem;
            }
            hashNode = hashNode->next;
        }
    }
    return nullptr;
}