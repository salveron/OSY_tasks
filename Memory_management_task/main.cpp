#ifndef __PROGTEST__
#include <cstdio>
#include <cstdint>
#include <cassert>
using namespace std;
#endif /* __PROGTEST__ */

#define BYTE unsigned char

struct HeapBlock{
    void * ptr;
    int size;
    bool isFree;
    HeapBlock * left;
    HeapBlock * right;
};

HeapBlock * root;
int blockCounter;


HeapBlock * newBlock(void * memPool, int blockSize){
    auto * newBlock = (HeapBlock *)memPool;
    newBlock->ptr = (BYTE *)memPool + sizeof(HeapBlock);
    newBlock->size = blockSize - (int)sizeof(HeapBlock);
    newBlock->isFree = true;
    newBlock->left = newBlock->right = nullptr;

    return newBlock;
}


void divideBlock(HeapBlock * parent){
    assert(parent);
    int childSize = (int)(parent->size / 2);
    parent->left = newBlock(parent->ptr, childSize);
    void * rightChildMem = (void *)((BYTE *)parent->ptr + childSize);
    parent->right = newBlock(rightChildMem, childSize);
    parent->ptr = nullptr;
}


void mergeBlock(HeapBlock * parent){
    assert(parent && !parent->ptr && parent->left && parent->right);
    assert(parent->isFree && parent->left->isFree && parent->right->isFree);
    assert(!parent->left->left && !parent->left->right && !parent->right->left && !parent->right->right);
    parent->ptr = (void *)parent->left;
    parent->left = parent->right = nullptr;
}


HeapBlock * recDivide(HeapBlock * recRoot, int & size){
    bool canBeDivided = size <= (recRoot->size / 2) - (int)sizeof(HeapBlock);
    if (!recRoot->ptr && !canBeDivided)  // has children
        return nullptr;
    else if (recRoot->ptr){  // has no children
        if (canBeDivided)
            divideBlock(recRoot);
        else {
            recRoot->isFree = false;
            return recRoot;
        }
    }

    HeapBlock * tmp = nullptr;
    if (recRoot->left->isFree)
        tmp = recDivide(recRoot->left, size);
    if (!tmp && recRoot->right->isFree)
        tmp = recDivide(recRoot->right, size);
    recRoot->isFree = recRoot->left->isFree || recRoot->right->isFree;
    return tmp;
}


HeapBlock * recMerge(HeapBlock * recRoot, const void * blk){
    HeapBlock * tmp = nullptr;
    if (!recRoot->ptr && blk < recRoot->right)  // go to the left depending on address
        tmp = recMerge(recRoot->left, blk);
    else if (!recRoot->ptr && blk >= recRoot->right)  // go to the right depending on address
        tmp = recMerge(recRoot->right, blk);

    if (tmp) { // the left or right block was freed -> try to merge
        recRoot->isFree = recRoot->left->isFree || recRoot->right->isFree;
        if (recRoot->left->isFree && recRoot->right->isFree && recRoot->left->ptr && recRoot->right->ptr)
            mergeBlock(recRoot);
        return tmp;
    }

    if (recRoot->ptr && recRoot->ptr == blk && !recRoot->isFree){
        recRoot->isFree = true;
        return recRoot;
    } else return nullptr;
}


void printBlock(HeapBlock * block){
    printf("Blk: str [%p], ptr [%p], size [%d], free [%d], left [%p], right [%p]\n",
           (void *)block, block->ptr, block->size, block->isFree, (void *)block->left, (void *)block->right);
}


void printTree(HeapBlock * treeRoot, int indentation){
    if (!treeRoot) return;
    for (int i = 0; i < indentation; i++)
        printf("| ");
    printBlock(treeRoot);
    printTree(treeRoot->left, indentation + 1);
    printTree(treeRoot->right, indentation + 1);
}


void HeapInit(void * memPool, int memSize){
    root = newBlock(memPool, (memSize / 2) * 2);
    blockCounter = 0;
}


void * HeapAlloc(int size){
    HeapBlock * perfectBlock = recDivide(root, size);
    if (!perfectBlock || perfectBlock->size < size)
        return nullptr;

    blockCounter++;
    return perfectBlock->ptr;
}


bool HeapFree(void * blk){
    HeapBlock * freedBlock = recMerge(root, blk);
    if (!freedBlock)
        return false;

    blockCounter--;
    return true;
}


void HeapDone(int * pendingBlk){
    *pendingBlk = blockCounter;
}


#ifndef __PROGTEST__
void test(uint8_t * memPool){
    uint8_t *p0, *p1, *p2, *p3, *p4;
    HeapInit(memPool, 2097152);

    p0 = (uint8_t *)HeapAlloc(100000);
    assert (p0);
    p1 = (uint8_t *)HeapAlloc(500000);
    assert (p1);
    p2 = (uint8_t *)HeapAlloc(1000000);
    assert (p2);
    p3 = (uint8_t *)HeapAlloc(50000);
    assert (p3);
    p4 = (uint8_t *)HeapAlloc(60000);
    assert (p4);

    printf("---------------------------\n");
    printTree(root, 0);

    assert(HeapFree(p0));
    assert(HeapFree(p1));
    assert(HeapFree(p2));
    assert(HeapFree(p3));
    assert(HeapFree(p4));

    printf("---------------------------\n");
    printTree(root, 0);

    int blocks;
    HeapDone(&blocks);
}


int main(){
    static uint8_t memPool[3 * 1048576];
    test(memPool);
    return 0;
}

#endif /* __PROGTEST__ */
