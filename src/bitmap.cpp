#include "bitmap.h"
#include <cstring>
#include "vm_declarations.h"
#include <iostream>
Bitmap::Bitmap(PhysicalAddress adr, unsigned long size_in_bytes, unsigned long size_in_bits)
{
	map = (char*)adr;
	memset(adr, 0, size_in_bytes);
	this->sizeInBits = size_in_bits;
	this->sizeInBytes = size_in_bytes;
	lastFree = 0;
}

unsigned long Bitmap::getFreeCluster()
{
	for (unsigned long i = lastFree; i < sizeInBytes; i++)
	{
		if (map[i] == ~0)
			continue;
		unsigned last = 8;
		if (i == sizeInBytes - 1)
			last = sizeInBits % 8;
		for (int j = 0; j < last; j++)
			if ((map[i] & (1ul << j)) == 0)
			{
				lastFree = i;
				return j + i * 8;
			}
	}
}

void Bitmap::setBit(unsigned long num, bool state)
{
	if (num >= sizeInBits)
		return;
	
	unsigned pos = num % 8;

	if (state)
		map[num / 8] = map[num / 8] | (1 << pos);
	else
	{
		map[num / 8] = map[num / 8] & (~(1 << pos));
		if ((num / 8) < lastFree)
			lastFree = num / 8;
	}
}