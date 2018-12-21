#include "tree.h"

public tree* newTree(size_t max, freeFunc freeFuncName)
{
	tree* tr = (tree*)malloc(sizeof(tree));
	if (!tr) {
        debug(DEBUG_ERROR, "Can't create tree");
        return NULL;
	}

	tr->max = max;
	tr->total = 0;
	tr->root = NULL;
	tr->valid = true;
	tr->freeFuncName = freeFuncName;
	return tr;
}


public treeNode* newTreeNode(int key, mapValue* value)
{
	treeNode* tn = (treeNode*) malloc(sizeof(treeNode));
	if (!tn) {
        debug(DEBUG_ERROR, "Can't create tree node");
        return NULL;
	}

	tn->left = tn->right = tn->parent = NULL;
	tn->color = BLACK;
	tn->key = key;
	tn->value = value;
	return tn;
}

public void freeTree(tree* tr)
{
	if (!tr) {
		return;
	}

	freeTreeNode(tr, tr->root, DELETE_ALL);
	tr->root = NULL;
	tr->valid = false;
	tr->total = 0;
	free(tr);
}

private void freeTreeNode(tree* tr, treeNode* tn, byte type)
{
	if (!tn) {
		return;
	}

	if (DELETE_ALL == type) {
		freeTreeNode(tr, tn->left, DELETE_ALL);
		freeTreeNode(tr, tn->right, DELETE_ALL);
	}

	tn->left = tn->right = NULL;
	freeMapValue(tn->value, tr->freeFuncName);
	tn->value = NULL;
	free(tn);
}

private void leftRotate(tree* tr, treeNode* tn)
{
	if (!tr || !tn) {
		return;
	}

	treeNode* son = rightOf(tn);
	treeNode* parent = parentOf(tn);
	if (leftOf(son)) {
		son->left->parent = tn;
	}

	tn->right = leftOf(son);
	son->parent = parent;
	tn->parent = son;
	if (!parent) {
		tr->root = son;
	} else if (leftOf(tn) == tn) {
		parent->left = son;
	} else {
		parent->right = son;
	}

	son->left = tn;
}

private void rightRotate(tree* tr, treeNode* tn)
{
	if (!tr || !tn) {
		return;
	}

	treeNode* son = leftOf(tn);
	treeNode* parent = parentOf(tn);
	if (rightOf(son)) {
		son->right->parent = tn;
	}

	tn->left = rightOf(son);
	son->parent = parent;
	tn->parent = son;
	if (!parent) {
		tr->root = son;
	} else if (leftOf(tn) == tn) {
		parent->left = son;
	} else {
		parent->right = son;
	}

	son->right = tn;
}

public mapValue* treeGet(tree* tr, int key)
{
	treeNode* tn = getTreeNode(tr, key);
	return tn ? tn->value : NULL;
}

public byte treePut(tree* tr, int key, mapValue* value)
{
	if (!tr) {
		debug(DEBUG_WARNING, "Can't put value to empty tree");
		return S_ERR;
	}

	if (tr->max > 0 && tr->total >= tr->max) {
		debug(DEBUG_WARNING, "Can't put value to full tree");
		return S_ERR;
	}

	treeNode* tn = tr->root;
	treeNode* parent = NULL;
	boolean isLeft = false;
	while (tn) {
		parent = tn;
		if (key < tn->key) {
			isLeft = true;
			tn = leftOf(tn);
		} else if (key > tn->key) {
			isLeft = false;
			tn = rightOf(tn);
		} else {
			freeMapValue(tn->value, tr->freeFuncName);
			tn->value = value;
			return S_OK;
		}
	}

	treeNode* tmp = newTreeNode(key, value);
	if (!tmp) {
		return S_ERR;
	}

	if (!parent) {
		tr->root = tmp;
	} else if (isLeft) {
		parent->left = tmp;
	} else {
		parent->right = tmp;
	}

	tr->total++;
	fixAfterInsert(tr, tmp);
	return S_OK;
}

private void fixAfterInsert(tree* tr, treeNode* tn)
{
	if (!tr || !tn) {
		return;
	}

	setRedColor(tn);
	treeNode* uncle;
	treeNode* grand;
	while (tn && parentOf(tn) && isRedColor(parentOf(tn))) {
		grand = parentOf(parentOf(tn));
		if (parentOf(tn) == leftOf(grand)) {
			// 左节点
			uncle = rightOf(grand);
			if (isRedColor(uncle)) {
				setBlackColor(parentOf(tn));
				setBlackColor(uncle);
				setRedColor(grand);
				tn = grand;
			} else {
				if (rightOf(parentOf(tn)) == tn) {
					// 右节点
					tn = parentOf(tn);
					leftRotate(tr, tn);
					grand = parentOf(parentOf(tn));
				}

				setBlackColor(parentOf(tn));
				setRedColor(grand);
				rightRotate(tr, grand);
			}
		} else {
			uncle = leftOf(grand);
			if (isRedColor(uncle)) {
				setBlackColor(parentOf(tn));
				setBlackColor(uncle);
				setRedColor(grand);
				tn = grand;
			} else {
				if (leftOf(parentOf(tn)) == tn) {
					tn = parentOf(tn);
					rightRotate(tr, tn);
					grand = parentOf(parentOf(tn));
				}

				setBlackColor(parentOf(tn));
				setRedColor(grand);
				leftRotate(tr, grand);
			}
		}
	}

	setBlackColor(tr->root);
}

