#include "kernelsystem.h"
#include "slab.h"
#include "process.h"
#include "kernelprocess.h"
#include <unordered_map>
#include "part.h"
#include <iostream>
#include "pmt1entry.h"

ProcessId KernelSystem::freeID = 0;

KernelSystem::KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
	PhysicalAddress pmtSpace, PageNum pmtSpaceSize,
	Partition* partition)
{
	//alociranje Frame tabele na pocetku PMTSpace-a
	frameTable = new (pmtSpace) FrameTableEntry[processVMSpaceSize];
	frameTableCount = processVMSpaceSize;
	std::cout <<  "frameTable pocetak " << frameTable << " broj ulaza u frame table "<<frameTableCount<< std::endl;
	pmtMemoryStartAddress = (char*)pmtSpace + sizeof(FrameTableEntry)*processVMSpaceSize;
	//alociranje Cluster bitmap-e iza Frame Table-a
	unsigned long clusterNum = partition->getNumOfClusters();
	std::cout << "ClusterBitmap pocetak: " << pmtMemoryStartAddress << " size in bytres " << (clusterNum / 8 + ((clusterNum % 8) != 0) )<< std::endl;
	clusterBitmap = new Bitmap(pmtMemoryStartAddress, clusterNum / 8 + ((clusterNum % 8) != 0), clusterNum);
	pmtMemoryStartAddress = (char*)pmtMemoryStartAddress + (clusterNum / 8 + ((clusterNum % 8) != 0));
	//inicijalizacija slab sistema za pmt-ove u ostatku PMT memorije
	unsigned sizeOfFrameAndBitmapInBytes = (char*)pmtMemoryStartAddress - (char*)pmtSpace;
	unsigned sizeofFrameAndBitmapInPages = sizeOfFrameAndBitmapInBytes / PAGE_SIZE + (sizeOfFrameAndBitmapInBytes % PAGE_SIZE != 0);
	std::cout << "size of first two in pages " << sizeofFrameAndBitmapInPages << std::endl;
	pmtMemoryStartAddress = (char*)pmtSpace + sizeofFrameAndBitmapInPages*PAGE_SIZE;
	pmtSpaceSizeInPages = pmtSpaceSize - sizeofFrameAndBitmapInPages;
	std::cout << "pmtmemorystartadress: " << pmtMemoryStartAddress << " size in pages " << pmtSpaceSizeInPages << std::endl;
	firstFreeSlot = (FreeSlot*)pmtMemoryStartAddress;
	firstFreeSlot->next = 0;
	pagesMemoryStartAddress = processVMSpace;
	this->swapPartition = partition;
	freeSlabsCount = pmtSpaceSizeInPages / slabSizeInPages;
	std::cout << "ima mesta za " << freeSlabsCount << " slabova." << std::endl;
	std::cout << "PagesVMSpace pocinje od " << pagesMemoryStartAddress << " i ima mesta za " << processVMSpaceSize << " stranice." << std::endl;
	//Inicijalizacija ostalih stvari
	pmt0Cache = new Cache<Pmt0>(this);
	pmt1Cache = new Cache<Pmt1>(this);
	processMap = new std::unordered_map<ProcessId, KernelProcess*>();
	sharedSegmentsMap = new std::unordered_map<std::string, SharedSegmentMapEntry*>();
}

void* KernelSystem::getSlabMemory()
{
	if (freeSlabsCount == 0)
		return 0;
	freeSlabsCount--;
	void *newSlab = firstFreeSlot;
	if (firstFreeSlot->next == 0)
	{
		firstFreeSlot = firstFreeSlot + PAGE_SIZE*slabSizeInPages/sizeof(FreeSlot*);
		firstFreeSlot->next = 0;
	}
	else
		firstFreeSlot = firstFreeSlot->next;

	//std::cout << "Kernel was asked for Slab Memory from " << newSlab << std::endl;
	memset(newSlab, 0, PAGE_SIZE*slabSizeInPages);
	return newSlab;
}

void KernelSystem::returnSlabMemory(void *memory)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	if (memory >= pmtMemoryStartAddress && memory < (char*)pmtMemoryStartAddress + pmtSpaceSizeInPages*PAGE_SIZE)//what have I done
	{
		//std::cout << "Kernel was returned Slab Memory starting from "<< memory << std::endl;
		FreeSlot *slot = (FreeSlot*)memory;
		slot->next = firstFreeSlot;
		firstFreeSlot = slot;
		freeSlabsCount++;
	}
}

