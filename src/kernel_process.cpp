#include "kernelprocess.h"
#include "vm_declarations.h"
#include "pmt0.h"
#include "pmt1.h"
#include "kernelsystem.h"
#include <cmath>
#include <iostream>
#include <mutex>
#include <cstring>

KernelProcess::KernelProcess(ProcessId pid)
{
	this->pid = pid;
	sharedSegmentsNameMap = new std::unordered_map<VirtualAddress, std::string>();
}

//TODO PAZI OVO KOD DELJENIH SEGMENATA
KernelProcess::~KernelProcess()
{
	if (pmt0 != 0)
	{
		for (int i = 0; i < Pmt0::entriesNumber; i++)
			if (pmt0->entries[i] != 0)
			{
				Pmt1* pmt1 = pmt0->entries[i];
				for (int j = 0; j < Pmt1::entriesNumber; j++)
					if (pmt1->entries[j].isUsed())
					{
						//Oslobodi klaster
						kernelSystem->getBitmap()->setBit(pmt1->entries[j].swapSpaceClusterNum, 0);
						//Oslobodi stranicu u ramu
						if (pmt1->entries[j].isInPhysicalMemory())
							kernelSystem->getFrameTable()[pmt1->entries[j].getFrameNum()].setFree(true);
					}
				kernelSystem->returnSlabMemory(pmt1);
			}
		kernelSystem->returnSlabMemory(pmt0);
	}
	delete sharedSegmentsNameMap;
	//std::cout << "destruktor procesa" << std::endl;
}

ProcessId KernelProcess::getProcessId() const
{
	return this->pid;
}

Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize,
	unsigned long flags)
{
	std::lock_guard<std::mutex> lock(mutex);
	return allocateSegment(startAddress, segmentSize, flags, false, 0);
}
Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize,
	unsigned long flags, void* content)
{
	std::lock_guard<std::mutex> lock(mutex);
	return allocateSegment(startAddress, segmentSize, flags, true, content);
}

Status KernelProcess::allocateSegment(VirtualAddress startAddress, PageNum segmentSize, unsigned long flags, bool fill, void* content, bool shared)
{
	unsigned offset = startAddress & ((1ul<<10) - 1);
	startAddress = startAddress >> 10;
	unsigned pmt1Address = startAddress & ((1ul<<6) - 1);
	startAddress = startAddress >> 6;
	unsigned pmt0Address = startAddress & ((1ul<<8) - 1);

	std::cout << "Zahtev za alokacijom. pmt0=" << pmt0Address << " pmt1=" << pmt1Address << " offset=" << offset << std::endl;

	if (segmentSize == 0)
		return Status::ERROR;
	if (offset != 0)
		return Status::ERROR;
	if (pmt0Address >= Pmt0::entriesNumber)
		return Status::ERROR;
	if (pmt1Address >= Pmt1::entriesNumber)
		return Status::ERROR;

	unsigned pagesReqNum = 0;
	//Provera da li ima segmentSize slobodnih sukcesivnih stranica
	if (!pmt0->haveFreeFromAddress(pmt0Address, pmt1Address, segmentSize, &pagesReqNum))
		return Status::ERROR;

	//std::cout << "Potrebno stranica za sve pmt1= " << pagesReqNum << std::endl;
	//std::cout << "Slobodno stranica= " << kernelSystem->getNumberOfFreeSlabs() << std::endl;
	if (kernelSystem->getNumberOfFreeSlabs() < pagesReqNum) // RADI SAMO ZA VELICINU SLABA = VELICINU STRANICE = VELICINA pmt1
		return Status::ERROR;

	Status s = pmt0->allocateFromAddress(kernelSystem, pmt0Address, pmt1Address, segmentSize, flags, fill, content);
	if (s != Status::OK)
		return Status::ERROR;
	//std::cout << "ALOCIRAO SEGMENT " << pid << " " << pmt0Address << " " << pmt1Address << " " << segmentSize << " " << std::endl;
	//std::cout << pmt0->getEntry(pmt0Address, pmt1Address)->swapSpaceClusterNum << std::endl;
	return Status::OK;
}

