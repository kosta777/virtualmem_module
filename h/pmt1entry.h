#ifndef _pmt1_entry_h_
#define _pmt1_entry_h_

#include "vm_declarations.h"

class Pmt1Entry
{
public:
	Pmt1Entry(unsigned long rights, unsigned long segmentSize, unsigned long clusterNum);
	Pmt1Entry(Pmt1Entry *sharedPmt1Entry, AccessType rights);//for existing shared segments initialization
	bool isUsed();
	void setUnused();
	unsigned isInPhysicalMemory();
	void setInPhysicalMemory(unsigned long frame);
	unsigned long getFrameNum();
	void setUnloaded();
	void setDirty();
	bool isDirty();
	void setClean();
	bool isShared();
	Pmt1Entry* getSharedEntry();
	unsigned hasReadRights();
	unsigned hasWriteRights();
	unsigned hasExecuteRights();
	AccessType getAccessRights();
private:
	union FrameOrEntry {
		unsigned long frameNum;
		Pmt1Entry *sharedEntry;
	} frameOrEntry;
	unsigned long rights; // ReferencedBit(2^16); 2.Byte = dodatni biti referenciranja ; IsShared (64) Loaded(32) Dirty(16) Used(8) R(4) W(2) X(0)
	unsigned long swapSpaceClusterNum;
	unsigned long segmentSize; //0 -> in the middle of the segment, X>0 -> beginning of the segment

	friend class Pmt0;
	friend class Pmt1;
	friend class KernelProcess;
	friend class KernelSystem;
};

#endif