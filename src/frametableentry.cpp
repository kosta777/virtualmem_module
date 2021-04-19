#include "frametableentry.h"

bool FrameTableEntry::isFree()
{
	return frameFree & 1;
}
void FrameTableEntry::setFree(bool value)
{
	if (value)
		frameFree |= 1;
	else
		frameFree &= (~(0ul) << 1);
}


void FrameTableEntry::setReferenced()
{
	frameFree |= 1 << 16;
}

void FrameTableEntry::shiftReferencedBits()
{
	unsigned bits = frameFree >> 9;
	frameFree &= 0x00FF;
	frameFree |= (bits << 8);
}

unsigned FrameTableEntry::getReferencedBitsValue()
{
	return frameFree >> 8;
}