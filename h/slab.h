#ifndef _slab_h_
#define _slab_h_
#include "vm_declarations.h"

class KernelSystem;
template <typename T>
class Slab
{
public:
	Slab(void* memory)
	{
		firstFree = (FreeSlot<T>*)memory;
		firstFree->next = 0;
		nextSlab = 0;
		maxCount = (PAGE_SIZE*KernelSystem::slabSizeInPages) / sizeof(T);
		currentCount = 0;
		memorySpaceStart = memory;
		memorySpaceEnd = (char*)memory + (PAGE_SIZE*KernelSystem::slabSizeInPages);
	}

	T *getObject()
	{
		if (currentCount == maxCount)
			return 0;
		T *object;
		object = (T*)firstFree;
		if (firstFree->next == 0)
		{
			firstFree = (FreeSlot<T>*)(object + 1);
			firstFree->next = 0;
		}
		else
		{
			firstFree = firstFree->next;
		}
		currentCount++;
		memset(object, 0, sizeof(object)); //inicijalizuje nulama
		return object;
	}

	int deleteObject(T* object)
	{
		FreeSlot<T> *oldFirst = firstFree;
		firstFree = (FreeSlot<T>*)object;
		firstFree->next = oldFirst;
		if (currentCount > 0)
			currentCount--;
		if (currentCount == 0)
			return 1;
		return 0;
	}

	int belongsToSlab(T* object)
	{
		return (object < memorySpaceEnd && object >= memorySpaceStart);
	}

	int isFree()
	{
		return currentCount != maxCount;
	}

	Slab * getNextSlab()
	{
		return this->nextSlab;
	}

	void setNextSlab(Slab *slab)
	{
		this->nextSlab = slab;
	}

private:

	template <typename T>
	struct FreeSlot
	{
		FreeSlot* next;
	};

	FreeSlot<T> *firstFree;
	Slab *nextSlab;
	unsigned long maxCount;
	unsigned long currentCount;
	PhysicalAddress memorySpaceStart;
	PhysicalAddress memorySpaceEnd;

};
#endif