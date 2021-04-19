#ifndef _shared_segment_map_entry_
#define _shared_segment_map_entry_
#include "vm_declarations.h"
#include <unordered_map>

class Pmt0;

class SharedSegmentMapEntry
{
public:
	SharedSegmentMapEntry(Pmt0*);
	~SharedSegmentMapEntry();

	unsigned long getCount() const;
	void inc(ProcessId, VirtualAddress);
	void dec(ProcessId);
	VirtualAddress getVirtualAddressForProcess(ProcessId);
	std::unordered_map<ProcessId, VirtualAddress> *getProcessMap();
private:
	Pmt0 *pmt0;
	unsigned long count;
	std::unordered_map<ProcessId, VirtualAddress> *processes;
	friend class KernelSystem;
	friend class SharedSegmentMapPointer;
};

#endif