Status KernelProcess::deleteSegment(VirtualAddress startAddress, bool isShared = false)
{
	std::lock_guard<std::mutex> lock(mutex);
	unsigned offset = startAddress & ((1ul << 10) - 1);
	startAddress = startAddress >> 10;
	unsigned pmt1Address = startAddress & ((1ul << 6) - 1);
	startAddress = startAddress >> 6;
	unsigned pmt0Address = startAddress & ((1ul << 8) - 1);

	if (offset != 0)
		return Status::ERROR;
	if (pmt0Address >= Pmt0::entriesNumber)
		return Status::ERROR;
	if (pmt1Address >= Pmt1::entriesNumber)
		return Status::ERROR;
	if (pmt0->getEntry(pmt0Address, pmt1Address)->isShared())
		return Status::ERROR;

	//std::cout << "Zahtev za brisanjem segmenta na adresi " << startAddress << std::endl;

	unsigned long numberOfPages = pmt0->getEntry(pmt0Address, pmt1Address)->segmentSize;
	for (int i = 0; i < numberOfPages; i++)
	{
		//oslobadjanje stranice u ramu (ako je ucitana)
		if(!isShared && pmt0->getEntry(pmt0Address, pmt1Address)->isInPhysicalMemory())
			kernelSystem->getFrameTable()[pmt0->getEntry(pmt0Address, pmt1Address)->getFrameNum()].setFree(true);
		//oslobadjanje klastera na disku
		if (!isShared)
			kernelSystem->getBitmap()->setBit(pmt0->getEntry(pmt0Address, pmt1Address)->swapSpaceClusterNum, 0);
		if (!isShared)
			pmt0->getEntry(pmt0Address, pmt1Address)->setUnused();
		else
			pmt0->entries[pmt0Address]->entries[pmt1Address].setUnused();
		pmt1Address++;
		if (pmt1Address >= Pmt1::entriesNumber)
		{
			pmt1Address = 0;
			pmt0Address++;
		}
	}
	return Status::OK;
}

Status KernelProcess::deleteSegmentPrivate(VirtualAddress startAddress)
{
	unsigned offset = startAddress & ((1ul << 10) - 1);
	startAddress = startAddress >> 10;
	unsigned pmt1Address = startAddress & ((1ul << 6) - 1);
	startAddress = startAddress >> 6;
	unsigned pmt0Address = startAddress & ((1ul << 8) - 1);

	if (offset != 0)
		return Status::ERROR;
	if (pmt0Address >= Pmt0::entriesNumber)
		return Status::ERROR;
	if (pmt1Address >= Pmt1::entriesNumber)
		return Status::ERROR;
	if (pmt0->getEntry(pmt0Address, pmt1Address)->isShared())
		return Status::ERROR;

	//std::cout << "Zahtev za brisanjem segmenta na adresi " << startAddress << std::endl;

	unsigned long numberOfPages = pmt0->getEntry(pmt0Address, pmt1Address)->segmentSize;
	for (int i = 0; i < numberOfPages; i++)
	{
		//ne treba preko funkcije za dohvatanje nego bukvalno ovako jer mi treba realni entry ne metaforicki
		pmt0->entries[pmt0Address]->entries[pmt1Address].setUnused();
		pmt1Address++;
		if (pmt1Address >= Pmt1::entriesNumber)
		{
			pmt1Address = 0;
			pmt0Address++;
		}
	}
	return Status::OK;
}

