#include <string>
#include "NameSpace.h"

NameSpace::NameSpace()
{
	// Zero was reserved by default
	NameBlock	nb;
	nb.start = 0;
	nb.end = 0;
	mNameBlockLists.push_back(nb);
}

bool NameSpace::genNames(unsigned n, unsigned *pNames)
{
	NameBlockList_t::iterator iter = mNameBlockLists.begin();

	while(iter != mNameBlockLists.end())
	{
		NameBlockList_t::iterator next = std::next(iter, 1);
		unsigned end = iter->end + n;

		// wrap happen, just return false
		if(end < iter->end)
		{
			return false;
		}

		if(next == mNameBlockLists.end() || end < next->start - 1)
		{
			iter->end = end;
			break;
		}
		else if(end == next->start - 1)
		{
			iter->end = next->end;
			mNameBlockLists.erase(next);
			break;
		}
		else
		{
			iter++;
		}
	}

	for(size_t i = 0; i < n; i++)
		pNames[i] = iter->end - (n - i) + 1;

	return true;
}

bool NameSpace::deleteNames(unsigned n, const unsigned *pNames)
{
	for(size_t i = 0; i < n; i++)
	{
		NameBlockList_t::iterator iter = mNameBlockLists.begin();

		if(!pNames[i])
			continue;

		while(iter != mNameBlockLists.end())
		{
			if(pNames[i] <= iter->end)
				break;

			iter++;
		}

		if(iter == mNameBlockLists.end())
			continue;

		if(iter->start == iter->end)
		{
			mNameBlockLists.erase(iter);
		}
		else if(iter->start == pNames[i])
		{
			iter->start++;
		}
		else if(iter->end == pNames[i])
		{
			iter->end--;
		}
		else
		{
			NameBlock nb;
			nb.start = iter->start;
			nb.end = pNames[i] - 1;
			iter->start = pNames[i] + 1;
			mNameBlockLists.insert(iter, nb);
		}
	}

	return true;
}

bool NameSpace::validate(unsigned name)
{
	if(!name)
		return NULL;

	NameBlockList_t::const_iterator iter = mNameBlockLists.begin();
	while(iter != mNameBlockLists.end())
	{
		if(name <= iter->end)
			return &(*iter);

		iter++;
	}

	return NULL;
}

NameItem * NameSpace::retrieveObject(unsigned name)
{
	NameHashTable_t::const_iterator iter = mNameHashTable.find(name);
	
	if(iter != mNameHashTable.end())
	{
		return mNameHashTable[name];
	}
	else
	{
		return NULL;
	}
}

bool NameSpace::insertObject(NameItem *pNameItem)
{
	unsigned name = pNameItem->getName();
	if(!validate(name))
		return false;

	NameHashTable_t::const_iterator iter = mNameHashTable.find(name);
	
	if(iter != mNameHashTable.end())
	{
		return false;
	}
	else
	{
		mNameHashTable[name] = pNameItem;
		return true;
	}
}

bool NameSpace::removeObject(NameItem *pNameItem)
{
	unsigned name = pNameItem->getName();
	if(!validate(name))
		return false;

	NameHashTable_t::const_iterator iter = mNameHashTable.find(name);
	
	if(iter != mNameHashTable.end())
	{
		mNameHashTable.erase(iter);
		return true;
	}
	else
	{
		return false;
	}
}
