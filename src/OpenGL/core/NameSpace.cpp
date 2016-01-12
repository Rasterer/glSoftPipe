#include "NameSpace.h"

#include <algorithm>
#include <string>
#include <utility>

namespace glsp {

NameSpace::NameSpace()
{
	// Zero was reserved by default
	NameBlock	nb;
	nb.start = 0;
	nb.end = 0;
	mNameBlockLists.push_back(nb);
}

NameSpace::~NameSpace()
{
	for(auto it = mNameHashTable.begin(); it != mNameHashTable.end(); it++)
	{
		if(it->second)
			delete (it->second);
	}
}

bool NameSpace::genNames(unsigned n, unsigned *pNames)
{
	auto iter = mNameBlockLists.begin();
	unsigned end = 0xFFFFFFFF;

	while(iter != mNameBlockLists.end())
	{
		auto next = std::next(iter, 1);
		end = iter->end + n;

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

	for(unsigned i = 0; i < n; i++)
		pNames[i] = end - (n - i) + 1;

	return true;
}

bool NameSpace::deleteNames(unsigned n, const unsigned *pNames)
{
	for(unsigned i = 0; i < n; i++)
	{
		if(!pNames[i])
			continue;

		auto iter = mNameBlockLists.begin();

		while(iter != mNameBlockLists.end())
		{
			if(pNames[i] <= iter->end)
				break;

			iter++;
		}

		// This name doesn't exist in this namespace
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
		return false;

	NameBlockList_t::const_iterator iter = mNameBlockLists.begin();
	while(iter != mNameBlockLists.end())
	{
		if(name <= iter->end)
			return true;

		iter++;
	}

	return false;
}

NameItem* NameSpace::retrieveObject(unsigned name)
{
	NameHashTable_t::const_iterator iter = mNameHashTable.find(name);
	
	if(iter != mNameHashTable.end())
	{
		return iter->second;
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
		// already exist
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

} // namespace glsp