Status KernelProcess::pageFault(VirtualAddress address)
{
	std::lock_guard<std::mutex> lock(mutex);
	//std::cout << "Page fault solve request" << std::endl;
	unsigned offset = address & ((1ul << 10) - 1);
	address = address >> 10;
	unsigned pmt1Address = address & ((1ul << 6) - 1);
	address = address >> 6;
	unsigned pmt0Address = address & ((1ul << 8) - 1);

	if (pmt0Address >= Pmt0::entriesNumber)
		return Status::ERROR;
	if (pmt1Address >= Pmt1::entriesNumber)
		return Status::ERROR;
	if (pmt0->entries[pmt0Address] == 0 || !pmt0->getEntry(pmt0Address, pmt1Address)->isUsed())
		return Status::ERROR;

	unsigned long framesCount = kernelSystem->getFrameTableCount();
	FrameTableEntry *frameTable = kernelSystem->getFrameTable();
	unsigned long targetFrame = 0;
	bool foundFrame = false;
	for(unsigned long i=0;i<framesCount;i++)
		if (frameTable[i].isFree())
		{
			foundFrame = true;
			targetFrame = i;
			break;
		}

	if (!foundFrame)
	{
	//	std::cout << "ALL FRAMES FULL" << std::endl;
		//find frame with 
		unsigned long min = -1;
		unsigned long ind = -1;
		for (unsigned long i = 0; i < framesCount; i++)
			if (frameTable[i].getReferencedBitsValue() < min)
			{
				min = frameTable[i].getReferencedBitsValue();
				ind = i;
			}
		if (ind == -1)
			ind = 0;
		
		//Pronasao stranicu za izbacivanje sad treba izbaciti
		frameTable[ind].pmt1entry->setUnloaded();
		if (frameTable[ind].pmt1entry->isDirty())
		{
			Status status = kernelSystem->writeToCluster(frameTable[ind].pmt1entry->swapSpaceClusterNum, frameTable[ind].pmt1entry->getFrameNum());
			if (status != Status::OK)
				return Status::ERROR;
		}
		targetFrame = ind;
	}
	//std::cout << "Frame found " << targetFrame << std::endl;
	//prepisi stranicu sa diska u memoriju
	frameTable[targetFrame].setFree(false);
	frameTable[targetFrame].pmt1entry = pmt0->getEntry(pmt0Address, pmt1Address);
	pmt0->getEntry(pmt0Address, pmt1Address)->setInPhysicalMemory(targetFrame);
	Status s = kernelSystem->readFromCluster(pmt0->getEntry(pmt0Address, pmt1Address)->swapSpaceClusterNum, targetFrame);
	if (s != Status::OK)
		return Status::ERROR;

	return Status::OK;
}

PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address)
{
	std::lock_guard<std::mutex> lock(mutex);
	unsigned offset = address & ((1ul << 10) - 1);
	address = address >> 10;
	unsigned pmt1Address = address & ((1ul << 6) - 1);
	address = address >> 6;
	unsigned pmt0Address = address & ((1ul << 8) - 1);

	if (pmt0Address >= Pmt0::entriesNumber)
		return 0;
	if (pmt1Address >= Pmt1::entriesNumber)
		return 0;

	if (pmt0->entries[pmt0Address] == 0)
		return 0;
	if (!pmt0->getEntry(pmt0Address, pmt1Address)->isUsed())
		return 0;
	if (!pmt0->getEntry(pmt0Address, pmt1Address)->isInPhysicalMemory())
		return 0;

	return kernelSystem->getPageAddress(pmt0->getEntry(pmt0Address, pmt1Address)->getFrameNum());
}

unsigned KernelProcess::isVirtualAddressValidPublic(VirtualAddress address)
{
	std::lock_guard<std::mutex> lock(mutex);
	unsigned offset = address & ((1ul << 10) - 1);
	address = address >> 10;
	unsigned pmt1Address = address & ((1ul << 6) - 1);
	address = address >> 6;
	unsigned pmt0Address = address & ((1ul << 8) - 1);

	if (pmt0Address >= Pmt0::entriesNumber)
		return 0;
	if (pmt1Address >= Pmt1::entriesNumber)
		return 0;

	if (pmt0->entries[pmt0Address] == 0)
		return 0;
	if (!pmt0->getEntry(pmt0Address, pmt1Address)->isUsed())
		return 0;

	return 1;
}

unsigned KernelProcess::isVirtualAddressValidPrivate(VirtualAddress address)
{
	unsigned offset = address & ((1ul << 10) - 1);
	address = address >> 10;
	unsigned pmt1Address = address & ((1ul << 6) - 1);
	address = address >> 6;
	unsigned pmt0Address = address & ((1ul << 8) - 1);

	if (pmt0Address >= Pmt0::entriesNumber)
		return 0;
	if (pmt1Address >= Pmt1::entriesNumber)
		return 0;

	if (pmt0->entries[pmt0Address] == 0)
		return 0;
	if (!pmt0->getEntry(pmt0Address, pmt1Address)->isUsed())
		return 0;

	return 1;
}

Pmt1Entry* KernelProcess::getPmt1Entry(VirtualAddress address)
{
	std::lock_guard<std::mutex> lock(mutex);
	if (!isVirtualAddressValidPrivate(address))
		return 0;

	unsigned offset = address & ((1ul << 10) - 1);
	address = address >> 10;
	unsigned pmt1Address = address & ((1ul << 6) - 1);
	address = address >> 6;
	unsigned pmt0Address = address & ((1ul << 8) - 1);

	return pmt0->getEntry(pmt0Address, pmt1Address);
}

