#pragma once

#include <list>
#include <unordered_map>
#include "common/glsp_defs.h"


NS_OPEN_GLSP_OGL()

class NameItem
{
public:
	NameItem(): mName(0) { }
	virtual ~NameItem() { }

	// accessors
	unsigned getName() const { return mName; }

	// mutators
	void setName(unsigned name) { mName = name; }

private:
	unsigned int mName;
};

struct NameBlock
{
	unsigned start;	// included
	unsigned end;	// included
};

class NameSpace
{
public:
	NameSpace();
	virtual ~NameSpace();

	bool genNames(unsigned n, unsigned *pNames);
	bool deleteNames(unsigned n, const unsigned *pNames);

	bool validate(unsigned name);
	NameItem* retrieveObject(unsigned name);

	bool insertObject(NameItem *pNameItem);
	bool removeObject(NameItem *pNameItem);

private:
	typedef std::unordered_map<unsigned, NameItem *> NameHashTable_t;
	typedef std::list<NameBlock>	NameBlockList_t;

	NameHashTable_t	mNameHashTable;
	NameBlockList_t	mNameBlockLists;
};

NS_CLOSE_GLSP_OGL()