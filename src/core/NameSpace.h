#pragma once

#define NS_HASH_TABLE_SIZE 11

struct NameItem
{
	unsigned int mName;
	NameItem *mNext;
};

struct NameBlock
{
	unsigned start;
	unsigned end;
	NameBlock *mPrev;
	NameBlock *mNext;
};

class NameSpace
{
public:
	NameSpace();
	bool genNames(unsigned n, unsigned *pNames);
	bool deleteNames(unsigned n, unsigned *pNames);
	NameBlock * validate(unsigned name);
	NameItem * retrieveObject(unsigned name);
	bool insertObject(NameItem *pNameItem);
	bool removeObject(NameItem *pNameItem);

private:
	NameItem *mNameHashTable[NS_HASH_TABLE_SIZE];
	NameBlock *mNameBlocks;
};