void KernelProcess::printPmt0()
{
	std::lock_guard<std::mutex> lock(mutex);
	for (int i = 0; i < Pmt0::entriesNumber; i++)
		std::cout << pmt0->entries[i] << std::endl;
}

Status KernelProcess::createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags)
{
	std::lock_guard<std::mutex> lock(mutex);
	VirtualAddress adr = startAddress;

	unsigned offset = startAddress & ((1ul << 10) - 1);
	startAddress = startAddress >> 10;
	unsigned pmt1Address = startAddress & ((1ul << 6) - 1);
	startAddress = startAddress >> 6;
	unsigned pmt0Address = startAddress & ((1ul << 8) - 1);

	//std::cout << "Zahtev za alokacijom deljenog segmenta. pmt0=" << pmt0Address << " pmt1=" << pmt1Address << " offset=" << offset << std::endl;

	if (segmentSize == 0)
		return Status::ERROR;
	if (offset != 0)
		return Status::ERROR;
	if (pmt0Address >= Pmt0::entriesNumber)
		return Status::ERROR;
	if (pmt1Address >= Pmt1::entriesNumber)
		return Status::ERROR;

	unsigned pagesReqNum = 0;
	//Provera da li ima segmentSize slobodnih sukcesivnih stranica
	if (!pmt0->haveFreeFromAddress(pmt0Address, pmt1Address, segmentSize, &pagesReqNum))
		return Status::ERROR;

	Status status;
	if (!kernelSystem->sharedSegmentExists(name))
		kernelSystem->createSharedSegment(name, segmentSize, flags);

	Pmt0* pmt0shared = kernelSystem->getSharedSegment(name);
	Status s = pmt0->allocateFromShared(kernelSystem, pmt0shared, pmt0Address, pmt1Address, segmentSize, flags);
	if (s != Status::OK)
		return Status::ERROR;
	kernelSystem->addToSharedSegment(name, getProcessId(), startAddress);
	sharedSegmentsNameMap->insert({ adr, name });
	return Status::OK;
}

Status KernelProcess::createSharedSegmentUnlocked(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags)
{
	VirtualAddress adr = startAddress;
	unsigned offset = startAddress & ((1ul << 10) - 1);
	startAddress = startAddress >> 10;
	unsigned pmt1Address = startAddress & ((1ul << 6) - 1);
	startAddress = startAddress >> 6;
	unsigned pmt0Address = startAddress & ((1ul << 8) - 1);

	//std::cout << "Zahtev za alokacijom deljenog segmenta. pmt0=" << pmt0Address << " pmt1=" << pmt1Address << " offset=" << offset << std::endl;

	if (segmentSize == 0)
		return Status::ERROR;
	if (offset != 0)
		return Status::ERROR;
	if (pmt0Address >= Pmt0::entriesNumber)
		return Status::ERROR;
	if (pmt1Address >= Pmt1::entriesNumber)
		return Status::ERROR;

	unsigned pagesReqNum = 0;
	//Provera da li ima segmentSize slobodnih sukcesivnih stranica
	if (!pmt0->haveFreeFromAddress(pmt0Address, pmt1Address, segmentSize, &pagesReqNum))
		return Status::ERROR;

	if (!kernelSystem->sharedSegmentExists(name))
		kernelSystem->createSharedSegment(name, segmentSize, flags);

	Pmt0* pmt0shared = kernelSystem->getSharedSegment(name);
	Status s = pmt0->allocateFromShared(kernelSystem, pmt0shared, pmt0Address, pmt1Address, segmentSize, flags);
	if (s != Status::OK)
		return Status::ERROR;
	kernelSystem->addToSharedSegment(name, getProcessId(), startAddress);
	sharedSegmentsNameMap->insert({ adr, name });
	return Status::OK;
}

Status KernelProcess::disconnectSharedSegment(const char* name)
{
	std::lock_guard<std::mutex> lock(mutex);

	VirtualAddress vadr = kernelSystem->getSharedSegmentVirtualAddress(name, this->getProcessId());
	sharedSegmentsNameMap->erase(vadr);
	return deleteSegmentPrivate(vadr);
}

Status KernelProcess::disconnectSharedSegmentNoLock(VirtualAddress vadr)
{
	std::cout << "disconnect" << std::endl;
	return deleteSegmentPrivate(vadr);
}

