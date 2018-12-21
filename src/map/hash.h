#ifndef __HASH_HEADER
#define __HASH_HEADER

#include <stdio.h>
#include "base.h"
#include <stdint.h>
#include "map.h"

typedef struct node {
    mapKey* key;
    mapValue* value;
    // link next node
    uint32_t next;
    boolean def;
} node;

typedef struct hash {
    uint32_t avalid;
    uint32_t max;
    uint32_t total;
    // link to undef node
    uint32_t undef;
    // node and hash index
    node* n;
    freeFunc freeFuncName;
} hash;

#define HASH_KEY_DEFAULT_INDEX ((uint32_t) - 1)
#define HASH_KEY_INDEX_VALID(table, index)                  \
	(index != HASH_KEY_DEFAULT_INDEX && index < table->max)
#define HASH_EXCHANGE_NODE_TYPE(table) ((uint32_t*)(table->n))
// get hash key
#define HASH_KEY(table, keyIndex) HASH_EXCHANGE_NODE_TYPE(table)[(int32_t)keyIndex]
#define HASH_NEXT(current) current->next

#define HASH_DEFAULT_MAX_NODE 8
#define HASH_IDLE_NODE (HASH_DEFAULT_MAX_NODE << 1)

private boolean isHashKeyEqual(mapKey*, mapKey*);
private int32_t getHashKey(hash*, mapKey*);
private uint32_t getHashNextIndex(hash*, node*);
private node* getHashNode(hash*, uint32_t);
private node* findHashNode(hash*, mapKey*);
public byte newHash(hash**, freeFunc);
private byte resizeHash(hash*);
public mapValue* hashPut(hash*, mapKey*, mapValue*);
public void hashPutAndFree(hash*, mapKey*, mapValue*);
// 注意，该函数不是释放内存，请自行释放
public mapValue* hashRemove(hash*, mapKey*);
public void hashRemoveAndFree(hash*, mapKey*);
public mapValue* hashGet(hash*, mapKey*);
public void freeHash(hash*);

public boolean isExistKey(hash*, mapKey*);

#define FOREACH(table, mk, mv)                      \
    do {                                            \
        if (!table || !table->n) {                  \
            continue;                               \
        }                                           \
                                                    \
        uint32_t index = 0;                         \
        for (; index < table->avalid; index++) {    \
            node* n = table->n + index;             \
            if (!n->def) {                          \
                continue;                           \
            }                                       \
                                                    \
            mk = n->key;                            \
            mv = n->value;

#define END_FOREACH()                               \
        }                                           \
    } while (0);


#define HASH_IS_INIT(table, freeFuncName)           \
    do {                                            \
        if (!table) {                               \
            newHash(&table, freeFuncName);          \
        }                                           \
    } while (0)

#endif