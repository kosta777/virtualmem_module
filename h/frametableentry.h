#ifndef _frame_table_entry_h_
#define _frame_table_entry_h_

#include "vm_declarations.h"
#include "pmt1entry.h"

class FrameTableEntry
{
public:
	FrameTableEntry()
	{
		frameFree = 1;
		pmt1entry = 0;
	}

	bool isFree();
	void setFree(bool value);
	void setReferenced();
	void shiftReferencedBits();
	unsigned getReferencedBitsValue();
private:
	friend class KernelSystem;
	friend class KernelProcess;
	unsigned long frameFree; // ReferencedBit(2^16); 2.Byte = dodatni biti referenciranja ; (0) -> isFree
	Pmt1Entry *pmt1entry;
};

#endif