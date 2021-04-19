#include "pmt1entry.h"
#include <iostream>

Pmt1Entry::Pmt1Entry(unsigned long rights, unsigned long segmentSize, unsigned long clusterNum)
{
	this->rights = rights;
	this->rights = this->rights | 8; //sets Used bit to 1
	this->segmentSize = segmentSize;
	this->swapSpaceClusterNum = clusterNum;
	//std::cout << "Pmt1Entry created. segmentSize=" << segmentSize << " clusterNum=" << swapSpaceClusterNum << " rights=" << rights << std::endl;
}
//Samo za shared
Pmt1Entry::Pmt1Entry(Pmt1Entry *sharedPmt1Entry, AccessType rights)
{
	this->rights = rights;
	this->rights = this->rights | 8; //sets Used bit to 1
	this->rights = this->rights | 64; 
	this->frameOrEntry.sharedEntry = sharedPmt1Entry;
	//std::cout << "Pmt1Entry created. segmentSize=" << segmentSize << " clusterNum=" << swapSpaceClusterNum << " rights=" << rights << std::endl;
}

bool Pmt1Entry::isUsed()
{
	return rights & 8;
}

void Pmt1Entry::setUnused()
{
	rights &= ~(1ul << 3);
	frameOrEntry.frameNum = 0;
	swapSpaceClusterNum = 0;
	segmentSize = 0;
}

bool Pmt1Entry::isShared()
{
	return rights & 64;
}

Pmt1Entry* Pmt1Entry::getSharedEntry()
{
	if (!isShared())
		return 0;
	return frameOrEntry.sharedEntry;
}
unsigned Pmt1Entry::isInPhysicalMemory()
{
	return rights & 32;
}

void Pmt1Entry::setInPhysicalMemory(unsigned long frame)
{
	rights |= 32;
	frameOrEntry.frameNum = frame;
	setClean();
}

unsigned long Pmt1Entry::getFrameNum()
{
	return frameOrEntry.frameNum;
}

void Pmt1Entry::setUnloaded()
{
	rights &= (0xDF);
}

void Pmt1Entry::setDirty()
{
	rights |= 16;
}

bool Pmt1Entry::isDirty()
{
	return rights & 16;
}

void Pmt1Entry::setClean()
{
	rights &= ~(1ul << 4);
}

unsigned Pmt1Entry::hasReadRights()
{
	return rights & 4;
}
unsigned Pmt1Entry::hasWriteRights()
{
	return rights & 2;
}
unsigned Pmt1Entry::hasExecuteRights()
{
	return rights & 1;
}

AccessType Pmt1Entry::getAccessRights()
{
	AccessType t = (AccessType)(rights & 7);
	return t;
}