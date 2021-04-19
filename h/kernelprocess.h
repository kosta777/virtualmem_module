#ifndef _kernelprocess_h_
#define _kernelprocess_h_
#include "process.h"
#include "vm_declarations.h"
#include <mutex>
#include <unordered_map>

class KernelProcess
{
public:
	KernelProcess(ProcessId pid);
	~KernelProcess();
	ProcessId getProcessId() const;
	Status createSegment(VirtualAddress startAddress, PageNum segmentSize, unsigned long flags);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize, unsigned long flags, void* content);
	Status deleteSegment(VirtualAddress startAddress, bool isShared);
	Status pageFault(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
	unsigned isVirtualAddressValidPublic(VirtualAddress address);
	unsigned isVirtualAddressValidPrivate(VirtualAddress address);
	Pmt1Entry* getPmt1Entry(VirtualAddress address);

	Status createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags);
	Status disconnectSharedSegment(const char* name);
	Status disconnectSharedSegmentNoLock(VirtualAddress vadr);
	Status deleteSharedSegment(const char* name);
	Process* clone(ProcessId pid);

	void printPmt0();
	Pmt0 *pmt0;
private:
	VirtualAddress getVirtualAddressFromPmt(unsigned long pmt0Address, unsigned long pmt1Address);
	Status createSharedSegmentUnlocked(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags);
	Status deleteSegmentPrivate(VirtualAddress startAddress);
	std::mutex mutex;
	Status allocateSegment(VirtualAddress startAddress, PageNum segmentSize, unsigned long flags,bool fill, void* content, bool shared = false);

	std::unordered_map<VirtualAddress, std::string> *sharedSegmentsNameMap;

	ProcessId pid;
	KernelSystem *kernelSystem;
	friend class KernelSystem;
};

#endif