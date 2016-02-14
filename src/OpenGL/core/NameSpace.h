#pragma once

#include <cassert>
#include <list>
#include <unordered_map>


namespace glsp {

class NameSpace;

class NameItem
{
public:
	NameItem();
	virtual ~NameItem();

	// accessors
	unsigned int getName() const { return mName; }
	unsigned int getRefCount() const { return mRefCount; }

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
	NameSpace(const char *name);
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

	const char *mName;
};

} // namespace glsp
