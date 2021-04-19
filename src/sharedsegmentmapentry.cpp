#include "sharedsegmentmapentry.h"

SharedSegmentMapEntry::SharedSegmentMapEntry(Pmt0 *pmt0)
{
	this->pmt0 = pmt0;
	this->count = 1;
	this->processes = new std::unordered_map<ProcessId, VirtualAddress>();
}

unsigned long SharedSegmentMapEntry::getCount() const
{
	return count;
}

void SharedSegmentMapEntry::inc(ProcessId pid, VirtualAddress vadr)
{
	count++;
	processes->insert({pid, vadr});
}

void SharedSegmentMapEntry::dec(ProcessId pid)
{
	count--;
	processes->erase(pid);
}

VirtualAddress SharedSegmentMapEntry::getVirtualAddressForProcess(ProcessId pid)
{
	auto it = processes->find(pid);
	return it->second;
}

std::unordered_map<ProcessId, VirtualAddress> *SharedSegmentMapEntry::getProcessMap()
{
	return processes;
}

SharedSegmentMapEntry::~SharedSegmentMapEntry()
{
	delete processes;
}