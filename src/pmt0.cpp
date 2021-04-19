#include "pmt0.h"
#include "part.h"
#include <iostream>

bool Pmt0::haveFreeFromAddress(unsigned pmt0Address, unsigned pmt1Address, unsigned count, unsigned * pagesReqNum)
{
	unsigned pmtsRequired = 0;
	while (count > 0)
	{
		//if there is no pmt1 allocated at position pmt0Address
		if (entries[pmt0Address] == 0)
		{
			pmtsRequired++;
			if ((Pmt1::entriesNumber - pmt1Address) > count)
			{
				*pagesReqNum = pmtsRequired;
				return 1;
			}

			count -= (Pmt1::entriesNumber-pmt1Address);
			//std::cout << "smanjio count za " << (Pmt1::entriesNumber - pmt1Address) << std::endl;
			pmt0Address++;
			pmt1Address = 0;
			continue;
		}

		if (getEntry(pmt0Address, pmt1Address)->isUsed())
		{
			*pagesReqNum = pmtsRequired;
			return 0;
		}
		pmt1Address++;
		if (pmt1Address >= Pmt1::entriesNumber)
		{
			pmt1Address = 0;
			pmt0Address++;
			if (pmt0Address >= Pmt0::entriesNumber)
			{
				*pagesReqNum = pmtsRequired;
				return 0;
			}
		}
		count--;
	}
	*pagesReqNum = pmtsRequired;
	return 1;
}

Status Pmt0::allocateFromShared(KernelSystem *ks, Pmt0 *pmt0Shared, unsigned pmt0Address, unsigned pmt1Address, unsigned count, AccessType accessRights)
{
	unsigned long pmt0SharedAddress = 0;
	unsigned long pmt1SharedAddress = 0;
	if (entries[pmt0Address] == 0)
	{
		entries[pmt0Address] = ks->getPMT1();
		if (entries[pmt0Address] == 0)
			return Status::ERROR;
	}

	//First is done here because of the segmentSize field of Pmt1Entry
	Pmt1Entry *sharedEntry = pmt0Shared->getEntry(pmt0SharedAddress, pmt1SharedAddress);
	if (sharedEntry == 0)
	{
		return Status::ERROR;
	}
	entries[pmt0Address]->entries[pmt1Address] = Pmt1Entry(sharedEntry, accessRights);
	count--;
	pmt1Address++;
	pmt1SharedAddress++;
	if (pmt1Address >= Pmt1::entriesNumber)
	{
		pmt1Address = 0;
		pmt0Address++;
	}
	if (pmt1SharedAddress >= Pmt1::entriesNumber)
	{
		pmt1SharedAddress = 0;
		pmt0SharedAddress++;
	}
	while (count > 0)
	{
		if (entries[pmt0Address] == 0)
		{
			entries[pmt0Address] = ks->getPMT1();
			if (entries[pmt0Address] == 0)
				return Status::ERROR;
		}

		Pmt1Entry *sharedEntry = pmt0Shared->getEntry(pmt0SharedAddress, pmt1SharedAddress);
		if (sharedEntry == 0)
			return Status::ERROR;
		entries[pmt0Address]->entries[pmt1Address] = Pmt1Entry(sharedEntry, accessRights);
		pmt1Address++;
		pmt1SharedAddress++;
		if (pmt1Address >= Pmt1::entriesNumber)
		{
			pmt1Address = 0;
			pmt0Address++;
		}
		if (pmt1SharedAddress >= Pmt1::entriesNumber)
		{
			pmt1SharedAddress = 0;
			pmt0SharedAddress++;
		}
		count--;
	}
	return Status::OK;
}

Status Pmt0::allocateFromAddress(KernelSystem *ks, unsigned pmt0Address, unsigned pmt1Address, unsigned count, unsigned long accessRights, bool fill, void* content)
{
	unsigned long currentContentPosition = 0;
	if (entries[pmt0Address] == 0)
	{
		entries[pmt0Address] = ks->getPMT1();
		if (entries[pmt0Address] == 0)
			return Status::ERROR;
	}

	//First is done here because of the segmentSize field of Pmt1Entry
	entries[pmt0Address]->entries[pmt1Address] = Pmt1Entry(accessRights, count, ks->allocateClusterForPage());
	if (fill)
	{
		Status status = ks->writeToCluster(entries[pmt0Address]->entries[pmt1Address].swapSpaceClusterNum, ((const char*)content) + (ClusterSize * (currentContentPosition++)));
		if (status != Status::OK)
			return Status::ERROR;
	}
	count--;
	pmt1Address++;
	if (pmt1Address >= Pmt1::entriesNumber)
	{
		pmt1Address = 0;
		pmt0Address++;
	}
	while (count > 0)
	{
		if (entries[pmt0Address] == 0)
		{
			entries[pmt0Address] = ks->getPMT1();
			if (entries[pmt0Address] == 0)
				return Status::ERROR;
		}
		entries[pmt0Address]->entries[pmt1Address] = Pmt1Entry(accessRights, 0, ks->allocateClusterForPage());
		if (fill)
		{
			Status status = ks->writeToCluster(entries[pmt0Address]->entries[pmt1Address].swapSpaceClusterNum, ((const char*)content) + (ClusterSize * (currentContentPosition++)));
			if (status != Status::OK)
				return Status::ERROR;
		}
		pmt1Address++;
		if (pmt1Address >= Pmt1::entriesNumber)
		{
			pmt1Address = 0;
			pmt0Address++;
		}
		count--;
	}
	return Status::OK;
}

