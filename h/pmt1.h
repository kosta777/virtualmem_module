#ifndef _pmt1_h_
#define _pmt1_h_

#include "vm_declarations.h"
#include "pmt1entry.h"

class Pmt1
{
public:
	static const unsigned entriesNumber = 64;

private:
	Pmt1Entry entries[entriesNumber];
	friend class Pmt0;
	friend class KernelProcess;
};

#endif