public mapValue* treeRemove(tree* tr, int key)
{
	if (!tr || !tr->root) {
		return NULL;
	}

	treeNode* current = getTreeNode(tr, key);
	if (!current) {
		return NULL;
	}

	mapValue* mv = NULL;
	// 判断是否有左右节点
	if (leftOf(current) && rightOf(current)) {
		treeNode* search = successor(current);
		current->key = search->key;
		mv = current->value;
		current->value = search->value;
		search->value = mv;
		current = search;
	}

	// 选择替代节点
	treeNode* replace = leftOf(current) ? leftOf(current) : rightOf(current);
	if (replace != NULL) {
		replace->parent = parentOf(current);
		if (parentOf(current) == NULL) {
			tr->root = replace;
		}

		if (!parentOf(current)) {
			tr->root = replace;
		} else if (leftOf(parentOf(current)) == current) {
			current->parent->left = replace;
		} else {
			current->parent->right = replace;
		}

		if (isBlackColor(current)) {
			fixAfterRemove(tr, replace);
		}
	} else if (parentOf(replace) == NULL) {
		tr->root = NULL;
	} else {
		if (isBlackColor(current)) {
			fixAfterRemove(tr, current);
		}

		if (parentOf(current)) {
			if (leftOf(parentOf(current)) == current) {
				current->parent->left = NULL;
			} else {
				current->parent->right = NULL;
			}
		}
	}

	current->parent = current->left = current->right = NULL;
	mv = current->value;
	current->value = NULL;
	freeTreeNode(tr, current, DELETE_SELF);
	tr->total--;
	return mv;
}

public void treeRemoveAndFree(tree* tr, int key)
{
	freeMapValue(treeRemove(tr, key), tr->freeFuncName);
}

private void fixAfterRemove(tree* tr, treeNode* tn)
{
	treeNode* sib;
	while (tn != tr->root && isBlackColor(tn)) {
		if (leftOf(tn) == tn) {
			sib = rightOf(parentOf(tn));
			if (isRedColor(sib)) {
				// 兄弟节点为红色
				// 交换兄弟和父的颜色并左旋
				setBlackColor(sib);
				setRedColor(parentOf(tn));
				leftRotate(tr, parentOf(tn));
				sib = rightOf(parentOf(tn));
			}

			if (isBlackColor(leftOf(sib)) && isBlackColor(rightOf(sib))) {
				// 兄弟左右子节点都为黑
				// 直接设置兄弟节点为红色并下一轮比较
				setRedColor(sib);
				tn = parentOf(tn);
			} else {
				if (isBlackColor(rightOf(sib))) {
					// 兄弟右子节点为黑色
					// 交换兄弟和其左子节点颜色并右旋
					setRedColor(sib);
					setBlackColor(leftOf(sib));
					rightRotate(tr, sib);
					sib = rightOf(parentOf(tn));
				}

				// 交换兄弟和父的颜色并左旋
				// 兄弟节点为父颜色，旋转后整体黑色高度不变
				// 设置父为黑则左树补了一个黑点
				// 设置兄弟右子节点为黑色补足
				setColor(sib, getColor(parentOf(tn)));
				setBlackColor(parentOf(tn));
				setBlackColor(rightOf(sib));
				leftRotate(tr, parentOf(tn));
				tn = tr->root;
			}
		} else {
			sib = leftOf(parentOf(tn));
			if (isRedColor(sib)) {
				setBlackColor(sib);
				setRedColor(parentOf(tn));
				rightRotate(tr, parentOf(tn));
				sib = leftOf(parentOf(tn));
			}

			if (isBlackColor(leftOf(sib)) && isBlackColor(rightOf(sib))) {
				setRedColor(sib);
				tn = parentOf(tn);
			} else {
				if (isBlackColor(leftOf(sib))) {
					setRedColor(sib);
					setBlackColor(rightOf(sib));
					leftRotate(tr, sib);
					sib = leftOf(parentOf(tn));
				}

				setColor(sib, getColor(parentOf(tn)));
				setBlackColor(parentOf(tn));
				setBlackColor(leftOf(sib));
				rightRotate(tr, parentOf(tn));
				tn = tr->root;
			}
		}
	}

	setBlackColor(tn);
}

public boolean isRedColor(treeNode* tn)
{
	return getColor(tn) == RED;
}

public boolean isBlackColor(treeNode* tn)
{
	return getColor(tn) == BLACK;
}

public boolean getColor(treeNode* tn)
{
	return tn ? tn->color : BLACK;
}

public void setRedColor(treeNode* tn)
{
	setColor(tn, RED);
}

public void setBlackColor(treeNode* tn)
{
	setColor(tn, BLACK);
}

public void setColor(treeNode* tn, boolean color)
{
	if (!tn) {
		return;
	}

	tn->color = color;
}

public treeNode* leftOf(treeNode* tn)
{
	return tn ? tn->left : NULL;
}

public treeNode* rightOf(treeNode* tn)
{
	return tn ? tn->right : NULL;
}

public treeNode* parentOf(treeNode* tn)
{
	return tn ? tn->parent : NULL;
}

private treeNode* successor(treeNode* tn)
{
	// 只考虑有左右子节点的
	if (!tn || rightOf(tn) == NULL) {
		return NULL;
	}

	treeNode* tmp = rightOf(tn);
	while (leftOf(tmp)) {
		tmp = leftOf(tmp);
	}

	return tmp;
}

private treeNode* getTreeNode(tree* tr, int key)
{
	if (!tr) {
		return NULL;
	}

	treeNode* tn = tr->root;
	while (tn) {
		if (key < tn->key) {
			tn = leftOf(tn);
		} else if (key > tn->key) {
			tn = rightOf(tn);
		} else {
			return tn;
		}
	}

	return NULL;
}