void KernelSystem::returnSlabMemoryPrivate(void *memory)
{
	if (memory >= pmtMemoryStartAddress && memory < (char*)pmtMemoryStartAddress + pmtSpaceSizeInPages*PAGE_SIZE)//what have I done
	{
		//std::cout << "Kernel was returned Slab Memory starting from "<< memory << std::endl;
		FreeSlot *slot = (FreeSlot*)memory;
		slot->next = firstFreeSlot;
		firstFreeSlot = slot;
		freeSlabsCount++;
	}
}

unsigned long KernelSystem::getNumberOfFreeSlabs()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	return freeSlabsCount;
}

KernelSystem::~KernelSystem()
{
	processMap->clear();
	delete pmt0Cache;
	delete pmt1Cache;
	delete processMap;
	delete clusterBitmap;
}

Process* KernelSystem::createProcess()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	Process *newProcess = new Process(freeID++);
	newProcess->pProcess->pmt0 = getPMT0();
	if (newProcess->pProcess->pmt0 == 0)
	{
		delete newProcess;
		return 0;
	}
	std::cout << "Kreiranje novog procesa." << std::endl;
	newProcess->pProcess->kernelSystem = this;
	processMap->insert({ newProcess->getProcessId(), newProcess->pProcess });
	return newProcess;
}

Time KernelSystem::periodicJob()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	for (int i = 0; i < frameTableCount; i++)
		if (!frameTable[i].isFree())
			frameTable[i].shiftReferencedBits();
	return 1000;
}

// Hardware job
Status KernelSystem::access(ProcessId pid, VirtualAddress address, AccessType type)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	std::unordered_map<ProcessId, KernelProcess*>::iterator got = processMap->find(pid);
	if (got == processMap->end())
		return Status::ERROR;
	KernelProcess *kernelProcess = got->second;
	
	if (!kernelProcess->isVirtualAddressValidPublic(address))
		return Status::ERROR;

	AccessType rights = kernelProcess->pmt0->getAccessRights(address);
	if (type == AccessType::READ && rights != AccessType::READ && rights != AccessType::READ_WRITE)
		return Status::ERROR;

	if (type == AccessType::READ_WRITE && rights != AccessType::READ_WRITE)
		return Status::ERROR;

	if (type == AccessType::WRITE && rights != AccessType::WRITE && rights != AccessType::READ_WRITE)
		return Status::ERROR;

	if (type == AccessType::EXECUTE && rights != AccessType::EXECUTE)
		return Status::ERROR;

	if (!kernelProcess->getPmt1Entry(address)->isInPhysicalMemory())
		return Status::PAGE_FAULT;

	if (type == AccessType::WRITE || type == AccessType::READ_WRITE)
		kernelProcess->getPmt1Entry(address)->setDirty();

	frameTable[kernelProcess->getPmt1Entry(address)->getFrameNum()].setReferenced();

	return Status::OK;
}

Pmt0 *KernelSystem::getPMT0()
{
	return pmt0Cache->getObject();
}

Pmt0 *KernelSystem::getPMT0PublicLock()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	return pmt0Cache->getObject();
}

Pmt1 *KernelSystem::getPMT1()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	return pmt1Cache->getObject();
}

Pmt1 *KernelSystem::getPMT1KernelSystem()
{
	return pmt1Cache->getObject();
}

Bitmap* KernelSystem::getBitmap()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	return clusterBitmap;
}

unsigned long KernelSystem::allocateClusterForPage()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	unsigned long num = clusterBitmap->getFreeCluster();
	clusterBitmap->setBit(num, true);
	return num;
}

unsigned long KernelSystem::allocateClusterForPageNoLock()
{
	unsigned long num = clusterBitmap->getFreeCluster();
	clusterBitmap->setBit(num, true);
	return num;
}

Status KernelSystem::writeToCluster(unsigned long clusterNum, unsigned long frameNum)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	int rez = this->swapPartition->writeCluster(clusterNum, (char*)pmtMemoryStartAddress + PAGE_SIZE*frameNum);
	if (rez == 0)
		return Status::ERROR;
	return Status::OK;
}

Status KernelSystem::writeToCluster(unsigned long clusterNum, const char* content)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	int rez = this->swapPartition->writeCluster(clusterNum, content);
	if (rez == 0)
		return Status::ERROR;
	return Status::OK;
}

Status KernelSystem::readFromCluster(unsigned long clusterNum, unsigned long frameNum)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	int rez = this->swapPartition->readCluster(clusterNum, (char*)pagesMemoryStartAddress + PAGE_SIZE*frameNum);
	if (rez == 0)
		return Status::ERROR;
	return Status::OK;
}

Status KernelSystem::readFromCluster(unsigned long clusterNum, char* location)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	int rez = this->swapPartition->readCluster(clusterNum, location);
	if (rez == 0)
		return Status::ERROR;
	return Status::OK;
}

