#ifndef __TREE_HEADER
#define __TREE_HEADER

#include "base.h"
#include "map.h"
#include "debug/debug.h"

// 注： void* value不是由malloc申请的内存
// 这里也不会调用free

private const boolean RED = true;
private const boolean BLACK = false;

typedef struct treeNode {
	struct treeNode* left;
	struct treeNode* right;
	struct treeNode* parent;
	boolean color;
	int key;
	mapValue* value;
} treeNode, treeNodeRoot;

typedef struct tree {
	// max item
	size_t max;
	// current item
    size_t total;
    boolean valid;
    treeNodeRoot* root;
    freeFunc freeFuncName;
} tree;

#define DELETE_SELF 0
#define DELETE_ALL 1

public tree* newTree(size_t, freeFunc);

public treeNode* newTreeNode(int, mapValue*);

public void freeTree(tree*);

private void freeTreeNode(tree*, treeNode*, byte);

/**
 * 左旋
 * @param tree* tr
 * @param treeNode* n
 */
private void leftRotate(tree*, treeNode*);

/**
 * 右旋
 * @param tree* tr
 * @param treeNode* n
 */
private void rightRotate(tree*, treeNode*);

/**
 * @param tree* tr
 * @param int key
 */
public mapValue* treeGet(tree*, int);

/**
 * @param tree* tr
 * @param int key
 * @param mapValue* value
 */
public byte treePut(tree*, int, mapValue*);

/**
 * 插入新数据调整(以叔叔节点为参照物)
 * @param tree* tr
 * @param treeNode* n
 */
private void fixAfterInsert(tree*, treeNode*);

/**
 * 注意，该函数不是释放内存，请自行释放
 * @param tree* tr
 * @param int key
 */
public mapValue* treeRemove(tree*, int);

public void treeRemoveAndFree(tree*, int);

/**
 * 删除数据调整(以兄弟节点为参照物)
 * @param tree* tr
 * @param treeNode* n
 */
private void fixAfterRemove(tree*, treeNode*);

public boolean isRedColor(treeNode*);

public boolean isBlackColor(treeNode*);

public boolean getColor(treeNode*);

public void setRedColor(treeNode*);

public void setBlackColor(treeNode*);

public void setColor(treeNode*, boolean);

public treeNode* leftOf(treeNode*);

public treeNode* rightOf(treeNode*);

public treeNode* parentOf(treeNode*);

/**
 * 获取替代节点，此处右树最小
 * @param treeNode* tn
 * @return treeNode*
 */
private treeNode* successor(treeNode*);

private treeNode* getTreeNode(tree*, int);

#endif