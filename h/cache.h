#ifndef _cache_h_
#define _cache_h_

#include "vm_declarations.h"
#include "slab.h"

class KernelSystem;
template <typename T>
class Cache
{
public:

	Cache(KernelSystem *kernelSystem)
	{
		allSlabs = 0;
		this->kernelSystem = kernelSystem;
	}

	T* getObject()
	{
		T* ret = 0;
		if (allSlabs == 0)
		{
			void *memory = kernelSystem->getSlabMemory();
			if (memory == 0)
				return 0;
			allSlabs = new Slab<T>(memory);
			ret = allSlabs->getObject();
		}
		else
		{
			Slab<T> *s = allSlabs;
			while (s && !s->isFree())
				s = s->getNextSlab();
			if (!s)
			{
				void *memory = kernelSystem->getSlabMemory();
				if (!memory)//Nema vise mesta
					return 0;

				Slab<T>* newSlab = new Slab<T>(memory);
				newSlab->setNextSlab(allSlabs);
				allSlabs = newSlab;
				s = newSlab;
			}
			ret = s->getObject();
		}
		return ret;
	}

	void deleteObject(T* object)
	{
		Slab<T> *s = allSlabs;
		Slab<T> *last = 0;
		while (s && !s->belongsToSlab(object))
		{
			last = s;
			s = s->nextSlab;
		}
		if (!s)
			return;
		if (s->deleteObject(object))//slab is now free
		{
			std::cout << "slot free" << std::endl;
			if (s == allSlabs)
				allSlabs = allSlabs->nextSlab;
			else
				last->nextSlab = s->nextSlab;
			kernelSystem->returnSlabMemory(s->memorySpaceStart);
		}
	}
private:
	Slab<T> *allSlabs;
	KernelSystem *kernelSystem;
};

#endif