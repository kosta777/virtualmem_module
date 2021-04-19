#ifndef _pmt0_h_
#define _pmt0_h_

#include "vm_declarations.h"
#include "pmt1.h"
#include "kernelsystem.h"
#include "pmt1entry.h"

class Pmt0
{
public:
	static const unsigned entriesNumber = 256;

	bool haveFreeFromAddress(unsigned pmt0Address, unsigned pmt1Address, unsigned count, unsigned *pagesReqNum);

	Status allocateFromAddress(KernelSystem *ks, unsigned pmt0Address, unsigned pmt1Address, unsigned count, unsigned long accessRights, bool fill, void* content);
	Status allocateFromShared(KernelSystem *ks, Pmt0 *pmt0Shared, unsigned pmt0Address, unsigned pmt1Address, unsigned count, AccessType accessRights);
	Status allocateFromAddressForKernelSystem(KernelSystem *ks, unsigned pmt0Address, unsigned pmt1Address, unsigned count, unsigned long accessRights, bool fill, void* content);

	Pmt1Entry* getEntry(VirtualAddress);
	Pmt1Entry* getEntry(unsigned long pmt0Address, unsigned long pmt1Address);
	AccessType getAccessRights(VirtualAddress vadr);
private:
	Pmt1* entries[entriesNumber];
	friend class KernelProcess;
	friend class KernelSystem;
};

#endif