//kao obican allocate samo sto zove getPmt1() bez zakljucavanja kernelsystema zato sto se odatle zove
Status Pmt0::allocateFromAddressForKernelSystem(KernelSystem *ks, unsigned pmt0Address, unsigned pmt1Address, unsigned count, unsigned long accessRights, bool fill, void* content)
{
	unsigned long currentContentPosition = 0;
	if (entries[pmt0Address] == 0)
	{
		entries[pmt0Address] = ks->getPMT1KernelSystem();
		if (entries[pmt0Address] == 0)
			return Status::ERROR;
	}

	//First is done here because of the segmentSize field of Pmt1Entry
	entries[pmt0Address]->entries[pmt1Address] = Pmt1Entry(accessRights, count, ks->allocateClusterForPageNoLock());
	if (fill)
	{
		Status status = ks->writeToCluster(entries[pmt0Address]->entries[pmt1Address].swapSpaceClusterNum, ((const char*)content) + (ClusterSize * (currentContentPosition++)));
		if (status != Status::OK)
			return Status::ERROR;
	}
	count--;
	pmt1Address++;
	if (pmt1Address >= Pmt1::entriesNumber)
	{
		pmt1Address = 0;
		pmt0Address++;
	}
	while (count > 0)
	{
		if (entries[pmt0Address] == 0)
		{
			entries[pmt0Address] = ks->getPMT1KernelSystem();
			if (entries[pmt0Address] == 0)
				return Status::ERROR;
		}
		entries[pmt0Address]->entries[pmt1Address] = Pmt1Entry(accessRights, 0, ks->allocateClusterForPageNoLock());
		if (fill)
		{
			Status status = ks->writeToCluster(entries[pmt0Address]->entries[pmt1Address].swapSpaceClusterNum, ((const char*)content) + (ClusterSize * (currentContentPosition++)));
			if (status != Status::OK)
				return Status::ERROR;
		}
		pmt1Address++;
		if (pmt1Address >= Pmt1::entriesNumber)
		{
			pmt1Address = 0;
			pmt0Address++;
		}
		count--;
	}

	return Status::OK;
}


Pmt1Entry* Pmt0::getEntry(VirtualAddress vadr)
{
	unsigned offset = vadr & ((1ul << 10) - 1);
	vadr = vadr >> 10;
	unsigned pmt1Address = vadr & ((1ul << 6) - 1);
	vadr = vadr >> 6;
	unsigned pmt0Address = vadr & ((1ul << 8) - 1);

	if (offset != 0)
		return 0;
	if (pmt0Address >= Pmt0::entriesNumber)
		return 0;
	if (pmt1Address >= Pmt1::entriesNumber)
		return 0;

	if (entries[pmt0Address] == 0)
		return 0;

	if (!entries[pmt0Address]->entries[pmt1Address].isShared())
		return &entries[pmt0Address]->entries[pmt1Address];
	else
		return entries[pmt0Address]->entries[pmt1Address].getSharedEntry();
}

Pmt1Entry* Pmt0::getEntry(unsigned long pmt0Address, unsigned long pmt1Address)
{
	if (pmt0Address >= Pmt0::entriesNumber)
		return 0;
	if (pmt1Address >= Pmt1::entriesNumber)
		return 0;

	if (entries[pmt0Address] == 0)
		return 0;

	if (!entries[pmt0Address]->entries[pmt1Address].isShared())
		return &entries[pmt0Address]->entries[pmt1Address];
	else
		return entries[pmt0Address]->entries[pmt1Address].getSharedEntry();
}

AccessType Pmt0::getAccessRights(VirtualAddress vadr)
{
	unsigned offset = vadr & ((1ul << 10) - 1);
	vadr = vadr >> 10;
	unsigned pmt1Address = vadr & ((1ul << 6) - 1);
	vadr = vadr >> 6;
	unsigned pmt0Address = vadr & ((1ul << 8) - 1);

	return entries[pmt0Address]->entries[pmt1Address].getAccessRights();
}