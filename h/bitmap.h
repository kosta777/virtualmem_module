#ifndef _bitmap_h_
#define _bitmap_h_

#include "vm_declarations.h"

class Bitmap
{
public:
	Bitmap(PhysicalAddress address, unsigned long size_in_bytes, unsigned long size_in_bits);
	unsigned long getFreeCluster();
	void setBit(unsigned long num, bool state);

private:
	char* map;
	unsigned long sizeInBits;
	unsigned long sizeInBytes;
	unsigned long lastFree;
};

#endif