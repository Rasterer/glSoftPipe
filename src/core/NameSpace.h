#pragma once

#include <unordered_map>
#include <list>

using namespace std;

struct NameItem
{
	unsigned int mName;
};

struct NameBlock
{
	unsigned start;
	unsigned end;
};

class NameSpace
{
public:
	NameSpace();
	bool genNames(unsigned n, unsigned *pNames);
	bool deleteNames(unsigned n, const unsigned *pNames);
	const NameBlock *validate(unsigned name);
	NameItem *retrieveObject(unsigned name);
	bool insertObject(NameItem *pNameItem);
	bool removeObject(NameItem *pNameItem);

private:
	typedef unordered_map<unsigned, NameItem *> NameHashTable_t;
	typedef list<NameBlock>	NameBlockList_t;
	NameHashTable_t	mNameHashTable;
	NameBlockList_t	mNameBlockLists;
};
