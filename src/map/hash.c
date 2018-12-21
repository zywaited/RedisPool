#include "hash.h"
#include "debug/debug.h"
#include <math.h>
#include <string.h>

private boolean isHashKeyEqual(mapKey* tmpKey, mapKey* key)
{
    if (tmpKey->h != key->h) {
        return false;
    }

    if (!tmpKey->str && !key->str) {
        return true;
    }

    if ((!tmpKey->str && key->str) || (tmpKey->str && !key->str)) {
        return false;
    }

    if (strcmp(tmpKey->str, key->str) != 0) {
        return false;
    }

    return true;
}

private int32_t getHashKey(hash* table, mapKey* tmpKey)
{
    int key;
    if (tmpKey->h == 0 && tmpKey->str && strlen(tmpKey->str)) {
        key = hashCode(tmpKey->str);
        tmpKey->h = key;
    } else {
        key = tmpKey->h;
    }

    return key | (-table->max);
}

private node* getHashNode(hash* table, uint32_t index)
{
    if (HASH_KEY_INDEX_VALID(table, index)) {
        return table->n + index;
    }

    return NULL;
}

private node* findHashNode(hash* table, mapKey* key)
{
    int32_t keyIndex = getHashKey(table, key);
    uint32_t index = HASH_KEY(table, keyIndex);
    node* current = getHashNode(table, index);
    while (current) {
        if (!current->def) {
            break;
        }

        if (isHashKeyEqual(key, current->key)) {
            return current;
        }

        current = getHashNode(table, HASH_NEXT(current));
    }

    return NULL;
}

private uint32_t getHashNextIndex(hash* table, node* current)
{
    current = getHashNode(table, HASH_NEXT(current));
    if (current) {
        return current->next;
    }

    return -1;
}

public mapValue* hashPut(hash* table, mapKey* key, mapValue* value)
{
    if (!key) {
        goto hashNodeError;
    }

    HASH_IS_INIT(table, NULL);
    if (table->total >= table->max) {
        resizeHash(table);
    }

    node* current = findHashNode(table, key);
    if (current) {
        mapValue* mv = current->value;
        current->value = value;
        return mv;
    }

    int32_t keyIndex = getHashKey(table, key);
    uint32_t index;
    current = getHashNode(table, table->undef);
    if (current) {
        index = table->undef;
        table->undef = HASH_NEXT(current);
    } else if (table->avalid < table->max) {
        current = table->n + table->avalid;
        index = table->avalid;
        table->avalid++;
    } else {
        goto hashNodeError;
    }

    HASH_NEXT(current) = HASH_KEY(table, keyIndex);
    HASH_KEY(table, keyIndex) = index;
    current->key = key;
    current->value = value;
    current->def = true;
    table->total++;
    return NULL;
hashNodeError:
    debug(DEBUG_ERROR, "The table node num is error or key is NULL");
    return NULL;
}

public void hashPutAndFree(hash* table, mapKey* key, mapValue* value)
{
    freeMapValue(hashPut(table, key, value), table->freeFuncName);
}

private byte resizeHash(hash* table)
{
    uint32_t tmpMax = table->max;
    node* oldNode = NULL;
    if (table->total < (tmpMax >> 1) - HASH_IDLE_NODE) {
        // 空闲节点过多
        tmpMax >>= 1;
        oldNode = table->n;
        table->n += tmpMax - table->max;
    } else if (table->total >= table->max) {
        tmpMax *= 2;
        node* tmpNode = (node*) malloc(sizeof(node) * (tmpMax << 1));
        if (!tmpNode) {
            debug(DEBUG_ERROR, "Can't resize hash table");
            return S_ERR;
        }

        tmpNode += tmpMax;
        memcpy(tmpNode, table->n, sizeof(node) * table->max);
        memset(tmpNode + table->max, 0, sizeof(node) * table->max);
        // 删除所有的内存
        free(table->n - table->max);
        table->n = tmpNode;
    } else {
        return S_OK;
    }

    table->max = tmpMax;
    // 初始化索引
    uint32_t* indexNode = HASH_EXCHANGE_NODE_TYPE(table);
    memset(
        indexNode - table->max,
        HASH_KEY_DEFAULT_INDEX,
        sizeof(uint32_t) * table->max
    );

    uint32_t i = 0;
    int32_t keyIndex;
    node* p = table->n;
    if (oldNode) {
        // 缩减内存
        tmpMax <<= 1;
        uint32_t tmpTotal = 0;
        table->avalid = table->total - 1;
        while (i++ < tmpMax && tmpTotal < table->total) {
            if (oldNode->def) {
                p->key = oldNode->key;
                p->value = oldNode->value;
                keyIndex = p->key->h | (-table->max);
                HASH_NEXT(p) = indexNode[keyIndex];
                indexNode[keyIndex] = i;
                p->def = true;
                tmpTotal++;
                p++;
            }

            oldNode++;
        }

        free(table->n + table->max);
    } else {
        // 新增
        while (i++ < table->total) {
            keyIndex = p->key->h | (-table->max);
            HASH_NEXT(p) = indexNode[keyIndex];
            indexNode[keyIndex] = i;
        }
    }

    return S_OK;
}

