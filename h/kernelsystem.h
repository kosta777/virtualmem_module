#ifndef _kernelsystem_h_
#define _kernelsystem_h_
#include "system.h"
#include "process.h"
#include "vm_declarations.h"
#include "cache.h"
#include "frametableentry.h"
#include <unordered_map>
#include "sharedsegmentmapentry.h"
#include <string.h>
#include "bitmap.h"
#include <mutex>

class Pmt0;
class Pmt1;

class KernelSystem
{
public:
	KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
		PhysicalAddress pmtSpace, PageNum pmtSpaceSize,
		Partition* partition);

	~KernelSystem();
	Process* createProcess();

	Time periodicJob();
	// Hardware job
	Status access(ProcessId pid, VirtualAddress address, AccessType type);

	void *getSlabMemory();
	void returnSlabMemory(void *memory);
	unsigned long getNumberOfFreeSlabs();

	Pmt0* getPMT0PublicLock();
	Pmt1* getPMT1();
	Pmt1* getPMT1KernelSystem();
	Bitmap* getBitmap();
	unsigned long allocateClusterForPage();
	unsigned long allocateClusterForPageNoLock();
	Status writeToCluster(unsigned long clusterNum, const char* content);
	Status writeToCluster(unsigned long clusterNum, unsigned long frameNum);
	Status readFromCluster(unsigned long clusterNum, unsigned long frameNum);
	Status readFromCluster(unsigned long clusterNum, char* location);
	const char* readFromPage(unsigned long frameNum);
	FrameTableEntry* getFrameTable();
	PhysicalAddress getPageAddress(unsigned long pageNum);
	unsigned long getFrameTableCount();

	bool sharedSegmentExists(std::string) const;
	void addToSharedSegment(std::string, ProcessId, VirtualAddress);
	void removeFromSharedSegment(std::string, ProcessId);

	Status createSharedSegment(const char* name, PageNum segmentSize, AccessType);
	Pmt0 *getSharedSegment(const char* name) const;
	VirtualAddress getSharedSegmentVirtualAddress(std::string, ProcessId);
	void deleteSharedSegment(std::string);
	void addClonnedProcess(ProcessId, KernelProcess*);


	static const PageNum slabSizeInPages = 1;
private:
	void returnSlabMemoryPrivate(void *memory);
	struct FreeSlot
	{
		FreeSlot* next;
	};

	Pmt0* getPMT0();
	std::mutex globalMutex;
	static ProcessId freeID;
	Cache<Pmt0> *pmt0Cache;
	Cache<Pmt1> *pmt1Cache;
	FrameTableEntry *frameTable;
	unsigned long frameTableCount;
	std::unordered_map<std::string, SharedSegmentMapEntry*> *sharedSegmentsMap;
	std::unordered_map<ProcessId, KernelProcess*> *processMap;
	Bitmap *clusterBitmap;

	FreeSlot *firstFreeSlot;
	void *pmtMemoryStartAddress;
	PageNum pmtSpaceSizeInPages;
	PhysicalAddress pagesMemoryStartAddress;
	unsigned long freeSlabsCount;
	Partition *swapPartition;
};

#endif