#include <string.h>
#include "common.hpp"
#include "ParseUtil.hpp"


// --------------------------------------------------------------------------
// 
//                               HashMap
// 
// --------------------------------------------------------------------------

int VoidHashMap::calc_hash(const char *key, int keyLength, size_t max)
{
    unsigned int sum = keyLength;
    int border = 128;

    int salt = 0; // prevent same result from only order different of letters
    for (int i = 0; i < keyLength && i < border; i++) {
        unsigned char unsignedValue = key[i];
        sum += unsignedValue;
        salt += i % 2 == 0 ? unsignedValue * i : -unsignedValue * i;
    }

    for (int i = keyLength-1,j=0; i >= border && j < border; i--,j++) {
        unsigned char unsignedValue = key[i];// (key[i] < 0 ? -key[i] : key[i]);
        sum += unsignedValue;
        salt += j % 2 == 0 ? unsignedValue * j : -unsignedValue * j;
    }
    if (salt < 0) {
        salt = -(salt);
    }
    return (sum + salt) % max;
}

void VoidHashMap::put(const char *keyA, int keyLength, void* val) const
{
    auto hashInt = calc_hash(keyA, keyLength, this->entries_length);
    VoidHashNode* hashNode = this->entries[hashInt];

    if (hashNode == nullptr) {// || hashNode->key == nullptr) {
        auto *newHashNode = this->memBuffer->newMem<VoidHashNode>(1);
        newHashNode->next = nullptr;
        this->entries[hashInt] = newHashNode;


        char *keyB = this->memBuffer->newText(keyLength);
        for (int i = 0; i < keyLength; i++) {
            keyB[i] = keyA[i];
        }
        newHashNode->key = keyB;
        newHashNode->keyLength = keyLength;
        newHashNode->nodeBase = val;
        return;
    }

    while (true) {
        // find same key
        if (hashNode->keyLength == keyLength) {
            bool sameKey = true;
            for (int i = 0; i < keyLength; i++) {
                if (hashNode->key[i] != keyA[i]) {
                    sameKey = false;
                    break;
                }
            }
            if (sameKey) {
                hashNode->nodeBase = val;
                return;
            }
        }

        if (hashNode->next == nullptr) {
            break;
        }
        hashNode = hashNode->next;
    }


    auto *newHashNode = this->memBuffer->newMem<VoidHashNode>(1);
    char *keyB =  this->memBuffer->newText(keyLength);
    for (int i = 0; i < keyLength; i++) {
        keyB[i] = keyA[i];
    }
    newHashNode->key = keyB;
    newHashNode->keyLength = keyLength;
    newHashNode->nodeBase = val;
    newHashNode->next = nullptr;

    hashNode->next = newHashNode;
}


void VoidHashMap::init(MemBuffer* membuffer)
{
    this->memBuffer = membuffer;
    this->entries = (VoidHashNode**)this->memBuffer->newMemArray<VoidHashNode>(HashNode_TABLE_SIZE);
    this->entries_length = HashNode_TABLE_SIZE;
    memset(this->entries, 0, sizeof(VoidHashNode*)*this->entries_length);
    for (unsigned int i = 0; i < this->entries_length; i++) {
        this->entries[i] = nullptr;
    }
}

bool VoidHashMap::has(const char * key, int keyLength)
{
    return this->entries[calc_hash0(key, keyLength)] != nullptr;
}

void VoidHashMap::deleteKey(const char * key, int keyLength)
{
    if (this->entries[calc_hash0(key, keyLength)] != nullptr) {
        free(this->entries[calc_hash0(key, keyLength)]);
    }
}

void* VoidHashMap::get(const char * key, int keyLength)
{
    auto keyInt = calc_hash0(key, keyLength);
    if (this->entries[keyInt] != nullptr) {
        auto * hashNode = this->entries[keyInt];
        while (hashNode) {
            if (hashNode->keyLength == keyLength) {
                bool sameKey = true;
                for (int i = 0; i < keyLength; i++) {
                    if (hashNode->key[i] != key[i]) {
                        sameKey = false;
                        break;
                    }
                }

                if (sameKey) {
                    return hashNode->nodeBase;
                }
            }

            hashNode = hashNode->next;
        }
    }
    return nullptr;
}