public byte newHash(hash** table, freeFunc freeFuncName)
{
    *table = (hash*)malloc(sizeof(hash));
    if (!table) {
        debug(DEBUG_ERROR, "Can't create hash table");
        return S_ERR;
    }

    (*table)->n = (node*)malloc(sizeof(node) * HASH_IDLE_NODE);
    if (!(*table)->n) {
        debug(DEBUG_ERROR, "Can't create hash table node");
        return S_ERR;
    }

    // 初始化
    (*table)->n += HASH_DEFAULT_MAX_NODE;
    memset((*table)->n, 0, sizeof(node) * HASH_DEFAULT_MAX_NODE);
    int32_t index = -HASH_DEFAULT_MAX_NODE;
    uint32_t* indexNode = HASH_EXCHANGE_NODE_TYPE((*table));
    for (; index < 0; index++) {
        indexNode[index] = -1;
    }

    (*table)->avalid = 0;
    (*table)->max = HASH_DEFAULT_MAX_NODE;
    (*table)->total = 0;
    (*table)->undef = -1;
    (*table)->freeFuncName = freeFuncName;
    return S_OK;
}

public mapValue* hashRemove(hash* table, mapKey* key)
{
    if (!table) {
        return NULL;
    }

    int32_t keyHash = getHashKey(table, key);
    uint32_t index = HASH_KEY(table, keyHash);
    node* current = getHashNode(table, index);
    node* parent = NULL;
    while (current) {
        if (!current->def) {
            break;
        }

        if (!isHashKeyEqual(key, current->key)) {
            parent = current;
            current = getHashNode(table, HASH_NEXT(parent));
            index = HASH_NEXT(parent);
            continue;
        }

        uint32_t nextIndex = HASH_NEXT(current);
        // first
        if (!parent) {
            // exit next node
            if (HASH_KEY_INDEX_VALID(table, nextIndex)) {
                HASH_KEY(table, keyHash) = nextIndex;
            }
        } else {
            HASH_NEXT(parent) = nextIndex;
        }

        uint32_t tmpIndex = table->undef;
        table->undef = index;
        HASH_NEXT(current) = tmpIndex;
        freeMapKey(current->key);
        mapValue* mv = current->value;
        current->value = NULL;
        current->def = false;
        table->total--;
        resizeHash(table);
        return mv;
    }
    
    return NULL;
}

public void hashRemoveAndFree(hash* table, mapKey* key)
{
    freeMapValue(hashRemove(table, key), table->freeFuncName);
}

public mapValue* hashGet(hash* table, mapKey* key)
{
    if (!table) {
        return NULL;
    }

    node* current = findHashNode(table, key);
    if (current) {
        return current->value;
    }

    return NULL;
}

public void freeHash(hash* table)
{
    if (!table) {
        return;
    }

    if (table->n) {
        node* n;
        uint32_t index = 0;
        while (index <= table->avalid) {
            n = table->n + index;
            freeMapValue(n->value, table->freeFuncName);
            freeMapKey(n->key);
            index++;
        }

        free(table->n);
    }

    table->n = NULL;
    table->undef = -1;
    table->total = table->avalid = 0;
    free(table);
}

public boolean isExistKey(hash* table, mapKey* key)
{
    if (!table) {
        return false;
    }

    node* current = findHashNode(table, key);
    return current ? true : false;
}