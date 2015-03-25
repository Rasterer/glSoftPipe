#include <string>
#include "NameSpace.h"

NameSpace::NameSpace()
{
	for(size_t i = 0; i < NS_HASH_TABLE_SIZE; i++)
	{
		mNameHashTable[i] = NULL;
	}

	// Zero was reserved by default
	mNameBlocks = new NameBlock();
	mNameBlocks->start = 0;
	mNameBlocks->end = 0;
	mNameBlocks->mNext = NULL;
}

bool NameSpace::genNames(unsigned n, unsigned *pNames)
{
	NameBlock *pNameBlock = mNameBlocks;

	while(pNameBlock)
	{
		unsigned end = pNameBlock->end + n;
		NameBlock *pNext = pNameBlock->mNext;

		// wrap happen, just return false
		if(end < pNameBlock->end)
		{
			return false;
		}

		if(pNext == NULL || end < pNext->start - 1)
		{
			pNameBlock->end = end;
			break;
		}
		else if(end == pNext->start - 1)
		{
			pNameBlock->end = pNext->end;
			pNameBlock->mNext = pNext->mNext;
			delete pNext;
		}
		else
		{
			pNameBlock = pNameBlock->mNext;
		}
	}

	for(size_t i = 0; i < n; i++)
		pNames[i] = pNameBlock->end - (n - i) + 1;

	return true;
}

bool NameSpace::deleteNames(unsigned n, const unsigned *pNames)
{
	for(size_t i = 0; i < n; i++)
	{
		NameBlock *pNameBlock = validate(pNames[i]);
		if(!pNameBlock)
			continue;

		if(pNameBlock->start == pNameBlock->end)
		{
			NameBlock *pTmpBlock = mNameBlocks;
			NameBlock *pNextBlock;

			while(pTmpBlock)
			{
				pNextBlock = pTmpBlock->mNext;
				if(pNextBlock == pNameBlock)
				{
					pTmpBlock->mNext = pNextBlock->mNext;
					delete pNameBlock;
					break;
				}
			}
		}
		else if(pNameBlock->start == pNames[i])
		{
			pNameBlock->start++;
		}
		else if(pNameBlock->end == pNames[i])
		{
			pNameBlock->end--;
		}
		else
		{
			NameBlock *pNewBlock = new NameBlock();
			pNewBlock->start = pNames[i] + 1;
			pNewBlock->end = pNameBlock->end;
			pNameBlock->end = pNames[i] - 1;
			pNewBlock->mNext = pNameBlock->mNext;
			pNameBlock->mNext = pNewBlock;
		}
	}

	return true;
}

NameBlock * NameSpace::validate(unsigned name)
{
	NameBlock *pNameBlock = mNameBlocks;

	if(!name)
		return NULL;

	while(pNameBlock)
	{
		if(name <= pNameBlock->end)
			return pNameBlock;

		pNameBlock = pNameBlock->mNext;
	}

	return NULL;
}

NameItem * NameSpace::retrieveObject(unsigned name)
{
	unsigned ht = name % NS_HASH_TABLE_SIZE;
	NameItem *pNameItem = mNameHashTable[ht];

	while(pNameItem)
	{
		if(pNameItem->mName == name)
			return pNameItem;

		pNameItem = pNameItem->mNext;
	}

	return NULL;
}

bool NameSpace::insertObject(NameItem *pNameItem)
{
	if(!validate(pNameItem->mName))
		return false;

	unsigned ht = pNameItem->mName % NS_HASH_TABLE_SIZE;
	NameItem *pEntry = mNameHashTable[ht];

	pNameItem->mNext = pEntry;
	mNameHashTable[ht] = pNameItem;

	return true;
}

bool NameSpace::removeObject(NameItem *pNameItem)
{
	if(!validate(pNameItem->mName))
		return false;

	unsigned ht = pNameItem->mName % NS_HASH_TABLE_SIZE;
	NameItem *pEntry = mNameHashTable[ht];
	NameItem *pNext;

	if(pEntry == pNameItem)
	{
		mNameHashTable[ht] = pEntry->mNext;
		return true;
	}

	while(pEntry)
	{
		pNext = pEntry->mNext;
		if(pNext == pNameItem)
		{
			pEntry->mNext = pNext->mNext;
			return true;
		}
	}

	return false;
}
