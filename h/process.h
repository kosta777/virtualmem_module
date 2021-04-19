#ifndef _proces_h_
#define _proces_h_
#include "vm_declarations.h"
#include "pmt0.h"

class KernelProcess;
class System;
class KernelSystem;
class Process 
{
public:
	Process(ProcessId pid);
	~Process();
	ProcessId getProcessId() const;
	Status createSegment(VirtualAddress startAddress, PageNum segmentSize,
		unsigned long flags);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize,
		unsigned long flags, void* content);
	Status deleteSegment(VirtualAddress startAddress);
	Status pageFault(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);
	Status createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags);
	Status disconnectSharedSegment(const char* name);
	Status deleteSharedSegment(const char* name);
	Process* clone(ProcessId pid);
	//TODO RETURN PRIVATE
private:
	KernelProcess *pProcess;
	Process(ProcessId, KernelProcess*);
	friend class System;
	friend class KernelSystem;
	friend class KernelProcess;
};
#endif