#pragma once

#include <cassert>
#include <list>
#include <unordered_map>


namespace glsp {

class NameItem
{
public:
	NameItem():
		mName(0),
		mRefCount(1)
	{ }
	virtual ~NameItem() { }

	// accessors
	unsigned getName() const { return mName; }

	// mutators
	void setName(unsigned name) { mName = name; }

	void IncRef()
	{
		assert(mRefCount >= 1);
		mRefCount++;
	}

	void DecRef()
	{
		assert(mRefCount >= 1);
		if (!(--mRefCount))
		{
			delete this;
		}
	}

private:
	unsigned int mName;
	unsigned int mRefCount;
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

} // namespace glsp