Status KernelProcess::deleteSharedSegment(const char* name)
{
	std::lock_guard<std::mutex> lock(mutex);

	sharedSegmentsNameMap->erase(kernelSystem->getSharedSegmentVirtualAddress(name, this->getProcessId()));
	kernelSystem->deleteSharedSegment(name);
	return Status::OK;
}

Process* KernelProcess::clone(ProcessId pid)
{
	std::lock_guard<std::mutex> lock(mutex);

	KernelProcess *clonnedProcess = new KernelProcess(pid);
	Process *p = new Process(pid, clonnedProcess);
	clonnedProcess->pmt0 = kernelSystem->getPMT0PublicLock();
	if (clonnedProcess->pmt0 == 0)
	{
		delete clonnedProcess;
		delete p;
		return 0;
	}
	clonnedProcess->kernelSystem = kernelSystem;
	//prolazak kroz pmt0 ovog procesa i kopiranje virtualnog prostora u clonnedProcess
	for (int i = 0; i < Pmt0::entriesNumber; i++)
	{
		if (pmt0->entries[i] != 0)
		{
			for(int j=0;j<Pmt1::entriesNumber;j++)
				if (pmt0->entries[i]->entries[j].isUsed())
				{
					//Ako je shared segment
					if (pmt0->entries[i]->entries[j].isShared())
					{
						unsigned long segmentSize = pmt0->getEntry(i, j)->segmentSize;
						auto it = sharedSegmentsNameMap->find(getVirtualAddressFromPmt(i, j));
						if (it == sharedSegmentsNameMap->end())
							std::cout << "Cant find " << getVirtualAddressFromPmt(i, j) << std::endl;
						AccessType access = pmt0->getEntry(i, j)->getAccessRights();
						clonnedProcess->createSharedSegmentUnlocked(getVirtualAddressFromPmt(i, j), segmentSize, it->second.c_str(), access);

						//kreiranje novog i i j tako sto se na trenutno doda segmentsize i dohvate novi pmt0 i pmt1 ulazi za dalje provere
						j += segmentSize;
						if (j >= Pmt1::entriesNumber)
						{
							i += j / Pmt1::entriesNumber;
							j = j % Pmt1::entriesNumber;
						}
					}
					else
					{
						unsigned long segmentSize = pmt0->getEntry(i, j)->segmentSize;
						//alociranje stranica i klastera
						clonnedProcess->allocateSegment(getVirtualAddressFromPmt(i, j), segmentSize, pmt0->getEntry(i, j)->getAccessRights(), false, 0, false);
						//kopiranje sadrzaja - ako je stranica ucitana onda iz stranice, ako nije onda sa diska
						for (int z = 0; z < segmentSize; z++)
						{
							//ako je u fizickoj memoriji, treba da se inicijalizuje sadrzajem sa stranice
							if (pmt0->getEntry(i, j)->isInPhysicalMemory())
							{
								const char* content = kernelSystem->readFromPage(pmt0->getEntry(i, j)->getFrameNum());
								//pisanje u klaster kopije
								Status status = kernelSystem->writeToCluster(clonnedProcess->pmt0->getEntry(i, j)->swapSpaceClusterNum, content);
								if (status != Status::OK)
								{
									delete clonnedProcess;
									delete p;
									return 0;
								}
							}
							else
							{
								char* content = new char[PAGE_SIZE];
								//citanje sa originalnog klastera
								Status status = kernelSystem->readFromCluster(pmt0->getEntry(i, j)->swapSpaceClusterNum, content);
								if (status != Status::OK)
								{
									delete clonnedProcess;
									delete p;
									return 0;
								}
								//upisivanje u kopirani klaster
								status = kernelSystem->writeToCluster(clonnedProcess->pmt0->getEntry(i, j)->swapSpaceClusterNum, content);
								if (status != Status::OK)
								{
									delete clonnedProcess;
									delete p;
									return 0;
								}
								delete content;
							}
							j++;
							if (j >= Pmt1::entriesNumber)
							{
								j = 0;
								i++;
							}
						}
					}
				}
		}
	}
	kernelSystem->addClonnedProcess(pid, clonnedProcess);
	return p;
}

VirtualAddress KernelProcess::getVirtualAddressFromPmt(unsigned long pmt0Address, unsigned long pmt1Address)
{
	return (pmt0Address << 16) | (pmt1Address << 10);
}