const char* KernelSystem::readFromPage(unsigned long frameNum)
{
	std::lock_guard<std::mutex> lock(globalMutex);

	return (char*)pagesMemoryStartAddress + PAGE_SIZE*frameNum;
}

FrameTableEntry* KernelSystem::getFrameTable()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	return frameTable;
}

unsigned long KernelSystem::getFrameTableCount()
{
	std::lock_guard<std::mutex> lock(globalMutex);
	return frameTableCount;
}

PhysicalAddress KernelSystem::getPageAddress(unsigned long pageNum)
{
	std::lock_guard<std::mutex> lock(globalMutex);
	return (char*)pagesMemoryStartAddress + pageNum*PAGE_SIZE;
}

bool KernelSystem::sharedSegmentExists(std::string name)const
{
	return sharedSegmentsMap->count(name) != 0;
}

//TODO namestiti da pmt0->allocate vraca status ako nema vise slabova il nesto tako
Status KernelSystem::createSharedSegment(const char* name, PageNum segmentSize, AccessType accessType) 
{
	std::lock_guard<std::mutex> lock(globalMutex);

	Pmt0* pmt0 = this->getPMT0();
	if (pmt0 == 0)
		return Status::ERROR;
	Status s = pmt0->allocateFromAddressForKernelSystem(this, 0, 0, segmentSize, accessType, false, 0);
	if (s != Status::OK)
		return Status::ERROR;
	SharedSegmentMapEntry *mapEntry = new SharedSegmentMapEntry(pmt0);
	sharedSegmentsMap->insert({ name, mapEntry });
	std::cout << "pravi se novi shared segment" << std::endl;
	return Status::OK;
}

Pmt0 *KernelSystem::getSharedSegment(const char* name) const
{
	if (!sharedSegmentExists(name))
		return 0;
	return sharedSegmentsMap->find(name)->second->pmt0;
}

void KernelSystem::addToSharedSegment(std::string name, ProcessId pid, VirtualAddress vadr)
{
	std::lock_guard<std::mutex> lock(globalMutex);

	if (sharedSegmentExists(name))
		sharedSegmentsMap->find(name)->second->inc(pid, vadr);
}

void KernelSystem::removeFromSharedSegment(std::string name, ProcessId pid)
{
	std::lock_guard<std::mutex> lock(globalMutex);

	if (sharedSegmentExists(name))
	{
		sharedSegmentsMap->find(name)->second->dec(pid);
		//TODO da li treba da se brise ako ga niko ne referencira?
	}
}

VirtualAddress KernelSystem::getSharedSegmentVirtualAddress(std::string name, ProcessId pid)
{
	std::lock_guard<std::mutex> lock(globalMutex);

	return sharedSegmentsMap->find(name)->second->getVirtualAddressForProcess(pid);
}

void KernelSystem::deleteSharedSegment(std::string name)
{
	std::lock_guard<std::mutex> lock(globalMutex);

	SharedSegmentMapEntry *entry = sharedSegmentsMap->find(name)->second;
	if(entry == 0)
		return;

	std::unordered_map<ProcessId, VirtualAddress> *vadrMap = entry->getProcessMap();
	for (auto it : *vadrMap)
		processMap->find(it.first)->second->disconnectSharedSegmentNoLock(it.second);

	Pmt0* pmt0 = entry->pmt0;
	unsigned long pmt0Address = 0, pmt1Address = 0;
	//oslobadjanje i brisanje pmt-a
	unsigned long numberOfPages = pmt0->getEntry(pmt0Address, pmt1Address)->segmentSize;
	for (int i = 0; i < numberOfPages; i++)
	{
		//oslobadjanje stranice u ramu (ako je ucitana)
		if (pmt0->getEntry(pmt0Address, pmt1Address)->isInPhysicalMemory())
			frameTable[pmt0->getEntry(pmt0Address, pmt1Address)->getFrameNum()].setFree(true);
		//oslobadjanje klastera na disku
		clusterBitmap->setBit(pmt0->getEntry(pmt0Address, pmt1Address)->swapSpaceClusterNum, 0);
		pmt0->getEntry(pmt0Address, pmt1Address)->setUnused();
		pmt1Address++;
		if (pmt1Address >= Pmt1::entriesNumber)
		{
			pmt1Address = 0;
			pmt0Address++;
		}
	}
	returnSlabMemoryPrivate(pmt0);
	//zove destruktor
	sharedSegmentsMap->erase(name);
}


void KernelSystem::addClonnedProcess(ProcessId pid, KernelProcess* p)
{
	processMap->insert({ pid, p });
}